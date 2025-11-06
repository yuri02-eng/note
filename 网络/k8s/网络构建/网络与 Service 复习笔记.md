# Kubernetes 网络与 Service 知识笔记（详解版）

> 本笔记系统梳理 **安装之后** 的关键知识：Pod/CNI 网络、Service/Endpoints/EndpointSlice 机制、CoreDNS 解析、kube-proxy（iptables/IPVS）负载、数据流与源 IP、选择器匹配规则、Headless/无 selector/ExternalName 等多种 Service 形态，以及排障流程与可复制的最小/进阶示例。
>
> 你当前环境：Flannel（10.244.0.0/16），单控制面；示例用 `default` 命名空间。

---

## 1) 网络基础与角色分工

### 1.1 Node / Pod / Service 的地址体系

* **Node IP**（如 `192.168.241.137`）：宿主机/虚机在局域网中的地址；NodePort、SSH、节点间隧道都用它。
* **Pod 网段（CNI）**：例如 Flannel 使用 `10.244.0.0/16`；每个节点获分一个 `/24` 子网（如本机 `10.244.0.0/24`）。
* **Service 网段（ClusterIP CIDR）**：由 kube-proxy 负责“虚拟化”的 VIP 网段（如 `10.96.0.0/12`），**不是**真实网卡地址。

### 1.2 CNI 接口与隧道

* **cni0**：各节点的 Pod 网桥，对应该节点的 Pod 子网，通常网关为 `10.244.X.1`。
* **flannel.1**：VXLAN 隧道接口（UDP 8472），跨节点通信时封装/解封装 Pod 流量。

### 1.3 Pod 共享网络命名空间

* 创建 Pod 时先起 **pause**（沙箱）容器提供 **netns**；其他业务容器加入该 netns ⇒ **共享 Pod IP/路由/端口空间**，可用 `localhost` 互访。
* 端口冲突：同一 Pod 内不能同时绑定同一 `0.0.0.0:PORT`。

---

## 2) Service ⇄ Endpoints/EndpointSlice 机制

### 2.1 selector 与后端池

* **selector（等值匹配）**：`spec.selector: {app: web, tier: fe}` 在**同命名空间**按 **AND** 匹配 Pod 的 `.metadata.labels`。
* **EndpointSlice 控制器**：watch Pod/Service 变化；把“selector 命中 + Ready 的 Pod”写入 **EndpointSlice**（或 Endpoints）。
* **就绪门槛**：`readinessProbe` 失败 ⇒ Pod **不**纳入后端池。

### 2.2 kube-proxy 的两种数据面

* **iptables 模式**（默认广泛）：在每个节点编程 `KUBE-SVC-*`/`KUBE-SEP-*` 链；新连接通过概率/哈希在后端间**近似均衡**，再做 **DNAT** 到 `PodIP:Port`；**conntrack** 保持连接一致性。
* **IPVS 模式**：建立虚拟服务（ClusterIP/NodePort）与真实服务器（PodIP）；调度算法默认 **rr**，可配 **lc/wrr** 等；支持 `persistence` 实现 **ClientIP 粘性**。

### 2.3 影响分流的关键字段

* `sessionAffinity: ClientIP`（可配 `timeoutSeconds`）
* `internalTrafficPolicy: Local`：集群内访问时尽量只选**本节点后端**
* `externalTrafficPolicy: Local`：NodePort/LoadBalancer 保留客户端源 IP，仅转发本节点后端
* **Topology Aware Hints**：优先同 zone/同节点后端，减少跨域流量

---

## 3) DNS：CoreDNS 如何解析 Service 名

### 3.1 Pod 内的 resolv.conf

* `nameserver` 指向 `kube-dns` Service 的 **ClusterIP**（如 `10.96.0.10`）。
* `search`：`<ns>.svc.cluster.local`、`svc.cluster.local`、`cluster.local`，配合 `ndots` 规则简化名称。

### 3.2 kubernetes 插件的解析逻辑

* **普通 Service**：返回 **ClusterIP**（A 记录）；命名端口同时有 **SRV**。
* **Headless（clusterIP: None）**：直接返回 **所有后端 Pod IP**（多条 A 记录）；必要时可 `publishNotReadyAddresses: true`。
* **ExternalName**：返回 **CNAME** 指向外部域名，不产生 Endpoints/不经 kube-proxy。

