# 📘 **Kubernetes + NFS + PV/PVC + StatefulSet

——完整实验教程（附详细原理解析）**

非常适合作为：
✔ 学习文档
✔ 培训教材
✔ 团队内部知识库

---

# **📌 第一部分：实验目标**

通过本次实验，你将完全掌握：

1. **如何使用 NFS 作为 Kubernetes 持久化存储**
2. **如何创建静态 PV（PersistentVolume）并绑定 PVC**
3. **如何使用 StatefulSet 自动为每个 Pod 创建独立 PVC**
4. **如何让 Pod 具有独立的数据目录并实现重建不丢数据**
5. **彻底理解 StorageClass、PV、PVC 的“静态绑定”、“动态绑定”机制**

最终效果：

* 每个 StatefulSet Pod 各有一个独立的 NFS 目录
* 删除 Pod 不会丢失数据
* PVC 自动绑定到指定的 PV
* StatefulSet 自动管理 Pod 名字、顺序和卷挂载

---

# **📌 第二部分：实验环境**

| 角色            | 描述                                       |
| ------------- | ---------------------------------------- |
| NFS Server    | 一个可被 K8s 节点访问的 Linux 服务器，例如：10.151.30.57 |
| Kubernetes 集群 | kubeadm 搭建的 1 Master + N Worker          |
| 存储目录          | `/data/k8s`                              |

---

# **📌 第三部分：准备 NFS 服务端（Ubuntu）**

> 这一部分在 NFS Server 上执行
> 示例 IP：10.151.30.57

### 1. 安装 NFS Server

```bash
sudo apt update
sudo apt install -y nfs-kernel-server
```

### 2. 创建共享目录

```bash
sudo mkdir -p /data/k8s
sudo chmod 755 /data/k8s
```

### 3. 配置 NFS 导出

编辑：

```bash
sudo nano /etc/exports
```

添加：

```
/data/k8s  *(rw,sync,no_root_squash)
```

### 4. 重启 NFS 服务

```bash
sudo systemctl restart nfs-kernel-server
sudo exportfs -a
```

### 5. 检查是否导出

```bash
showmount -e
```

预期：

```
/data/k8s *
```

---

# **📌 第四部分：Node 节点测试 NFS（可选但强烈推荐）**

在任意 K8s 节点执行：

```bash
sudo apt install -y nfs-common
sudo mkdir -p /mnt/nfs-test
sudo mount -t nfs 10.151.30.57:/data/k8s /mnt/nfs-test
```

写测试文件：

```bash
echo "hello nfs" | sudo tee /mnt/nfs-test/test.txt
```

到 NFS Server 验证：

```bash
ls /data/k8s
```

测试完成后卸载：

```bash
sudo umount /mnt/nfs-test
```

> **这一挂载测试只是为了验证网络和权限，Kubernetes 本身不需要你提前挂载。**

---

# **📌 第五部分：为 StatefulSet 准备各自独立的目录**

进入 NFS Server：

```bash
sudo mkdir -p /data/k8s/web2-0
sudo mkdir -p /data/k8s/web2-1
sudo chmod 777 /data/k8s/web2-0 /data/k8s/web2-1
```

---

# **📌 第六部分：创建静态 PV（指向 NFS 子目录）**

### 1. pv-web2-0.yaml

```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: pv-web2-0
spec:
  capacity:
    storage: 1Gi
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  nfs:
    server: 10.151.30.57
    path: /data/k8s/web2-0
```

### 2. pv-web2-1.yaml

```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: pv-web2-1
spec:
  capacity:
    storage: 1Gi
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  nfs:
    server: 10.151.30.57
    path: /data/k8s/web2-1
```

创建：

```bash
kubectl apply -f pv-web2-0.yaml
kubectl apply -f pv-web2-1.yaml
kubectl get pv
```

预期：

```
pv-web2-0   Available
pv-web2-1   Available
```

---

# **📌 第七部分：创建 Headless Service（用于 StatefulSet DNS）**

service-web2.yaml：

```yaml
apiVersion: v1
kind: Service
metadata:
  name: web2-headless
spec:
  clusterIP: None
  selector:
    app: web2
  ports:
  - port: 80
    name: http
```

创建：

```bash
kubectl apply -f service-web2.yaml
```

---

# **📌 第八部分：创建 StatefulSet（自动生成 PVC）**

