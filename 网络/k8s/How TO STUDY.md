- 没问题！给你一条**面向实操、适配国内网络、覆盖从入门到进阶**的详细学习路线。按周推进，每周都包含：学习目标 → 环境前置 → 实验任务（含命令/YAML）→ 验收点 → 常见坑与排障。你现在是 K8s 1.28 + Flannel + 1 控制面 + 1 工作节点，路线完全匹配。

  ------

  # 第 0 周｜地基与诊断套路（0.5–1 天）

  **目标**：熟悉集群构成、关键组件与基本排障三板斧。
   **前置**：确认控制面 `NoSchedule`，业务只跑在 `worker-1`（可保持不变）。

  ### 实验

  1. **集群体检**

  ```bash
  kubectl version --short
  kubectl get nodes -o wide
  kubectl -n kube-system get pods -o wide
  kubectl get svc,endpoints -A | column -t
  ```

  1. **排障三板斧**（任选一个 Pod）

  ```bash
  kubectl describe pod <pod>
  kubectl logs <pod> -c <container> --tail=100
  kubectl get events --sort-by=.lastTimestamp | tail -n 30
  ```

  1. **网络基线**

  ```bash
  ip -br a | egrep 'cni0|flannel' || true
  ip route | egrep '10\.244|flannel|cni0' || true
  kubectl -n kube-system get pods | egrep 'coredns|kube-proxy|flannel'
  ```

  **验收点**：能快速定位“镜像拉不动/CNI 未就绪/探针失败”等常见问题。

  ------

  # 第 1 周｜工作负载 & 服务发现（1–2 天）

  **目标**：理解 Pod/Deployment、Service/Endpoints、CoreDNS。

  ### 实验 1：三件套（Deployment + ClusterIP）

  ```
  web.yaml
  apiVersion: apps/v1
  kind: Deployment
  metadata: { name: web }
  spec:
    replicas: 3
    selector: { matchLabels: { app: web } }
    template:
      metadata: { labels: { app: web } }
      spec:
        containers:
        - name: nginx
          image: docker.m.daocloud.io/library/nginx:latest
          ports: [{ name: http, containerPort: 80 }]
  ---
  apiVersion: v1
  kind: Service
  metadata: { name: web }
  spec:
    selector: { app: web }
    ports: [{ name: http, port: 80, targetPort: http }]
    type: ClusterIP
  kubectl apply -f web.yaml
  kubectl get pods -l app=web -o wide
  kubectl get svc web -o wide
  kubectl get endpoints web -o wide
  ```

  ### 实验 2：DNS 验证（集群内）

  ```bash
  kubectl run -it --rm dns --image=docker.m.daocloud.io/library/busybox:stable --restart=Never -- \
    sh -c 'cat /etc/resolv.conf; nslookup web.default.svc.cluster.local; wget -qO- http://web | head -n 3'
  ```

  ### 实验 3：NodePort 对外

  ```bash
  kubectl patch svc web -p '{"spec":{"type":"NodePort"}}'
  kubectl get svc web -o wide  # 记下 NodePort
  curl http://<任一NodeIP>:<NODE_PORT>
  ```

  **验收点**：能用 DNS 名访问；理解 ClusterIP 与 NodePort 的差异。
   **常见坑**：宿主机防火墙未放行 NodePort（30000–32767/TCP）。

  ------

  # 第 2 周｜健康检查、滚动与配置注入（1–2 天）

  **目标**：掌握 Liveness/Readiness，滚动发布/回滚，ConfigMap/Secret。

  ### 实验 1：加探针 + 滚动

  ```bash
  kubectl patch deploy web -p '{
   "spec":{"template":{"spec":{"containers":[{"name":"nginx",
   "livenessProbe":{"httpGet":{"path":"/","port":"http"},"initialDelaySeconds":5,"periodSeconds":10},
   "readinessProbe":{"httpGet":{"path":"/","port":"http"},"initialDelaySeconds":2,"periodSeconds":5}}]}}}}'
  kubectl rollout status deploy/web
  ```

  触发一次升级 & 回滚：

  ```bash
  kubectl set image deploy/web nginx=docker.m.daocloud.io/library/nginx:1.25-alpine
  kubectl rollout history deploy/web
  kubectl rollout undo deploy/web
  ```

  ### 实验 2：ConfigMap/Secret 注入

  ```bash
  kubectl create configmap web-conf --from-literal WELCOME="hello"
  kubectl create secret generic web-secret --from-literal TOKEN="abc123"
  kubectl patch deploy web -p '{
   "spec":{"template":{"spec":{"containers":[{"name":"nginx",
   "env":[{"name":"WELCOME","valueFrom":{"configMapKeyRef":{"name":"web-conf","key":"WELCOME"}}},
          {"name":"TOKEN","valueFrom":{"secretKeyRef":{"name":"web-secret","key":"TOKEN"}}}
   ]}]}}}}'
  kubectl exec -it deploy/web -- sh -c 'echo $WELCOME-$TOKEN'
  ```

  **验收点**：能平滑升级&回滚；探针配置正确（不抖动）。
   **常见坑**：探针路径/端口写错引起重启；忘记 `rollout status` 等待生效。

  ------

  # 第 3 周｜调度与拓扑（1–2 天）

  **目标**：控制“跑在哪里”，掌握亲和/反亲和、污点容忍、拓扑均衡。

  ### 实验 1：固定到 worker-1

  ```bash
  kubectl label node worker-1 role=web --overwrite
  kubectl patch deploy web -p '{"spec":{"template":{"spec":{"nodeSelector":{"role":"web"}}}}}'
  kubectl rollout restart deploy/web
  kubectl get pods -l app=web -o wide
  ```

  ### 实验 2：亲和/反亲和（软约束）+ 拓扑均衡

  ```bash
  # 反亲和：尽量不把同 label 的 Pod 放到同一节点
  kubectl patch deploy web --type='json' -p='[
   {"op":"add","path":"/spec/template/spec/affinity","value":{
     "podAntiAffinity":{"preferredDuringSchedulingIgnoredDuringExecution":[
       {"weight":100,"podAffinityTerm":{"topologyKey":"kubernetes.io/hostname",
        "labelSelector":{"matchLabels":{"app":"web"}}}}]}}}
  ]'
  # 拓扑均衡：跨节点均匀分布
  kubectl patch deploy web --type='json' -p='[
   {"op":"add","path":"/spec/template/spec/topologySpreadConstraints","value":[
    {"maxSkew":1,"topologyKey":"kubernetes.io/hostname","whenUnsatisfiable":"ScheduleAnyway",
     "labelSelector":{"matchLabels":{"app":"web"}}}
   ]}]'
  kubectl rollout restart deploy/web
  ```

  ### 实验 3：污点与容忍（控制面只控不跑）

  ```bash
  kubectl taint nodes ubuntu node-role.kubernetes.io/control-plane=:NoSchedule --overwrite
  # 需要恢复：
  # kubectl taint nodes ubuntu node-role.kubernetes.io/control-plane:NoSchedule-
  ```

  **验收点**：能通过 nodeSelector/affinity/taints 调控部署位置；副本分布均衡。
   **常见坑**：硬约束叠加导致 Pending；通过 `kubectl describe pod` 看调度失败原因。

  ------

  # 第 4 周｜Ingress 与可观测（1–2 天）

  **目标**：引入七层入口，掌握基本监控/日志/事件。

  ### 实验 1：安装 NGINX Ingress（国内镜像友好）

  > 如需我可以给你一份适配 1.28 + NodePort/DaemonSet 模式的安装清单，镜像统一指向阿里/DaoCloud。
  >  安装后：

  ```bash
  kubectl -n ingress-nginx get pods -o wide
  kubectl get svc -n ingress-nginx
  ```

  ### 实验 2：两域名路由

  ```yaml
  apiVersion: networking.k8s.io/v1
  kind: Ingress
  metadata:
    name: web-ing
    annotations:
      nginx.ingress.kubernetes.io/rewrite-target: /
  spec:
    rules:
    - host: app1.local
      http:
        paths:
        - path: /
          pathType: Prefix
          backend: { service: { name: web, port: { number: 80 }}}
    - host: app2.local
      http:
        paths:
        - path: /
          pathType: Prefix
          backend: { service: { name: web, port: { number: 80 }}}
  ```

  本机 `/etc/hosts` 绑定 `app1.local`/`app2.local` → 任一 NodeIP → 浏览器访问。

  ### 实验 3：观测

  ```bash
  # 安装 metrics-server（我可提供国内镜像版 YAML）
  kubectl top nodes
  kubectl top pods -A
  
  # 查看事件/错误
  kubectl get events --sort-by=.lastTimestamp | tail -n 20
  kubectl logs -n ingress-nginx deploy/ingress-nginx-controller --tail=100
  ```

  **验收点**：能通过域名访问；能用 `top/logs/events` 定位 4xx/5xx。

  ------

  # 第 5 周｜存储 & 任务编排（1–2 天）

  **目标**：理解 PV/PVC 基础；Job/CronJob 基本用法。

  ### 实验 1：hostPath（演示）

  ```yaml
  apiVersion: v1
  kind: PersistentVolume
  metadata: { name: pv-hostpath }
  spec:
    capacity: { storage: 1Gi }
    accessModes: [ReadWriteOnce]
    hostPath: { path: /data/pv1 }
  ---
  apiVersion: v1
  kind: PersistentVolumeClaim
  metadata: { name: pvc-web }
  spec:
    accessModes: [ReadWriteOnce]
    resources: { requests: { storage: 512Mi } }
  ```

  部署一个挂载 PVC 的 Pod，验证写入落到节点 `/data/pv1`。

  ### 实验 2：Job/CronJob

  ```yaml
  apiVersion: batch/v1
  kind: CronJob
  metadata: { name: hello-cron }
  spec:
    schedule: "*/2 * * * *"
    jobTemplate:
      spec:
        template:
          spec:
            restartPolicy: OnFailure
            containers:
            - name: echo
              image: docker.m.daocloud.io/library/busybox:stable
              command: ["sh","-c","date; echo hello"]
  kubectl get cronjob
  kubectl get jobs --watch
  kubectl logs job/<job-name>
  ```

  **验收点**：理解存储访问模式；能做定时任务并查看日志。
   **常见坑**：hostPath 仅演示用；生产应用 StorageClass/CSI。

  ------

  # 第 6 周｜自动伸缩 & RBAC（1–2 天）

  **目标**：掌握 HPA 与基本权限控制。

  ### 实验 1：HPA（CPU）

  ```bash
  # metrics-server 就绪后：
  kubectl autoscale deploy web --cpu-percent=50 --min=2 --max=6
  kubectl get hpa -w
  # 人工压测可用 busybox 循环 curl 或 wrk/hey（宿主机）
  ```

  ### 实验 2：RBAC

  ```bash
  # 只读角色 + 绑定到某用户/SA
  kubectl create role view-pods --verb=get,list,watch --resource=pods
  kubectl create rolebinding view-pods-bind --role=view-pods --serviceaccount=default:default
  ```

  验证：用该 SA 的 kubeconfig 只能看 Pod 不能改。

  **验收点**：HPA 能随压力变化；RBAC 生效。
   **常见坑**：metrics-server TLS/证书问题 → 用国内镜像/关闭聚合 TLS（我可给已验证 YAML）。

  ------

  ## 附录 A｜国内镜像快捷指令

  - **kube-proxy 换源（跟随控制面版本）**

  ```bash
  kubectl -n kube-system set image ds/kube-proxy \
    kube-proxy=registry.aliyuncs.com/google_containers/kube-proxy:v1.28.2
  ```

  - **Flannel 换源**

  ```bash
  kubectl -n kube-system set image ds/kube-flannel-ds --all \
    kube-flannel=docker.m.daocloud.io/flannel/flannel:v0.25.5 \
    cni-plugin=docker.m.daocloud.io/flannel/flannel-cni-plugin:v1.5.1-flannel1
  ```

  ------

  ## 附录 B｜常用“状态/排障”清单

  ```bash
  # 入口与后端
  kubectl get svc,endpoints -A | column -t
  kubectl get endpointslice -A -o wide
  
  # 调度 & 事件
  kubectl get pods -A -o wide
  kubectl describe pod <pod>
  kubectl get events --sort-by=.lastTimestamp | tail -n 30
  
  # 节点健康
  kubectl describe node worker-1 | sed -n '1,150p' | egrep 'Taints|Conditions|Capacity|Allocatable'
  ```

  ------

  ## 进阶选修（完成主线后可挑）

  - **拓扑分布高级用法**：`topologySpreadConstraints` 配合多 AZ/Zone。
  - **NetworkPolicy**：基于命名空间/label 的南北向/东西向访问控制。
  - **Gateway API**：替代/增强 Ingress 的新标准。
  - **Prometheus/Grafana**：拉取 kubelet/cadvisor/ingress 指标做仪表盘。
  - **多节点实验**：再加 1 个 worker，验证跨节点负载与亲和/反亲和。

  ------

  ## 交付物建议（每周沉淀）

  - `manifests/`：本周所有 YAML（含注释）。
  - `notes/`：问题 & 结论 & 命令速查。
  - `checklist.md`：每周验收点打勾记录。