### 3.3 NodeLocal DNS Cache（可选）

* 在每个节点上本地缓存与转发，降低 DNS 延迟与抖动（可按需部署）。

---

## 4) 数据流与源 IP 路径（带要点）

### 4.1 集群内：Pod → ClusterIP → Pod

1. DNS：`svc.ns.svc.cluster.local` → ClusterIP。
2. 连接 `ClusterIP:Port` 命中本节点 kube-proxy。
3. 选择后端 `PodIP:Port`（iptables ≈ 均衡；IPVS = 算法可选）。
4. 路由：同节点走 `cni0`；跨节点封装经 `flannel.1` 到目标节点再投递给 Pod。
5. **conntrack** 保证同一 TCP 连接不换后端。

### 4.2 集群外：Client → NodeIP:NodePort → Pod

1. 命中 NodePort 规则 ⇒ 定位对应 Service。
2. 从后端池选一个 `PodIP:Port` 并 DNAT。
3. **externalTrafficPolicy: Local** 时：只转发本节点后端，保留源 IP；否则可能经其他节点转发，源 IP 被 SNAT 成节点地址。

### 4.3 直接访问 Pod IP 的条件

* 集群/节点内：可直连 `10.244.x.y`。
* 集群外：默认无路由；可临时在外部主机加 `10.244.0.0/16` 静态路由经任一节点，或采用 NodePort/Ingress 暴露。

---

## 5) Service 四象限（看 `clusterIP`×`selector`）

| 类型                    | `clusterIP` | `selector` | 后端来源                     | DNS 返回             | 数据转发                      | 适用场景             |
| --------------------- | ----------- | ---------- | ------------------------ | ------------------ | ------------------------- | ---------------- |
| 普通 Service            | 有           | 有          | 控制器自动                    | **VIP**（ClusterIP） | kube-proxy VIP→Pod        | 常规微服务            |
| **Headless**          | **None**    | 有          | 控制器自动                    | **Pod IP 列表**      | 客户端直连 Pod（无 kube-proxy）   | StatefulSet/直连副本 |
| 无 selector（有 VIP）     | 有           | **无**      | **手工 Endpoints**（可外部 IP） | **VIP**            | kube-proxy VIP→外部 IP:Port | 纳管机房/外部服务        |
| 无 selector + Headless | **None**    | **无**      | 手工 Endpoints             | **你写的 IP 列表**      | 客户端直连外部 IP                | 直连外部实例           |
| **ExternalName**      | N/A         | N/A        | N/A                      | **CNAME** → 外部域名   | 不经 kube-proxy             | 仅需内部别名映射         |

---

## 6) 选择器与端口映射细节

* **selector 等值匹配**：仅支持 `key: value`，按 AND；跨命名空间不生效。
* **targetPort**：

  * 数字：直接指定后端端口；
  * 字符串：**命名端口**，在 Pod 容器 `ports[].name` 里找同名项映射到真实端口。
* **多端口 Service**：`spec.ports` 可列多个端口，支持不同 `name/port/targetPort/nodePort`。

---

## 7) 最小/进阶示例（可直接应用）

### 7.1 Deployment + ClusterIP（命名端口）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata: { name: web }
spec:
  replicas: 2
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
```

### 7.2 NodePort（保留客户端源 IP 的对外方案）

```yaml
apiVersion: v1
kind: Service
metadata: { name: web-nodeport }
spec:
  selector: { app: web }
  externalTrafficPolicy: Local   # 仅使用本节点后端，保留源 IP
  type: NodePort
  ports:
  - name: http
    port: 80
    targetPort: http
    nodePort: 30080             # 可自定，30000–32767
```

> 若某节点无后端 Pod，请求到该节点的 NodePort 可能会失败（因为不转发到其他节点）。可配合 DaemonSet/亲和性确保每节点有后端，或去掉 `externalTrafficPolicy: Local`。

### 7.3 Headless：DNS 直返 Pod IP（StatefulSet 友好）

```yaml
apiVersion: v1
kind: Service
metadata: { name: web-headless }
spec:
  clusterIP: None
  selector: { app: web }
  ports: [{ name: http, port: 80, targetPort: http }]