statefulset-web2.yaml：

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: web2
spec:
  serviceName: web2-headless
  replicas: 2
  selector:
    matchLabels:
      app: web2
  template:
    metadata:
      labels:
        app: web2
    spec:
      containers:
      - name: web2
        image: nginx:latest
        ports:
        - containerPort: 80
          name: http
        volumeMounts:
        - name: www
          mountPath: /usr/share/nginx/html
  volumeClaimTemplates:
  - metadata:
      name: www
    spec:
      accessModes: [ "ReadWriteOnce" ]
      resources:
        requests:
          storage: 1Gi
```

创建：

```bash
kubectl apply -f statefulset-web2.yaml
```

查看资源：

```bash
kubectl get pods -l app=web2
kubectl get pvc
kubectl get pv
```

你会看到：

* web2-0 → www-web2-0 → pv-web2-0
* web2-1 → www-web2-1 → pv-web2-1

这是 StatefulSet 自动 PVC 绑定静态 PV 的核心。

---

# **📌 第九部分：验证每个 Pod 有独立的数据**

### 1. 在 Pod 内写文件

```bash
kubectl exec -it web2-0 -- sh -c 'echo "I am pod web2-0" > /usr/share/nginx/html/index.html'
kubectl exec -it web2-1 -- sh -c 'echo "I am pod web2-1" > /usr/share/nginx/html/index.html'
```

### 2. 到 NFS Server 验证

```bash
cat /data/k8s/web2-0/index.html
cat /data/k8s/web2-1/index.html
```

预期：

```
I am pod web2-0
I am pod web2-1
```

说明每个 Pod 有自己的独立存储。

---

# **📌 第十部分：验证 Pod 重建后数据不丢失**

```bash
kubectl delete pod web2-0
kubectl get pods -l app=web2
```

当新的 web2-0 变为 Running 后：

```bash
kubectl exec -it web2-0 -- cat /usr/share/nginx/html/index.html
```

输出仍然：

```
I am pod web2-0
```

📌 **这证明：StatefulSet + PV/PVC 成功实现有状态应用的持久化存储。**

---

# 📘 第十一部分：核心原理解析

下面是整个实验背后的核心逻辑。

---

## 🟦 **1. 为什么 StatefulSet 需要 Headless Service？**

* 用来提供稳定的 DNS
* Pod 名字可解析为：

```
web2-0.web2-headless.default.svc.cluster.local
```

* StatefulSet 需要它来保证 Pod 的网络身份不变

---

## 🟩 **2. volumeClaimTemplates 的作用是什么？**

StatefulSet 会：

* 为每个 Pod 自动创建一个 PVC
* 命名格式：`<模板名称>-<StatefulSet 名>-<序号>`
* 例子：

```
www-web2-0
www-web2-1
```

每个 Pod 独享一个 PVC，天然实现 Pod 间独立数据。

---

## 🟧 **3. 为什么 PVC 能自动绑定到手动创建的 PV？**

因为满足静态绑定的 3 条规则：

| 项目               | PV  | PVC |
| ---------------- | --- | --- |
| storageClassName | ""  | ""  |
| accessMode       | RWO | RWO |
| 容量               | 1Gi | 1Gi |

所以可以自动绑定。

---

## 🟥 **4. 静态绑定 VS 动态绑定区别**

| 绑定方式 | 需要 storageClassName？ | PV 来源                | 场景                |
| ---- | -------------------- | -------------------- | ----------------- |
| 静态绑定 | 不需要（必须匹配一致）          | 手动创建 PV              | NFS、自建存储          |
| 动态绑定 | 必须写                  | StorageClass 自动创建 PV | 云厂商（AWS/GCP/Ceph） |

你的实验属于静态绑定。

---

## 🟨 **5. 为什么 Pod 删除数据不丢失？**

因为：

* StatefulSet 重新创建 Pod 时
* 使用的依然是原来的 PVC
* PVC 绑定同一个 PV
* 数据仍然在 NFS 上

---

# 📘 第十二部分：实验总结（非常重要）

你已经通过本实验掌握：

### ✔ Kubernetes 如何使用 NFS 做持久化存储

### ✔ 静态 PV / PVC 匹配机制

### ✔ StatefulSet 如何自动管理卷

### ✔ 每个 Pod 独立存储目录的实现方式

### ✔ Pod 重建后数据不丢失的根本原因

### ✔ Headless Service 在 StatefulSet 中的作用








# 🌟 **为什么 StatefulSet 一定需要 Headless Service？**

因为 StatefulSet 的核心功能就是：

> **给每个 Pod 分配一个稳定、不会变化的网络身份（DNS 名字）。**

而普通 Service **做不到这一点**。

因此 StatefulSet 必须配合 **Headless Service（clusterIP=None）** 才能实现。

---

# 🔥 一句话总结（非常重要）

> **StatefulSet 之所以需要 Headless Service，是因为 Headless Service 能提供“每个 Pod 独立的 DNS 名字”，
> 而普通 Service 只能提供负载均衡的统一入口。**

---

# 📌 用例子讲：web2 StatefulSet 的 DNS 是怎么来的？

你创建的 Headless Service：

```yaml
apiVersion: v1
kind: Service
metadata:
  name: web2-headless
