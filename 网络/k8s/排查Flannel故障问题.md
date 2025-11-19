可以，这个案例其实非常适合在面试里讲 😊
 我帮你整理成一套**可以直接在面试里说的“排障故事”**，你背一下大概结构就行。

------

## 一、场景背景

- 自建的 Kubernetes 集群（kubeadm 部署，containerd 作为 CRI，CNI 使用 flannel，Pod 网段 10.244.0.0/16）。
- 在集群里部署了一套 **WordPress + MySQL**：
  - MySQL 使用 `ClusterIP` Service：`mysql`
  - WordPress 使用 `NodePort` Service 对外暴露，通过浏览器访问。
- 目标：通过 `http://<任意节点IP>:NodePort` 访问 WordPress 正常安装页面。

------

## 二、故障现象

1. WordPress Pod、MySQL Pod 均为 `Running` 状态：

   ```bash
   kubectl -n blog get pods -o wide
   ```

2. 通过浏览器访问 WordPress，页面报错：

   > Error establishing a database connection

3. 说明 **应用本身能起来，但无法连接数据库**。看起来像是 DB 配置错误或者网络问题。

------

## 三、第一轮排查：确认应用层配置没问题

1. **检查 WordPress Deployment 环境变量：**

   ```bash
   kubectl -n blog get deploy wordpress-deploy -o yaml | grep -A4 WORDPRESS_DB_HOST
   ```

   确认是：

   ```yaml
   WORDPRESS_DB_HOST=mysql:3306
   WORDPRESS_DB_USER=wordpress
   WORDPRESS_DB_PASSWORD=wordpress
   WORDPRESS_DB_NAME=wordpress
   ```

2. **确认 MySQL 已经正常初始化：**

   ```bash
   kubectl -n blog logs deploy/mysql-deploy
   ```

   日志显示：

   - 创建了数据库 `wordpress`
   - 创建了用户 `wordpress`
   - 授权正常
      → 数据库本身没有明显异常。

3. **检查 WordPress 容器内的配置文件：**

   ```bash
   kubectl -n blog exec -it <wordpress-pod> -- grep DB_HOST /var/www/html/wp-config.php
   ```

   发现使用的是：

   ```php
   define( 'DB_HOST', getenv_docker('WORDPRESS_DB_HOST', 'mysql') );
   ```

   说明 WordPress 是通过 `WORDPRESS_DB_HOST` 环境变量来解析数据库地址，逻辑没问题。

> 小结：应用层配置（环境变量、wp-config.php 模板、MySQL 初始化）都 **基本正确**，怀疑点转向 **网络/DNS 解析**。

------

## 四、第二轮排查：从容器内部验证到 DB 的连通性

1. 进入 WordPress 容器，尝试使用 `mysql` 客户端直连：

   ```bash
   kubectl -n blog exec -it <wordpress-pod> -- bash
   apt update && apt install -y mariadb-client-compat
   mysql -h mysql -u wordpress -pwordpress -D wordpress
   ```

2. 得到的报错是：

   ```text
   ERROR 2005 (HY000): Unknown server host 'mysql' (-3)
   ```

   ➜ 很关键的一点：**连“mysql 这个主机名”都解析不了**，还没到数据库权限那一步。

3. 联想：Pod 内 DNS 解析存在问题，于是从容器内测试 `nslookup`：

   ```bash
   nslookup mysql
   ```

   输出：

   ```text
   ;; communications error to 10.96.0.10#53: connection refused
   ;; no servers could be reached
   ```

   ➜ 说明 Pod 正在使用 `10.96.0.10` 作为 DNS（kube-dns 的 ClusterIP），
    但是这个地址 **53 端口拒绝连接**，即 DNS 服务本身不工作。

> 小结：不是 MySQL 出问题，而是 **集群 DNS 挂了**，导致 `mysql` 这个 Service 名根本解析不了。

------

## 五、第三轮排查：聚焦 Kubernetes DNS（CoreDNS）

1. **看 kube-dns Service 和 endpoints：**

   ```bash
   kubectl -n kube-system get svc kube-dns
   kubectl -n kube-system get endpoints kube-dns
   ```

   结果：

   ```text
   NAME       TYPE        CLUSTER-IP   PORT(S)
   kube-dns   ClusterIP   10.96.0.10   53/UDP,53/TCP,...
   
   NAME       ENDPOINTS
   kube-dns               # 为空
   ```

   ➜ Service 存在，但 **没有任何后端 endpoint**，即后端 DNS Pod 一个都没挂上来。

