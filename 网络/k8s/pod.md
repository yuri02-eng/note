# Kubernetes Pod 详细笔记

## 1. Pod 总览（你需要先记住的 5 件事）

* **最小可部署单元**：K8s 调度的是 Pod，不是单个容器。
* **同生共死**：一个 Pod 内多个容器共享网络命名空间与卷，生命周期一致。
* **短暂可替换**：Pod 自身是易失的；稳定性靠 Deployment/StatefulSet 等控制器。
* **就绪才能接流量**：只有 Readiness 通过的 Pod 才会被加到 Service 的 endpoints。
* **优雅终止**：删除 Pod 会先从 endpoints 移除→发 SIGTERM→等宽限期→SIGKILL。

---

## 2. Pod 对象结构（高层视角）

```text
apiVersion / kind / metadata / spec / status
```

* **metadata**：名字、命名空间、labels、annotations、ownerReferences。
* **spec**：最关键部分，描述容器、卷、探针、调度、安全、网络等。
* **status**：运行态信息：phase、conditions、(init)containerStatuses、podIP 等。

---

## 3. metadata 常用字段

* `name`：Pod 名称（避免与其他对象重名）。
* `namespace`：命名空间。
* `labels`：用于选择器（Service/Deployment/NetworkPolicy/亲和）。
* `annotations`：非结构化元信息（变更记录、ServiceMesh 注入、seccomp、AppArmor 等）。
* `ownerReferences`：指向上层控制器（删除控制器会级联删除 Pod）。

---

## 4. spec：运行与容器相关（核⼼）

### 4.1 containers（业务容器，必填）

常见字段：

* `name`、`image`、`imagePullPolicy`（IfNotPresent/Always）
* `command`/`args`：覆盖镜像 ENTRYPOINT/CMD。
* `workingDir`
* `ports`：容器开放的端口（仅元数据，非主机端口）。
* `env` / `envFrom`：环境变量（支持从 ConfigMap/Secret 批量注入）。

  * `valueFrom.fieldRef`/`resourceFieldRef`/`configMapKeyRef`/`secretKeyRef`
* `resources`：

  * `requests.cpu/memory`（参与调度）
  * `limits.cpu/memory`（上限，内存超限 OOMKilled）
* `volumeMounts`：挂载卷（`subPath` 可挂子目录；注意：`subPath` 配合 ConfigMap/Secret **不热更新**）。
* `lifecycle.postStart/preStop`：容器生命周期钩子。
* `securityContext`（容器级）：`runAsUser/group`、`runAsNonRoot`、`capabilities`、`allowPrivilegeEscalation`、`readOnlyRootFilesystem`、`seccompProfile` 等。
* 健康检查（见 §6）：`livenessProbe` / `readinessProbe` / `startupProbe`

### 4.2 initContainers（初始化容器，可选）

* 顺序执行，全部成功后主容器才会启动；失败会重试。
* 典型用途：等待外部依赖、拷贝配置、做迁移、修权限。

### 4.3 ephemeralContainers（临时容器，调试用）

* 运行时注入（`kubectl debug`），不参与重启策略，不支持端口与探针。

### 4.4 volumes（见 §8）

* `emptyDir`、`hostPath`、`configMap`、`secret`、`downwardAPI`、`projected`、`persistentVolumeClaim` 等。
* 在容器中通过 `volumeMounts` 绑定路径。

### 4.5 其他运行相关

* `restartPolicy`：`Always`（Deployment/STS 必须）、`OnFailure`、`Never`。
* `terminationGracePeriodSeconds`：优雅关停时间窗（默认 30s）。
* `shareProcessNamespace`：同 Pod 容器互相可 `ps`（调试用）。
* `enableServiceLinks`：是否把服务注入为环境变量（默认 true，建议关闭以减噪）。

---

## 5. spec：调度与容忍

* `nodeName`：强制到某节点（一般不直接用）。
* `nodeSelector`：简单键值匹配。
* `affinity`：

  * `nodeAffinity`：按节点标签选（如 `topology.kubernetes.io/zone`）。
  * `podAffinity/podAntiAffinity`：按 Pod 标签聚集/打散。
  * `requiredDuringSchedulingIgnoredDuringExecution`（强）/`preferred...`（弱）。
* `tolerations`：可落在带污点（taint）的节点上。effect：`NoSchedule`/`PreferNoSchedule`/`NoExecute`。
* `topologySpreadConstraints`：跨 zone/节点均衡分布，防热点。
* `priorityClassName`：优先级与抢占。
* `schedulerName`：自定义调度器（少用）。

---

## 6. 健康检查（Probes）

**目的**：把“活着／可服务／启动完毕”三个状态拆开管理。

