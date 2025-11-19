# Kubernetes + NFS 持久化存储全流程实验手册

---

## 0. 实验总览与学习路径

整个实验分成 4 个阶段，每一阶段都是在上一阶段基础上升级：

1. **阶段一：静态 NFS + PV + PVC + Pod**
   先搞清楚 NFS 是什么、PV/PVC 是什么，它们怎么绑在一起。
2. **阶段二：部署 NFS 动态 Provisioner（nfs-subdir-external-provisioner）**
   让 Kubernetes 能“自动为 PVC 创建 PV”。
3. **阶段三：StorageClass + PVC 动态创建 PV + Pod 验证“删 Pod 数据还在”**
   真正体会“持久卷”的意义。
4. **阶段四：StatefulSet + volumeClaimTemplates + StorageClass**
   每个 Pod 自动拥有自己的独立数据目录，接近生产级使用方式。

另外还会有一个附录专门讲：

* **ServiceAccount + RBAC 为什么是动态卷的灵魂？**

---

## 1. 实验环境与前置条件

### 1.1 环境假设

* 操作系统：Ubuntu（K8s 节点 + NFS Server）
* Kubernetes：已安装好，`kubectl` 可以正常使用
* K8s 集群中至少有：

  * 1 个 master
  * 1 个 worker（也可以更多）

### 1.2 NFS 服务器信息（你已经搭好了）

* NFS Server IP：`192.168.241.144`
* 共享目录：`/data/k8s`
* 所有 K8s 节点能访问 `192.168.241.144:2049`

> 如果你以后在别的环境做一模一样的实验，只需要把 IP 和路径换掉即可。

---

## 2. 阶段一：静态 NFS + PV + PVC + Pod（基础理解）

### 2.1 在 NFS 服务器上准备目录（已完成，但写在文档里方便复现）

```bash
sudo mkdir -p /data/k8s
sudo chmod 755 /data/k8s
```

`/etc/exports` 中有类似配置：

```text
/data/k8s  *(rw,sync,no_root_squash)
```

然后导出：

```bash
sudo exportfs -a
sudo systemctl restart nfs-kernel-server
```

### 2.2 在其中一个 K8s 节点上测试 NFS（推荐做一次）

```bash
sudo apt update
sudo apt install -y nfs-common

sudo mkdir -p /mnt/nfs-test
sudo mount -t nfs 192.168.241.144:/data/k8s /mnt/nfs-test

echo "hello nfs" | sudo tee /mnt/nfs-test/test.txt

ls -l /mnt/nfs-test

sudo umount /mnt/nfs-test
```

在 NFS 服务器上验证：

```bash
ls -l /data/k8s
cat /data/k8s/test.txt
```

看到 `hello nfs` 说明网络 + NFS 都 OK。

---

### 2.3 创建静态 PV（pv1）

**文件：`pv1.yaml`**

```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: pv1
spec:
  capacity:
    storage: 1Gi               # PV 大小
  accessModes:
  - ReadWriteOnce              # 单节点读写
  persistentVolumeReclaimPolicy: Retain
  nfs:
    server: 192.168.241.144    # NFS 服务 IP
    path: /data/k8s            # NFS 共享目录
```

应用：

```bash
kubectl apply -f pv1.yaml
kubectl get pv
```

你会看到：

```text
NAME   CAPACITY   ACCESS MODES   RECLAIM POLICY   STATUS      CLAIM   STORAGECLASS   AGE
pv1    1Gi        RWO            Retain           Available           ...
```

---

### 2.4 创建 PVC（pvc1），手动绑定静态 PV

**文件：`pvc1.yaml`**

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: pvc1
spec:
  accessModes:
  - ReadWriteOnce
  resources:
    requests:
      storage: 1Gi
```

应用：

```bash
kubectl apply -f pvc1.yaml
kubectl get pvc
kubectl get pv
```

预期：

* `pvc1` 的状态：`Bound`
* `pv1` 的状态：`Bound`，`CLAIM = default/pvc1`

---

### 2.5 创建 Pod 使用 PVC（验证读写）

**文件：`nfs-pod.yaml`**

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: nfs-pod
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: nfs-vol
      mountPath: /usr/share/nginx/html   # nginx 默认主页目录
  volumes:
  - name: nfs-vol
    persistentVolumeClaim:
      claimName: pvc1
```

应用 & 验证：

```bash
kubectl apply -f nfs-pod.yaml
kubectl get pod nfs-pod
```