2. **查看 CoreDNS Pod 状态：**

   ```bash
   kubectl -n kube-system get pods -o wide | grep coredns
   ```

   发现：

   ```text
   coredns-xxx   0/1   CrashLoopBackOff
   coredns-yyy   0/1   CrashLoopBackOff
   ```

   ➜ CoreDNS 本身在不停崩溃，怪不得 kube-dns 没 endpoints。

3. **查看 CoreDNS 日志：**

   ```bash
   kubectl -n kube-system logs <其中一个coredns-pod>
   ```

   关键报错：

   ```text
   plugin/kubernetes: Kubernetes API connection failure:
   Get "https://10.96.0.1:443/version": dial tcp 10.96.0.1:443: connect: no route to host
   ```

   ➜ CoreDNS 无法访问 `10.96.0.1:443`，也就是 `kubernetes` Service 对应的 apiserver ClusterIP。
    这是一个非常典型的 **CNI / Pod 网络不通** 的信号。

> 小结：DNS 挂的根因不是 CoreDNS 配错，而是 **Pod 到 API Server 的网络完全不通**，
>  进而导致 CoreDNS 启动失败 → kube-dns 无 endpoints → 所有 Pod DNS 挂。

------

## 六、第四轮排查：锁定到 CNI（flannel）问题

1. 根据 Pod IP（10.244.x.x）判断 CNI 使用的是 flannel，然后查看 flannel 是否存在：

   ```bash
   kubectl -n kube-system get pods -o wide | grep -i flannel
   ```

   **没有任何输出** ➜ 集群中根本没有在运行的 flannel DaemonSet Pod。

2. 再看 CNI 组件（DaemonSet）：

   ```bash
   kubectl -n kube-system get daemonset
   ```

   发现没有 flannel 的 DaemonSet 或状态异常。
    ➜ 说明 **CNI 插件（flannel）已经不在正常工作**，导致整个 Pod 网络出现 “no route to host”。

3. 结合现象回顾：

   - 曾修改过 containerd 配置并重启
   - 集群运行多天，虚拟机网络环境可能不稳定
      ➜ 非常符合 **flannel 配置漂移 / Pod 网络断裂** 的典型场景。

> 小结：故障根因定位为：**flannel CNI 网络插件损坏或未正常运行 → Pod 网络不可达 → CoreDNS 无法连 API → DNS 全挂 → 上层服务（WordPress）无法通过 Service 名连 MySQL。**

------

## 七、解决方案：恢复 CNI（flannel）+ 重启 CoreDNS

1. **重新部署 flannel：**

   ```bash
   kubectl apply -f https://raw.githubusercontent.com/flannel-io/flannel/v0.25.0/Documentation/kube-flannel.yml
   ```

   确认 flannel DaemonSet 和 Pod 正常：

   ```bash
   kubectl -n kube-system get pods -o wide | grep -i flannel
   ```

   期望结果为 `1/1 Running`。

2. **删除 CoreDNS Pod，让它们用恢复后的网络重新启动：**

   ```bash
   kubectl -n kube-system delete pod -l k8s-app=kube-dns
   ```

   再看状态：

   ```bash
   kubectl -n kube-system get pods -o wide | grep coredns
   ```

   变为 `1/1 Running`。

3. **确认 kube-dns endpoints 恢复：**

   ```bash
   kubectl -n kube-system get endpoints kube-dns
   ```

   看到类似：

   ```text
   kube-dns   10.244.0.29:53,10.244.0.30:53
   ```

4. **在业务 Pod 中验证 DNS：**

   ```bash
   kubectl -n blog exec -it <wordpress-pod> -- bash
   nslookup mysql
   ```

   能解析为 `10.108.x.x` 这样的 ClusterIP。

5. **再次验证数据库连接：**

   ```bash
   mysql -h mysql -u wordpress -pwordpress -D wordpress
   ```

   成功进入 `mysql>`。

6. **最终在浏览器访问 NodePort：**
    WordPress 页面恢复正常，进入安装向导。

------

## 八、面试时可以强调的“思路亮点”

在面试里，你可以强调这几个关键点，体现你的排查能力：

1. **没有一上来就怀疑应用，而是从现象出发逐层拆解：**
   - 浏览器报 DB 错 → 容器里主动用 `mysql` 客户端验证连通性；
   - 错误是 `Unknown server host 'mysql'`，敏感到是 DNS 解析问题，而不是账号密码问题。
