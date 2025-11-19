# 一、实验目标

通过本实验，你要真正搞懂并亲手完成这件事：

> **让 Kubernetes 在使用 NFS 时，自动为 PVC 创建 PV 和实际目录，
> Pod 删了数据还在，再挂上去还能看到之前的数据。**

具体来说，你将学会：

1. 使用 **NFS 作为后端存储**
2. 部署 **nfs-subdir-external-provisioner（动态 Provisioner）**
3. 创建 **StorageClass**，让集群知道如何“自动生产 PV”
4. 创建 **PVC（test-pvc）**，并自动绑定动态创建的 PV
5. 用 **Pod 挂载 PVC 到 /mnt，写入数据**
6. 删除 Pod 再重建，验证：**数据仍然存在（持久卷的意义）**

---

# 二、实验环境

* 系统：Ubuntu（K8s 节点 + NFS Server）
* Kubernetes：已安装好，`kubectl` 可用
* NFS 服务器：

  * IP：`192.168.241.144`
  * 共享目录：`/data/k8s`
* 所有 K8s 节点可以访问 NFS 服务器 `192.168.241.144:2049`

> **说明：**
> NFS 服务的安装你已经完成，这里重点从 “StorageClass 开始动态创建 PV” 这一段。

---

# 三、部署 NFS 动态 Provisioner

我们使用的是 **nfs-subdir-external-provisioner**（新一代 NFS 动态卷插件）。

## 1. 创建 ServiceAccount + RBAC

文件：`nfs-client-sa.yaml`

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: nfs-client-provisioner

---
kind: ClusterRole
apiVersion: rbac.authorization.k8s.io/v1
metadata:
  name: nfs-client-provisioner-runner
rules:
  - apiGroups: [""]
    resources: ["persistentvolumes"]
    verbs: ["get", "list", "watch", "create", "delete"]
  - apiGroups: [""]
    resources: ["persistentvolumeclaims"]
    verbs: ["get", "list", "watch", "update"]
  - apiGroups: ["storage.k8s.io"]
    resources: ["storageclasses"]
    verbs: ["get", "list", "watch"]
  - apiGroups: [""]
    resources: ["events"]
    verbs: ["list", "watch", "create", "update", "patch"]
  - apiGroups: [""]
    resources: ["endpoints"]
    verbs: ["create", "delete", "get", "list", "watch", "patch", "update"]

---
kind: ClusterRoleBinding
apiVersion: rbac.authorization.k8s.io/v1
metadata:
  name: run-nfs-client-provisioner
subjects:
  - kind: ServiceAccount
    name: nfs-client-provisioner
    namespace: default
roleRef:
  kind: ClusterRole
  name: nfs-client-provisioner-runner
  apiGroup: rbac.authorization.k8s.io
```

应用：

```bash
kubectl apply -f nfs-client-sa.yaml
```

---

## 2. 部署 nfs-subdir-external-provisioner

文件：`nfs-client.yaml`
（这里已经是你最终生效的配置）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nfs-client-provisioner
spec:
  replicas: 1
  selector:
    matchLabels:
      app: nfs-client-provisioner
  template:
    metadata:
      labels:
        app: nfs-client-provisioner
    spec:
      serviceAccountName: nfs-client-provisioner
      containers:
      - name: nfs-client-provisioner
        image: registry.cn-hangzhou.aliyuncs.com/lfy_k8s_images/nfs-subdir-external-provisioner:v4.0.2
        env:
        - name: PROVISIONER_NAME
          value: k8s-sigs.io/nfs-subdir-external-provisioner   # ⭐ 非常关键
        - name: NFS_SERVER
          value: 192.168.241.144                               # ⭐ 你的 NFS 服务器 IP
        - name: NFS_PATH
          value: /data/k8s                                     # ⭐ 共享目录
        volumeMounts:
        - name: nfs-client-root
          mountPath: /persistentvolumes
      volumes:
      - name: nfs-client-root
        nfs:
          server: 192.168.241.144
          path: /data/k8s
```

应用：

```bash
kubectl apply -f nfs-client.yaml
kubectl get pods -l app=nfs-client-provisioner
```

预期状态：

```text
nfs-client-provisioner-xxxx   1/1   Running
```

---

# 四、创建 StorageClass（让集群知道怎么“自动造 PV”）

文件：`nfs-client-class.yaml`

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: course-nfs-storage
provisioner: k8s-sigs.io/nfs-subdir-external-provisioner   # ⭐ 必须和 PROVISIONER_NAME 完全一致
parameters:
  pathPattern: "${.PVC.namespace}-${.PVC.name}"            # 创建子目录的命名规则