* **livenessProbe**（存活）：失败 → kubelet 重启容器。
* **readinessProbe**（就绪）：失败 → 从 Service endpoints 移除（不接新流量）。
* **startupProbe**（启动期保护）：未通过前，liveness/readiness 不生效；适合慢启动应用。
* 方式：`httpGet` / `tcpSocket` / `exec`
* 关键参数：`initialDelaySeconds`、`periodSeconds`、`timeoutSeconds`、`failureThreshold`、`successThreshold`
* 常见误区：

  * 只配 Liveness 不配 Readiness → 未就绪就接流量。
  * 慢启动没配 Startup → 被 Liveness 误杀。
  * `timeoutSeconds` 太短导致误报。

---

## 7. 网络与 DNS

* **共享网络命名空间**：同 Pod 容器走 `localhost` 通信；端口号需避免冲突。
* **Service**：提供稳定虚 IP/DNS；由 EndpointSlice 追踪就绪 Pod。
* **Headless Service**（`clusterIP: None`）：DNS 直接解析到 Pod IP（StatefulSet 常用）。
* `hostNetwork/hostPort`：与主机共网或占主机端口（谨慎使用，注意冲突与安全）。
* `dnsPolicy`：默认 `ClusterFirst`；hostNetwork 时默认为 `ClusterFirstWithHostNet`。
* `dnsConfig`：自定义 `nameservers`、`searches`、`options`。

---

## 8. 存储（Volumes）详解

* **emptyDir**：随 Pod 生命周期；`medium: Memory` 走内存盘。
* **hostPath**：挂主机路径（仅特定场景用，注意权限与耦合）。
* **configMap/secret**：注入配置与密钥（文件或 env），文件方式支持热更新（有传播延迟；`subPath` 方式不热更新）。
* **downwardAPI**：把 Pod/容器信息（name、labels、requests/cpu 等）暴露为文件/env。
* **projected**：把多种源合并挂载到一个卷目录。
* **persistentVolumeClaim**（PVC）：持久卷声明，配合 PV/StorageClass 动态供应。

  * 访问模式：`RWO`（单节点写）/`RWX`（多节点写）/`ROX`。
  * 扩容：StorageClass 需开 `allowVolumeExpansion: true`。

---

## 9. 安全（ServiceAccount / SecurityContext / PSA）

* **serviceAccountName**：Pod 访问 API Server 的身份；不需要时可 `automountServiceAccountToken: false`。
* **securityContext（Pod/容器级）**：

  * `runAsUser/group`、`runAsNonRoot`、`fsGroup`
  * `capabilities.add/drop`、`allowPrivilegeEscalation: false`、`readOnlyRootFilesystem: true`
  * `seccompProfile`、`seLinuxOptions`（平台相关）
* **Pod Security Admission (PSA)**：`privileged` / `baseline` / `restricted`（命名空间级防护基线）。
* 少用 `privileged`、`hostPath`、`hostNetwork`；按最小权限原则配置 RBAC。

---

## 10. 终止流程（优雅下线）

1. 从 Service endpoints 移除（不再接新连接）。
2. 触发 `preStop`（如设置）。
3. kubelet 向主容器发 `SIGTERM`。
4. 等待 `terminationGracePeriodSeconds`（默认 30s）。
5. 超时未退出 → `SIGKILL`。

**建议**：

* `preStop` 里 `sleep 5-10`，让连接耗尽；应用需捕获 SIGTERM 做收尾（关闭监听/flush 队列）。
* 后台 worker 可适当把宽限期调大（60–120s）。

---

## 11. status：运行态与排障入口

* `phase`：`Pending` / `Running` / `Succeeded` / `Failed` / `Unknown`
* `conditions`：`PodScheduled`、`Initialized`、`Ready`、`ContainersReady`
* `containerStatuses[*]`：

  * `state`（`waiting/running/terminated`）
  * `lastState`（上次状态）
  * `restartCount`（重启次数）
  * 结合 `kubectl describe pod` 的 Events 快速定位问题（镜像拉取、调度失败、探针失败等）。

---

## 12. 常见模式与 YAML 模板

### 12.1 Web 服务（含三探针、优雅终止、资源）

```yaml
apiVersion: v1
kind: Pod
metadata: { name: web, labels: { app: web } }
spec:
  terminationGracePeriodSeconds: 40
  containers:
    - name: web
      image: nginx:1.25
      ports: [{ name: http, containerPort: 80 }]
      resources:
        requests: { cpu: "200m", memory: "256Mi" }
        limits:   { cpu: "500m", memory: "256Mi" }
      lifecycle:
        preStop: { exec: { command: ["sh","-c","sleep 10"] } }
      readinessProbe:
        httpGet: { path: /ready, port: http }
        initialDelaySeconds: 5
        periodSeconds: 5
      livenessProbe:
        httpGet: { path: /healthz, port: http }
        initialDelaySeconds: 15
        periodSeconds: 10
      startupProbe:
        httpGet: { path: /healthz, port: http }
        failureThreshold: 30
        periodSeconds: 5
```

### 12.2 Init + Sidecar（日志收集）