```

### 7.4 无 selector（有 VIP）：把外部后端挂进来

```yaml
apiVersion: v1
kind: Service
metadata: { name: external-apache }
spec:
  ports: [{ name: http, port: 80, targetPort: 80 }]
  type: ClusterIP
---
apiVersion: v1
kind: Endpoints
metadata: { name: external-apache }
subsets:
- addresses: [{ ip: 192.168.241.137 }]
  ports: [{ port: 80 }]
```

### 7.5 无 selector + Headless：DNS 直返外部 IP

```yaml
apiVersion: v1
kind: Service
metadata: { name: external-db-headless }
spec:
  clusterIP: None
  ports: [{ name: mysql, port: 3306 }]
---
apiVersion: v1
kind: Endpoints
metadata: { name: external-db-headless }
subsets:
- addresses: [{ ip: 192.168.241.200 }]
  ports: [{ port: 3306 }]
```

### 7.6 Ingress（对外 80/443 域名入口）

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: web-ing
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "false"
spec:
  ingressClassName: nginx
  rules:
  - host: web.local.test
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web
            port: { number: 80 }
```

> 需安装 ingress-nginx，并在本机 hosts 绑定 `web.local.test` 到某节点 IP。

---

## 8) 排障流程与命令清单

### 8.1 “入口 ⇄ 后端”是否匹配

```bash
kubectl get svc web -o wide
kubectl get endpoints web -o wide
kubectl get endpointslice -l kubernetes.io/service-name=web -o wide
```

* Endpoints 为空：label 不匹配/Pod 未就绪/端口名不对。

### 8.2 Pod 端监听/探针

```bash
kubectl get pods -l app=web -o wide --show-labels
kubectl describe pod -l app=web | sed -n '1,200p'
kubectl logs -l app=web --tail=200
```

### 8.3 DNS 解析链路

```bash
kubectl -n kube-system get svc kube-dns -o wide
kubectl run -it --rm dns --image=docker.m.daocloud.io/library/busybox:stable --restart=Never -- \
  sh -c 'cat /etc/resolv.conf; echo; nslookup web.default.svc.cluster.local || true'
```

### 8.4 kube-proxy 数据面

* **iptables**：

```bash
sudo iptables-save | egrep 'KUBE-SVC|KUBE-SEP|10\.|web'
```

* **IPVS**：

```bash
sudo ipvsadm -Ln | egrep -A3 'KUBE|:80'
```

### 8.5 CNI 与路由

```bash
ip -br a | egrep 'cni0|flannel'
ip route | egrep '10\.244|cni0|flannel'
```

### 8.6 NodePort/源 IP

* 无法保持源 IP：考虑 `externalTrafficPolicy: Local`；或在上游 LB 做透传。
* 某节点 NodePort 不通：该节点可能无后端；或防火墙未放行端口。

### 8.7 常见告警/问题

* `ImagePullBackOff`：国内镜像加速/预拉取。
* CoreDNS CrashLoop/无解析：先看 kube-dns Service/Endpoints，确认 CNI Ready。
* `Connection refused`：容器未监听或目标端口/命名端口不一致。

---

## 9) 进阶话题速记

* **Session 亲和**：

```yaml
spec:
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP: { timeoutSeconds: 10800 }
```

* **Topology Hints**：开启后，kube-proxy 优先同拓扑后端。
* **Internal/External TrafficPolicy**：Local 可降低跨节点转发并保源 IP。
* **StatefulSet 固定域名**：`<pod>.<headless-svc>.<ns>.svc.cluster.local`（如 `zk-0.zk.default.svc.cluster.local`）。
* **ExternalName vs 无 selector**：前者仅 DNS CNAME，不转发；后者可经 kube-proxy 转发到你指定的 IP。

---

## 10) 摘要与记忆法

* **入口稳定，后端弹性**：Service 保持 VIP ↔ Endpoints 映射；Pod 可随意扩缩容/漂移。
* **DNS 决策**：普通返回 VIP；Headless 直返 Pod IP 列表；ExternalName 返 CNAME。
* **数据面**：kube-proxy（iptables/IPVS）负责转发；conntrack 保连接一致；可配粘性/拓扑策略/保源 IP。
* **排障三板斧**：`svc+endpoints`、`dns+pod就绪`、`iptables/ipvs + CNI 路由`。

> 与“安装文档”配套：本页讲原理与运维；安装文档讲部署与脚本。按需速查即可。