进入容器：

```bash
kubectl exec -it nfs-pod -- sh
echo "hello from static pvc" > /usr/share/nginx/html/index.html
exit
```

在 NFS 上查看：

```bash
ls -l /data/k8s
cat /data/k8s/index.html
```

> ✅ 到这里，你已经用“全手动”的方式完成了：
> Pod → PVC（pvc1）→ PV（pv1）→ NFS（/data/k8s）

---

## 3. 阶段二：部署 NFS 动态 Provisioner（nfs-subdir-external-provisioner）

目标：不再手动创建 PV，让系统根据 PVC 自动创建 PV + NFS 子目录。

### 3.1 创建 ServiceAccount + RBAC

**文件：`nfs-client-sa.yaml`**

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

### 3.2 部署 nfs-subdir-external-provisioner

**文件：`nfs-client.yaml`**

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
      serviceAccountName: nfs-client-provisioner   # ⭐ 使用上面创建的 SA
      containers:
      - name: nfs-client-provisioner
        image: registry.cn-hangzhou.aliyuncs.com/lfy_k8s_images/nfs-subdir-external-provisioner:v4.0.2
        env:
        - name: PROVISIONER_NAME
          value: k8s-sigs.io/nfs-subdir-external-provisioner   # ⭐ provisioner 的名字
        - name: NFS_SERVER
          value: 192.168.241.144                               # ⭐ NFS 服务 IP
        - name: NFS_PATH
          value: /data/k8s                                     # ⭐ NFS 共享目录
        volumeMounts:
        - name: nfs-client-root
          mountPath: /persistentvolumes
      volumes:
      - name: nfs-client-root
        nfs:
          server: 192.168.241.144
          path: /data/k8s
```

应用 & 检查：

```bash
kubectl apply -f nfs-client.yaml
kubectl get pods -l app=nfs-client-provisioner
```

状态应为 `Running`。

如需看日志：

```bash
kubectl logs deployment/nfs-client-provisioner
```

---

## 4. 阶段三：StorageClass + PVC 动态创建 PV + 验证数据持久

### 4.1 创建 StorageClass

**文件：`nfs-client-class.yaml`**

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

你会看到：

```text
NAME                 PROVISIONER                                   AGE
course-nfs-storage   k8s-sigs.io/nfs-subdir-external-provisioner   ...
```

---

### 4.2 创建 PVC（test-pvc）触发动态创建 PV

**文件：`test-pvc.yaml`**

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: test-pvc
spec:
  storageClassName: course-nfs-storage   # ⭐ 使用上面的 StorageClass
  accessModes:
  - ReadWriteMany                        # NFS 支持多点读写
  resources:
    requests:
      storage: 1Mi                       # 1Mi 足够做实验
```

应用 & 查看：

```bash
kubectl apply -f test-pvc.yaml
kubectl get pvc
kubectl get pv
```

预期：

```text
NAME       STATUS   VOLUME                                     CAPACITY   ACCESS MODES   STORAGECLASS         AGE
test-pvc   Bound    pvc-xxxxxx...                              1Mi        RWX            course-nfs-storage   ...
```

同时 PV 中多出一个动态创建的 PV。

---

### 4.3 在 NFS 上查看自动创建的目录

在 NFS 服务器上：

```bash
ls /data/k8s
```

你会看到类似：

```text
default-test-pvc-pvc-02937a4d-a1ee-454e-ad56-a749572fb92b
```

这就是 provisioner 帮你为 `test-pvc` 创建的真实存储目录。

---

### 4.4 创建 Pod 挂载 test-pvc 写入数据

**文件：`test-pod.yaml`**

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
      claimName: test-pvc