```yaml
apiVersion: v1
kind: Pod
metadata: { name: app-with-logger, labels: { app: demo } }
spec:
  volumes: [{ name: shared, emptyDir: {} }]
  initContainers:
    - name: wait-db
      image: busybox
      command: ["sh","-c","until nc -z db 5432; do sleep 2; done"]
  containers:
    - name: app
      image: myorg/app:1.0.0
      volumeMounts: [{ name: shared, mountPath: /var/log/app }]
    - name: log-shipper
      image: fluent/fluent-bit:2
      args: ["-i","tail","-p","path=/var/log/app/*.log","-o","stdout"]
      volumeMounts: [{ name: shared, mountPath: /var/log/app }]
```

### 12.3 调度约束 + 均衡分布 + 污点容忍

```yaml
apiVersion: v1
kind: Pod
metadata: { name: scheduled-app, labels: { app: demo } }
spec:
  containers: [{ name: app, image: myorg/app:2.0.0 }]
  nodeSelector: { kubernetes.io/os: linux }
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
          - matchExpressions:
              - key: topology.kubernetes.io/zone
                operator: In
                values: ["zone-a","zone-b"]
    podAntiAffinity:
      preferredDuringSchedulingIgnoredDuringExecution:
        - weight: 100
          podAffinityTerm:
            topologyKey: kubernetes.io/hostname
            labelSelector: { matchLabels: { app: demo } }
  tolerations:
    - key: "workload"  # 允许落在带污点的 batch 节点
      operator: "Equal"
      value: "batch"
      effect: "NoSchedule"
  topologySpreadConstraints:
    - maxSkew: 1
      topologyKey: topology.kubernetes.io/zone
      whenUnsatisfiable: ScheduleAnyway
      labelSelector: { matchLabels: { app: demo } }
```

---

## 13. kubectl 常用命令速览

```bash
# 观察与排错
kubectl get pods -o wide
kubectl describe pod <pod>
kubectl logs <pod> -c <container>
kubectl logs <pod> -c <container> --previous
kubectl exec -it <pod> -c <container> -- sh
kubectl port-forward pod/<pod> 8080:80
kubectl get events --sort-by=.lastTimestamp
kubectl top pod             # 需安装 metrics-server

# 临时容器调试
kubectl debug -it <pod> --image=busybox --target=<container>
```

---

## 14. 常见故障与定位思路

* **CrashLoopBackOff**：看 `--previous` 日志；检查 command/args、配置挂载、端口/权限。
* **ImagePullBackOff**：镜像名/Tag、仓库权限、`imagePullSecrets`。
* **Readiness 长期失败**：健康检查路径、外部依赖是否可达、NetworkPolicy 放行。
* **OOMKilled/Evicted**：`describe` 看原因；调 `limits`，优化内存；控制 `ephemeral-storage`。
* **调度失败**：看 events；是否被 taint 拒绝、affinity 条件过严、资源不足。
* **DNS/网络不通**：`dnsPolicy/dnsConfig`、CoreDNS 日志、NetworkPolicy 规则、Service/EndpointSlice。

---

## 15. 最佳实践清单（Checklist）

* ✅ 在线服务必须配 **Readiness**（必要时再加 Liveness/Startup）。
* ✅ 合理设定 `requests/limits`；稳定延迟优先 **Guaranteed** QoS。
* ✅ 优雅终止：`preStop + terminationGracePeriodSeconds`。
* ✅ 固定镜像标签（避免 `:latest`）；私库配 `imagePullSecrets`。
* ✅ 最小权限：关闭不需要的 SA token、RBAC 最小化、`allowPrivilegeEscalation: false`。
* ✅ 少用 `hostPath/hostNetwork/privileged`。
* ✅ 配置与密钥尽量用文件挂载；需要热更新时避免 `subPath`。
* ✅ 跨区/跨节点均衡：`topologySpreadConstraints` + 配合 PDB（控制器层面）。
* ✅ 日志走 stdout/stderr，或 Sidecar/Agent 转发，避免撑爆临时存储。
* ✅ 调试用 `kubectl debug` 注入临时容器，而非在业务镜像塞工具。

---

## 16. 自测与练习（建议跟着做）

1. **健康检查与就绪**：在 Nginx Pod 中延迟 10s 才返回 `/ready=200`，观察 endpoints 加入的时机。
2. **Init 容器**：用 `nc -z db 5432` 等待 DB，上线后启动主容器。
3. **资源与 OOM**：设置较低内存 limit，运行一个吃内存脚本，观察 `OOMKilled` 与 `restartCount`。
4. **拓扑分布**：设置 `topologySpreadConstraints`；故意让一个 zone 无节点，观察调度行为。
5. **优雅关停**：配置 `preStop: sleep 10`，并发压测下删除 Pod，确认请求无明显失败尖峰。

---

需要的话，我还能把这份笔记**导出为打印友好的 PDF/Markdown**，或根据你的运行环境（EKS/AKS/GKE/自建）**定制一版更贴近实战的 Pod 配置清单与踩坑列表**。你更偏向哪种格式？我可以直接生成对应文件给你。