reclaimPolicy: Delete
volumeBindingMode: Immediate
```

应用：

```bash
kubectl apply -f nfs-client-class.yaml
kubectl get storageclass
```

预期结果：

```text
NAME                 PROVISIONER                                   AGE
course-nfs-storage   k8s-sigs.io/nfs-subdir-external-provisioner   ...
```

> ✅ 到这里为止：
> StorageClass + Provisioner 这一层就全部打通了。

---

# 五、创建 PVC（test-pvc），触发“动态创建 PV”

文件：`test-pvc.yaml`

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: test-pvc
spec:
  storageClassName: course-nfs-storage   # ⭐ 指定使用上面的 StorageClass
  accessModes:
  - ReadWriteMany                        # NFS 支持多节点读写
  resources:
    requests:
      storage: 1Mi                       # 实验用 1Mi 足够
```

创建：

```bash
kubectl apply -f test-pvc.yaml
kubectl get pvc
```

预期效果：

```text
NAME      STATUS   VOLUME                                     CAPACITY   ACCESS MODES   STORAGECLASS         AGE
test-pvc  Bound    pvc-02937a4d-a1ee-454e-ad56-a749572fb92b   1Mi        RWX            course-nfs-storage   ...
```

同时查看 PV：

```bash
kubectl get pv
```

会看到多出一个自动生成的 PV，`STORAGECLASS=course-nfs-storage`。

> 到这里为止，你已经完成了：
> **PVC → StorageClass → Provisioner 自动创建 PV**

---

# 六、在 NFS 上验证目录自动创建

登录到 NFS 服务器 `192.168.241.144`，执行：

```bash
ls /data/k8s
```

你会看到类似这样的目录：

```text
default-test-pvc-pvc-02937a4d-a1ee-454e-ad56-a749572fb92b
```

这就说明：

> Provisioner 已经在 NFS 上为 `test-pvc` 创建了一个独立的数据目录。

---

# 七、创建 Pod 挂载 PVC，并写入测试数据

现在用一个 Pod 来挂载 `test-pvc`，往里面写文件。

文件：`test-pod.yaml`

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: test-pod
spec:
  restartPolicy: Never
  containers:
  - name: test-pod
    image: busybox
    command: ["/bin/sh"]
    args:
      - -c
      - |
        echo "hello from dynamic pvc" > /mnt/SUCCESS;
        echo "sleeping..."; sleep 3600
    volumeMounts:
    - name: nfs-vol
      mountPath: /mnt
  volumes:
  - name: nfs-vol
    persistentVolumeClaim:
      claimName: test-pvc           # ⭐ 使用我们刚才创建的 PVC
```

创建并查看状态：

```bash
kubectl apply -f test-pod.yaml
kubectl get pod test-pod
```

当 `test-pod` 变为 `Running` 后，到 NFS 上看一下。

---

# 八、在 NFS 上验证数据确实写进来了

在 NFS 服务器上：

```bash
ls /data/k8s/default-test-pvc-*/ 
cat /data/k8s/default-test-pvc-*/SUCCESS
```

预期输出：

```text
hello from dynamic pvc
```

这说明：

> Pod `/mnt` 目录写入的内容，真实落到了 NFS 的目录里。
> 即：**Pod → PVC → PV → NFS** 全链路验证通过 ✅

---

# 九、验证“Pod 删除了，数据还在”（持久卷的核心意义）

1. 删除 Pod，但保留 PVC：

```bash
kubectl delete pod test-pod
kubectl get pod test-pod
# 显示 NotFound 或已删除
```

2. 再次用同一个 YAML 重建 Pod：

```bash
kubectl apply -f test-pod.yaml
kubectl get pod test-pod
```

3. 回到 NFS 再看：

```bash
ls /data/k8s/default-test-pvc-*/
cat /data/k8s/default-test-pvc-*/SUCCESS
```

你会发现：

* 目录还是同一个
* `SUCCESS` 文件仍然存在，内容还是 `hello from dynamic pvc`

> 这就印证了你说的那句话：
> **「Pod 虽然删掉了，但数据因为在 NFS 上，所以不会丢，只要以后有 Pod 再挂上这个 PVC，就能看到同一份数据。」**

这就是 **PersistentVolume（持久卷）** 的精髓：

* Pod 是一次性的、可重建的
* PVC/PV+NFS 上的数据是持久的、跨 Pod 生命周期存在的

---

# 十、最终原理总结

整个动态挂载实验，其实就一句话链路：

```text
Pod → PVC → StorageClass → Provisioner → PV → NFS 真实目录
```

* **Pod**：只关心 “我要一个卷，名字叫 test-pvc，挂到 /mnt”
* **PVC**：描述“我要什么样的存储”（容量、访问模式、storageClassName）
* **StorageClass**：指定“用哪种存储后端 / 哪个 Provisioner 帮我建卷”
* **Provisioner（nfs-subdir-external-provisioner）**：
  * 收到 PVC 请求
  * 在 NFS 上创建一个子目录
  * 创建一个对应的 PV，并绑定到 PVC
* **PV**：描述“这个 NFS 子目录是一个 1Mi / 1Gi 的卷，已经属于某个 PVC”
* **NFS 目录**：真实的数据位置，Pod 删了也不会动它

你在实验中亲手验证了两个关键点：

1. **动态创建 PV**：只写 PVC + StorageClass，PV 由 provisioner 自动创建
2. **数据持久性**：删 Pod 重建，数据仍然存在，因为它在 NFS 上
下面我把 **“ServiceAccount + RBAC 在动态存储中的作用”** 以 **正式实验文档补充章节** 的形式加入到你的《动态存储（StorageClass + NFS Provisioner）实验文档》中。

# 🔵 附录：ServiceAccount 与 RBAC 在 NFS 动态卷中的作用（实验文档补充）

在动态存储（StorageClass + 外部 Provisioner）实验中，你会注意到 Provisioner 的 Deployment 中包含一行：

```yaml
serviceAccountName: nfs-client-provisioner
```

这一行非常关键，它直接关系到 **PVC 能否自动创建 PV**。

本附录详细解释 ServiceAccount 与 RBAC 机制在本实验中的作用，帮助你彻底理解动态存储是如何运作的。

---

# 1. 为什么 Provisioner 必须使用 ServiceAccount？

Kubernetes 中，任何 Pod 若要访问 API Server，必须使用某个 **ServiceAccount（SA）** 来代表其身份，就像一个 Pod 的“身份证”。

而动态存储插件（Provisioner）必须访问 API Server 来执行以下操作：

| 操作                          | 是否必须权限？ |
| --------------------------- | ------- |
| Watch PVC                   | ✔       |
| Watch PV                    | ✔       |
| 自动创建 PV                     | ✔       |
| 更新 PVC 的状态（Pending → Bound） | ✔       |
| 创建事件 Event                  | ✔       |

如果没有 SA + 权限（RBAC），这些操作将全部失败，结果就是：

### ❌ PVC 永远处于 Pending

### ❌ Provisioner 日志反复报错

### ❌ PV 永远不会被自动创建

你之前遇到 PVC Pending 就是这个机制在工作。

---

# 2. 本实验中的 ServiceAccount 配置

创建 Provisioner 的时候，你使用了：

```yaml
serviceAccountName: nfs-client-provisioner
```

并配套创建了以下 RBAC 资源：

### ✔ ServiceAccount

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: nfs-client-provisioner
```