spec:
  clusterIP: None
  selector:
    app: web2
```

StatefulSet：

```yaml
spec:
  serviceName: web2-headless
```

这两个一起，就让 Kubernetes 给每个 Pod 分配这些 DNS：

| Pod 名字 | DNS 域名                                         |
| ------ | ---------------------------------------------- |
| web2-0 | web2-0.web2-headless.default.svc.cluster.local |
| web2-1 | web2-1.web2-headless.default.svc.cluster.local |

注意：

* **每个 Pod 是单独的 DNS 记录（A 记录）**
* DNS 名字包含固定序号（0、1、2…）
* Pod 重建之后 DNS 名字不会变

📌 **这就是 StatefulSet 能“保持身份稳定”的秘密。**

---

# 📌 那普通 Service 能做到吗？ —— 不能

普通 Service 做的事情是：

* 给一组 Pod 提供一个统一的 IP/名称
* 负载均衡到所有 Pod
* 不保证固定的 Pod 顺序或独立地址

举例：

```
nginx.default.svc.cluster.local
```

这个名字解析到的是：

```
10.244.1.10
10.244.1.11
```

只是多个 IP 的集合，但不会告诉你：

* 哪个是 web2-0
* 哪个是 web2-1
* 也不会保证顺序
* 也不会固定绑定

❌ **不能用于数据库**
❌ **不能用于每个节点有独立存储**
❌ **不能用于需要固定身份的服务**

---

# 📌 为什么 Headless Service 可以提供每个 Pod 独立 DNS？

当 `clusterIP=None` 时：

* Service 不提供 VIP（虚拟 IP）
* DNS 系统直接暴露 **每个 Pod 的 IP** 和 **主机名**

像这样：

```
web2-0.web2-headless.default.svc.cluster.local → 10.244.1.66
web2-1.web2-headless.default.svc.cluster.local → 10.244.1.67
```

📌 **关键点：这两个名字是绑定到各自的 Pod 的，而不是负载均衡。**

这就是 StatefulSet 的神奇之处。

---

# 📌 回答你的第二个问题：

> **“都是这个服务吗？”**

是的，StatefulSet 只认你在 `serviceName:` 写的这个 Service：

```yaml
spec:
  serviceName: web2-headless
```

它必须是：

* 一个 **Headless Service**
* 选择器（selector）和 StatefulSet 的 Pod label 一致

否则 StatefulSet 的 DNS 系统不能工作。

---

# 📘 一张图让你更直观理解

```
          +----------------------+
          | Headless Service     |
          |  web2-headless       |
          | clusterIP=None       |
          +----------+-----------+
                     |
         Kubernetes DNS 提供独立条目
                     |
   +-----------------+------------------+
   |                                    |
web2-0                                 web2-1
(10.244.1.66)                          (10.244.1.67)
DNS: web2-0.web2-headless...     DNS: web2-1.web2-headless...
```

📌 普通 Service 只有：

```
nginx.default.svc → 一个统一名字
```

📌 StatefulSet + Headless Service 有：

```
web2-0.web2-headless → 固定 Pod0  
web2-1.web2-headless → 固定 Pod1
```

这就是本质区别。

---

# 🔥 最终总结（务必记住）

StatefulSet 需要 Headless Service，因为：

### ✔ 每个 Pod 需要唯一、独立、固定 DNS

### ✔ Pod 重启后 DNS 名字不能改变

### ✔ DNS 名字用于挂载独立存储（PV/PVC）

### ✔ DNS 名字用于集群内部通信（如数据库主从）

### ✔ 普通 Service 无法提供这些能力