2. **善用 Kubernetes 自身对象来定位问题：**
   - 通过 `kubectl get svc/endpoints kube-dns` 发现 endpoints 为空；
   - 通过 `kubectl get pods -n kube-system` 发现 CoreDNS CrashLoopBackOff；
   - 通过 `coredns` 日志看到无法访问 10.96.0.1。
3. **能从 DNS 问题进一步追到 CNI 根因：**
   - 看到 “no route to host” 时，意识到是 Pod 网络（CNI）问题，而不是单纯的 DNS 配置错误；
   - 通过 `kubectl get pods -n kube-system | grep flannel` 确认 flannel 缺失；
   - 通过重新 apply flannel DaemonSet + 重启 CoreDNS 完整恢复网络和 DNS。
4. **最后总结出易踩坑点：**
   - 改 containerd、重启容器运行时可能影响 Pod 网络；
   - 在 Pod 中手动改 `/etc/resolv.conf` 是危险操作；
   - CNI（特别是 flannel）在多虚拟机环境中非常敏感，版本和配置需要控制。

------

# ✅ **面试口述版：K8s WordPress 无法连接 MySQL 的排障经历**

我在自建的 Kubernetes 集群里部署了一套 WordPress + MySQL（kubeadm + containerd + flannel，Pod 网段 10.244.0.0/16）。
 有一天发现业务访问异常：WordPress 页面报 **“Error establishing a database connection”**。

第一步，我先从应用层排查：

- 看 WordPress 的 Deployment 环境变量，`WORDPRESS_DB_HOST=mysql:3306`、用户和密码都正确；
- 查看 MySQL 的 Pod 日志，数据库和用户 `wordpress` 都已经按预期初始化成功。

应用配置没问题之后，我进入 WordPress 容器里，用 `mysql -h mysql -u wordpress` 直接连库，结果返回：

> ```
> ERROR 2005 (HY000): Unknown server host 'mysql'
> ```

这说明不是账号密码错误，而是 **连 “mysql” 这个 Service 名都解析不了**，怀疑是 DNS 问题。
 于是我在容器里 `nslookup mysql`，发现提示：

> `no servers could be reached`，且指向的 DNS 是 `10.96.0.10`（kube-dns）。

接着我查了 kube-system 里的 DNS 相关资源：

- `kube-dns` Service 存在，但 `endpoints` 是空的；
- CoreDNS 的 Pod 状态是 `CrashLoopBackOff`。

看 CoreDNS 日志，关键报错是：

```text
plugin/kubernetes: Kubernetes API connection failure:
Get "https://10.96.0.1:443/version": dial tcp 10.96.0.1:443: no route to host
```

这说明 **Pod 根本访问不到 API Server 的 ClusterIP（10.96.0.1）**，已经不是单纯 DNS 配置问题，而是 **CNI/Pod 网络出了问题**。

因为我的 Pod 网段是 10.244.x.x，所以我去看 flannel 的情况：

- kube-system 里能看到 flannel 的 DaemonSet 和 Pod；
- 但节点上 flannel 对应的虚拟网卡（比如 `flannel.1`、`cni0`）处于 down/不通状态，表现为 Pod 间和 Pod→Service 的流量都“no route to host”。

结合之前我动过 containerd 配置、重启过服务，以及虚拟机网络环境不稳定，基本可以确认：
 **flannel CNI 网络已经处于异常状态，导致 Pod 网络断 → CoreDNS 连不上 API → kube-dns 无 endpoints → 所有 Service 名解析失败 → 最终表现为 WordPress 连不上 MySQL。**

解决方案上，我选择：

1. 重新 apply flannel 的官方 YAML，恢复 CNI 配置和 DaemonSet；
2. 删除 coredns 的 Pod，让它们在网络恢复后重新拉起；
3. 再次检查：
   - flannel Pod `1/1 Running`，节点 CNI 网桥恢复；
   - `kube-dns` endpoints 出现 CoreDNS 的 IP:53；
   - 在业务 Pod 里 `nslookup mysql` 能解析到 MySQL 的 ClusterIP；
   - `mysql -h mysql -u wordpress -pwordpress` 可以正常进入数据库。

最后刷新浏览器，WordPress 正常进入安装界面，问题彻底解决。

整个排障过程，我是从：

> **应用报错 → 容器内直连验证 → DNS 解析 → CoreDNS 日志 → Pod 到 API 的网络 → flannel CNI 状态**

一层层往下走，最终把问题归因到 **CNI(flannel) 网络异常**，再通过 **重新部署 CNI + 重启 CoreDNS** 来恢复集群的 DNS 和业务访问。