### ✔ ClusterRole（声明权限）

```yaml
rules:
  - apiGroups: [""]
    resources: ["persistentvolumes"]
    verbs: ["get", "list", "watch", "create", "delete"]

  - apiGroups: [""]
    resources: ["persistentvolumeclaims"]
    verbs: ["get", "list", "watch", "update"]

  - apiGroups: ["storage.k8s.io"]
    resources: ["storageclasses"]
    verbs: ["get", "list", "watch"]

  - apiGroups: [""]
    resources: ["events"]
    verbs: ["list", "watch", "create", "update", "patch"]
```

### ✔ ClusterRoleBinding（把权限授予 SA）

```yaml
subjects:
  - kind: ServiceAccount
    name: nfs-client-provisioner
    namespace: default
```

这一整套就是 **让 Provisioner 拥有创建 PV 的权限**。

---

# 3. 没有 ServiceAccount + RBAC 会怎样？

如果这行缺失：

```yaml
serviceAccountName: nfs-client-provisioner
```

Provisioner 会以 **default serviceAccount** 身份运行，而 default 的权限是极少的，不能创建 PV。

PVC 会一直这样：

```
Pending
```

Provisioner 的日志可能出现类似：

```
cannot create PV: forbidden
cannot update PVC: forbidden
```

PVC 永远不会绑定。

---

# 4. 动态卷的完整授权流程示意图

```
PVC（声明：我要存储）
      ↓
StorageClass（指定 provisioner）
      ↓
Provisioner Pod
      ↓（需要权限操作）
使用 ServiceAccount → ClusterRole → ClusterRoleBinding
      ↓
自动创建 PV
      ↓
绑定 PVC ←→ PV
```

你现在的实验是这个流程的完整实现。

---

# 5. 如何验证 Provisioner 正在使用正确的 ServiceAccount？

你可以运行：

```bash
kubectl describe pod -l app=nfs-client-provisioner | grep "Service Account"
```

输出应该是：

```
Service Account:  nfs-client-provisioner
```

这代表权限链路正确启用。

---

# 6. 本实验中 ServiceAccount 的用途总结（最重要的部分）

| 作用                 | 说明                       |
| ------------------ | ------------------------ |
| 访问 PVC             | 监听是否有新 PVC 申请            |
| 创建 PV              | 动态创建真正的 PV 对象            |
| 更新 PVC 状态          | 把 PVC 从 Pending 改为 Bound |
| 写事件                | 记录 Provisioning 的日志      |
| Watch StorageClass | 监听对应的 class              |

一句话总结：

> **ServiceAccount + RBAC 就是让 Provisioner 拥有“管理员权限”，从而能自动为 PVC 创建 PV 的关键。
> 缺了它，动态创建永远不会成功。**

---

如果你愿意，我可以为你把 **整个 NFS 动态卷实验文档（包含本附录）打包成一份清晰、美观、结构化的 Markdown 手册**，你以后可以随时翻阅、分享、打印。

需要我帮你整理成整体文档吗？