```

应用：

```bash
kubectl apply -f test-pod.yaml
kubectl get pod test-pod
```

等状态为 `Running` 后，在 NFS 上看：

```bash
ls /data/k8s/default-test-pvc-*/
cat /data/k8s/default-test-pvc-*/SUCCESS
```

如果输出：

```text
hello from dynamic pvc
```

说明：

> Pod → PVC（test-pvc）→ PV（自动）→ NFS 真实目录 ✅

---

### 4.5 验证“删 Pod 数据不丢”

1. 删除 Pod：

```bash
kubectl delete pod test-pod
```

2. 再创建一遍：

```bash
kubectl apply -f test-pod.yaml
kubectl get pod test-pod
```

3. 去 NFS 上再次查看：

```bash
cat /data/k8s/default-test-pvc-*/SUCCESS
```

内容依然存在 —— 这就是：

> **Pod 是一次性的，数据是持久的。**

---

## 5. 阶段四：StatefulSet + volumeClaimTemplates + StorageClass（生产级玩法）

目标：让 StatefulSet 的每个 Pod 自动拥有独立数据卷与独立 NFS 目录。

---

### 5.1 编写 StatefulSet YAML

**文件：`test-statefulset-nfs.yaml`**

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: nfs-web
spec:
  serviceName: "nfs-web-headless"          # 需要一个 headless Service（可另写）
  replicas: 3
  selector:
    matchLabels:
      app: nfs-web
  template:
    metadata:
      labels:
        app: nfs-web
    spec:
      terminationGracePeriodSeconds: 10
      containers:
      - name: nginx
        image: nginx:1.7.9
        ports:
        - containerPort: 80
          name: web
        volumeMounts:
        - name: www
          mountPath: /usr/share/nginx/html
  volumeClaimTemplates:
  - metadata:
      name: www
    spec:
      storageClassName: course-nfs-storage   # ⭐ 使用我们的 StorageClass
      accessModes: [ "ReadWriteOnce" ]
      resources:
        requests:
          storage: 1Gi
```

> 提示：`serviceName: nfs-web-headless` 对应的 Service 你可以单独建一个 headless Service（`clusterIP: None`）。

---

### 5.2 创建 StatefulSet

```bash
kubectl apply -f test-statefulset-nfs.yaml
kubectl get pods -l app=nfs-web
```

你会看到类似：

```text
nfs-web-0   Running
nfs-web-1   Running
nfs-web-2   Running
```

---

### 5.3 查看 PVC 自动创建情况

```bash
kubectl get pvc
```

你会看到：

```text
www-nfs-web-0   Bound   pvc-xxx   1Gi   RWO   course-nfs-storage
www-nfs-web-1   Bound   pvc-yyy   1Gi   RWO   course-nfs-storage
www-nfs-web-2   Bound   pvc-zzz   1Gi   RWO   course-nfs-storage
```

> 每个 Pod 都有一个独立的 PVC，名字由 `volumeClaimTemplates.name + Pod 名` 组合而来。

---

### 5.4 查看 PV 和 NFS 目录

```bash
kubectl get pv
```

可以看到 3 个 PV 都给 `default/www-nfs-web-*` 绑定了。

然后到 NFS 上：

```bash
ls /data/k8s
```

可以看到类似：

```text
default-www-nfs-web-0-pvc-xxx
default-www-nfs-web-1-pvc-yyy
default-www-nfs-web-2-pvc-zzz
```

每个 Pod 对应一个独立目录。

---

### 5.5 验证 nfs-web-0 的数据持久性

1. 进入 `nfs-web-0` 写数据：

```bash
kubectl exec -it nfs-web-0 -- bash
echo "hello from web-0" > /usr/share/nginx/html/index.html
exit
```

2. 删除这个 Pod：

```bash
kubectl delete pod nfs-web-0
```

3. 等 StatefulSet 自动重建，重新进入：

```bash
kubectl exec -it nfs-web-0 -- bash
cat /usr/share/nginx/html/index.html
```

内容仍然是：

```text
hello from web-0
```

> 说明：
> StatefulSet 中 **Pod 虽然可以被删除重建，但它的卷是“跟着编号走”的，数据一直保持。**

---

## 6. 附录：ServiceAccount + RBAC 在动态卷中的作用（总结版）

* Provisioner 需要：

  * watch PVC
  * create PV
  * update PVC status
  * 写 events
* 所以需要 ServiceAccount + ClusterRole + ClusterRoleBinding
* 在 Deployment 中通过：

```yaml
serviceAccountName: nfs-client-provisioner
```

把 Pod 和这个账户绑定。

**若没配置或没权限 ⇒ PVC 永远 Pending，PV 无法自动创建。**

---

到这里，这份实验文档已经把你这一路踩坑 + 实操过的所有东西都串成了一条完整的学习路线。

如果你之后想要：

* 单独再出一篇《故障排查指南》（例如 Pending、ImagePullBackOff、自定义 StorageClass 问题）
* 或者画一张总流程图（Pod ↔ PVC ↔ StorageClass ↔ Provisioner ↔ PV ↔ NFS）

我也可以帮你整理出来，做成第二份“进阶手册”。
