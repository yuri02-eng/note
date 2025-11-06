# Linux 命名空间完整技术指南

## 0) 一致性模型（所有命名空间共享的骨架）

* **initial namespace**：内核启动时为每种 ns 创建一个“**初始实例**”，后续创建的实例与其**并列存在**。
* **进程如何隶属命名空间**

  * `task_struct->nsproxy`：挂 **mount/uts/ipc/pid_for_children/net/cgroup/time** 等命名空间指针。
  * `task_struct->cred->user_ns`：单独存放 **user namespace**（能力与 ID 映射的根）。
* **命名空间对象**：每类对象（如 `struct mnt_namespace`, `struct net`, `struct pid_namespace`, …）内嵌统一头 `struct ns_common`（**引用计数**、**nsfs inode**、**ops**）。
* **用户态可见**：`/proc/<pid>/ns/*` 由 **nsfs** 暴露；不同实例以 **inode 号**区分。`open()` 得到的 fd 可用于 `setns()`。
* **三兄弟系统调用（统一视角）**

  * `clone()/clone3()`：子进程按 `CLONE_NEW*` **出生在新命名空间**。
  * `unshare()`：**当前线程**脱钩并切到**新命名空间**（线程级语义）。
  * `setns(fd, type)`：**当前线程**加入**已有命名空间**（能力检查基于目标对象的 **owner userns**）。
* **权限模型**：所有加入/管理操作最终走 `ns_capable(target_owner_userns, CAP_*)`，因此 **userns 是其它命名空间的授权根**。
* **生命周期**：对象带引用计数；被线程隶属、被 `open("/proc/.../ns/*")` 的 fd、被 bind-mount 等引用即保活。

---

## 1) PID 命名空间（`CLONE_NEWPID`）— 进程与 `/proc` 视图

**隔离内容**：PID 编号、进程树、`/proc` 可见性；支持**嵌套**（层级）。

**关键结构（示意）**：

```c
struct pid_namespace {            // kernel/pid_namespace.c
    struct ns_common ns;          // 引用/ops/inum
    struct pid_namespace *parent; // 父层
    unsigned int level;           // 层级深度
    struct task_struct *child_reaper; // 本 ns 的 PID 1
    /* 号段/idr/位图分配结构见 kernel/pid.c */
};

struct pid {                       // kernel/pid.c
    struct upid numbers[PIDNS_MAX]; // 同一任务在多级 ns 的 <ns,nr> 列表
};
```

**创建/切换调用链**：

* `unshare(CLONE_NEWPID)` / `clone3(CLONE_NEWPID)`
  → `copy_namespaces()` / `create_new_namespaces()`
  → `create_pid_namespace()`（新 pidns 诞生）。
* **必须 `fork` 才获得新 PID**：`_do_fork()` → `copy_process()` → `alloc_pid()` 在目标 pidns 分配新号；父线程仍在旧 pidns。

**`/proc` 渲染**：`fs/proc/*` 按“**查看者所在 pidns**”将真实 pid 转为该视图下的 pid（如 `pid_nr_ns()`）。

**信号/ptrace 边界（纠正常见误解）**：

* 不是“只能同 ns 发信号”。规则是**按层级与权限**：祖先 pidns 可见/可向后代发信号（权限满足）；后代对祖先不可见；同级不同 pidns 互不可见。

**常见坑**：容器 PID 1 需做 **reaper** 与信号处理；跨 ns 调试需用 `nsenter --pid` 切视图。

---

## 2) Mount 命名空间（`CLONE_NEWNS`）— 挂载视图与传播域

**隔离内容**：挂载点表、**传播关系**（shared/private/slave/unbindable）。

**关键结构/算法（示意）**：

```c
struct mnt_namespace {            // fs/namespace.c
    struct ns_common ns;          // 通用头
    struct mount *root;           // 挂载视图根
};

struct mount {                    // fs/namespace.c
    struct mount *mnt_parent;
    struct list_head mnt_share;       // 共享组
    struct mount *mnt_master;         // 主从关系
    struct list_head mnt_slave_list;  // 从属列表
    /* 以及挂载点、超级块、dentry 等关联 */
};
```

**核心路径**：`do_mount()` / `do_move_mount()` / `pivot_root()`（`fs/namespace.c`）。

**传播实现**：`propagate_mount()` / `propagate_umount()` 基于**共享子树/peer group** 复制/拆除事件。

**创建语义**：`create_mnt_ns()` **复制当前挂载视图结构**（非磁盘数据），再把 `nsproxy->mnt_ns` 指过去。

**idmapped mount**：`struct mnt_idmap`（`fs/mount_idmapped.c`）叠加 UID/GID 映射，与 **userns** 协同支持 **rootless** 文件属主视图。

**实践要点**：新 mntns 常先 `mount --make-rprivate /` 防传播“漏风”；随后挂 `proc/sysfs`、`pivot_root` 等。

---

## 3) Network 命名空间（`CLONE_NEWNET`）— 完整网络栈实例

**隔离内容**：设备、地址、端口、路由、nft/iptables、conntrack、per-net sysctl 等。

**关键结构与机制**：

```c
struct net {                       // net/core/net_namespace.c
    struct ns_common ns;
    struct list_head list;         // 全局链
    /* pernet 子系统私有数据挂这里（通过注册） */
};
```

* **pernet_operations**：各协议/子系统注册 `init()`/`exit()` 钩子：

  * 新建 netns：`setup_net()` 依次调用所有已注册 `init()`，初始化 loopback、路由表、sysctl 等；
  * 销毁 netns：`cleanup_net()` **逆序** 调用 `exit()`，按依赖释放。
* **对象归属**：

  * 套接字 `struct sock` 含 `sk->sk_net`；
  * 设备 `struct net_device` 用 `dev_net(dev)` 取所属；`ip link set X netns N` 迁移设备会切换其归属。
* **持久化**：`ip netns add` 通过 **nsfs fd bind-mount** 到 `/var/run/netns/<name>` 保活。

**实践要点**：新 netns 初始仅 `lo`，需手工建 `veth`、接入 bridge/macvlan、配路由/NAT、nftables/iptables 和 per-net sysctl。

---

## 4) UTS 命名空间（`CLONE_NEWUTS`）— 主机名/域名视图

**隔离内容**：`uname(2)` 的 `nodename/domainname`。

**关键结构/路径**：

```c
struct uts_namespace {             // kernel/utsname.c
    struct ns_common ns;
    struct new_utsname name;       // sysname, nodename, release, version, machine, domainname
};
```

* `sethostname()`/`setdomainname()` → `ns_capable(uts->user_ns, CAP_SYS_ADMIN)` → 修改当前 `nsproxy->uts_ns` 指向对象。

---

## 5) IPC 命名空间（`CLONE_NEWIPC`）— SysV/POSIX IPC 空间

**隔离内容**：SysV IPC（消息队列/信号量/共享内存）与 POSIX mqueue（`/dev/mqueue`）。

**关键结构/路径**：

```c
struct ipc_namespace {             // ipc/namespace.c
    struct ns_common ns;
    struct ipc_ids ids[3];         // msg/sem/shm 三套 id 空间（idr/rbtree）
    /* 资源上限：msgmni/shmmni 等 */
};
```

* SysV IPC 在 `ipc/msg.c`, `ipc/sem.c`, `ipc/shm.c`；权限由 `ipcperms()` 等检查；
* POSIX mqueue 在 `fs/mqueue/`，每个 ipcns 可挂载一套 mqueue 实例。

---

## 6) 用户命名空间（`CLONE_NEWUSER`）— 能力与 ID 映射的根

**隔离内容**：UID/GID 视图与 capabilities；实现“容器内 root ≠ 宿主 root”。

**关键结构/路径**：

```c
struct user_namespace {            // kernel/user_namespace.c
    struct ns_common ns;
    struct user_namespace *parent;
    unsigned int level;
    struct uid_gid_map uid_map, gid_map; // 多段映射
    /* cap 边界/关键掩码等 */
};

struct cred {                      // kernel/cred.c
    struct user_namespace *user_ns; // 以及 euid/egid/cap 集合
};
```

* **ID 映射**：用户态写 `/proc/<pid>/{uid_map,gid_map}`（写 `gid_map` 前需 `echo deny > setgroups`）；
* **能力检查统一收口**：`ns_capable(target_owner_userns, CAP_*)`；
* **与其它 ns 的关系**：每个命名空间对象都带“**owner userns**”。能否创建/加入/管理它们，取决于你在该 owner userns 的能力。

---

## 7) Cgroup 命名空间（`CLONE_NEWCGROUP`）— **仅视图根相对化**

**隔离内容**：把 `/proc/self/cgroup`、`/sys/fs/cgroup` 的路径**相对化**到某个子树，便于容器内 systemd/runc 再分层。

**重要更正**：**Cgroup 命名空间不提供资源隔离**；资源限额/权重来自 **cgroup 控制器**（尤其 v2 unified）。

**关键结构/路径（示意）**：

```c
struct cgroup_namespace {          // kernel/cgroup/cgroup.c
    struct ns_common ns;
    struct cgroup *root_cgrp;      // 视图根（相对化起点）
};
```

---

## 8) 时间命名空间（`CLONE_NEWTIME`）— 单调类时钟偏移

**隔离内容**：为 `CLOCK_MONOTONIC` 与 `CLOCK_BOOTTIME` **叠加偏移**；**不影响 `CLOCK_REALTIME`（墙钟）**。

**关键结构/路径（示意）**：

```c
struct time_namespace {            // kernel/time/time_namespace.c
    struct ns_common ns;
    struct timens_offsets {
        ktime_t mono;              // MONOTONIC 偏移
        ktime_t boot;              // BOOTTIME 偏移
    } offsets;
};
```

* 读单调类时钟：`ktime_get_*()` 得到全局基值 → 若 `current->nsproxy->time_ns != init`，叠加偏移；
* 接口：`/proc/<pid>/timens_offsets` 读写偏移；vDSO 快路径也会读取这些偏移。

---

## 9) nsfs 与 `/proc/<pid>/ns`（通用基础设施）

* **统一头**：`struct ns_common { u64 inum; const struct ns_ops *ops; atomic_long_t count; }`。
* **文件系统**：`fs/nsfs.c` 把命名空间暴露为“**可打开的文件**”；`setns()` 通过 `ops->install()` 完成线程加入；
* **保活策略**：打开的 ns fd、bind-mount 到文件路径（如 `/var/run/netns/<name>`）均增加引用计数，避免对象回收。

---

## 10) 三兄弟系统调用：典型调用链

* **`clone3` 路径**：
  `copy_process()` 早期根据 `CLONE_NEW*` 构建目标 `nsproxy/cred`，必要时调用 `create_*_namespace()`，使子进程**出生即处于**新实例。
* **`unshare` 路径**：
  `do_unshare()` → `unshare_nsproxy_namespaces()`：为**当前线程**复制/新建对象并替换指针（线程级）。
* **`setns` 路径**：
  解析 fd → 能力检查（**目标对象 owner userns**）→ `ops->install()` 把当前线程加入该实例。

---

## 11) 实战工作流（从源码到现象）

**A. 新世界（典型容器启动）**

```bash
# 子进程出生即处于多类新 ns
clone3(CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWNET | ...)

# 子进程内：
# userns：写映射（先 setgroups deny，再写 gid_map）
echo deny > /proc/self/setgroups
printf "0 100000 65536" > /proc/self/uid_map
printf "0 100000 65536" > /proc/self/gid_map

# mntns：防传播 + rootfs + proc/sysfs
mount --make-rprivate /
# 叠镜像 → pivot_root 或 chroot
mount -t proc proc /proc
mount -t sysfs sysfs /sys

# netns：veth/bridge，配 IP/路由/NAT，nft/iptables 与 sysctl
# pidns：记得 fork 一个 init（PID 1）处理 reaper/信号
exec -a init /sbin/tini -- your-app
```

**B. 进入现有容器（nsenter 原理）**

```bash
fd=$(open "/proc/<pid>/ns/{mnt,uts,ipc,net,pid}")
setns(fd, NSTYPE)
exec /bin/sh
```

**C. PID 特例：必须 fork**

```c
unshare(CLONE_NEWPID);
if (!fork()) {
  /* 子进程是该 pidns 的 PID 1 */
  execl("/sbin/tini","tini","--",argv0,NULL);
}
```

---

## 12) 性能与实现取舍（更贴近真实）

* **不是普遍 COW**：

  * mntns 复制**结构**（非数据）；
  * netns 多子系统各自初始化一套状态（成本相对高，但彼此独立）；
  * uts/ipc/time 等对象相对轻量。
* **延迟/按需初始化**：通过 `pernet_operations` 等框架降低冷启动成本与耦合。
* **并发与扩展性**：广泛使用 **percpu 计数、RCU、精细粒度锁**；netns 销毁时的 `cleanup_net()` 逆序释放确保依赖正确。

---

## 13) 常见误区速改

* **PID 信号**：不是“只能同 ns 发信号”；祖先可见并可向后代发（权限满足），后代看不见祖先，同级互不可见。
* **Cgroup 命名空间**：仅**视图根相对化**；资源限额由控制器（cgroup v2）决定。
* **Time 命名空间**：只偏移 `MONOTONIC/BOOTTIME`，不改 `REALTIME`。
* **线程语义**：`unshare/setns` 仅影响当前**线程**；多线程程序需在单线程阶段完成切换或逐线程处理。
* **PID ns 生效**：`unshare(CLONE_NEWPID)` 后**不立刻换 PID**；**必须 fork** 一个子进程才成为新 pidns 的 PID 1。

---

## 14) 速查表（结构体/源码/要点）

| 类型     | 结构体（简化）                            | 关键源码路径                                                | 关键要点                                                           |
| ------ | ---------------------------------- | ----------------------------------------------------- | -------------------------------------------------------------- |
| PID    | `pid_namespace`, `pid`, `upid[]`   | `kernel/pid_namespace.c`, `kernel/pid.c`, `fs/proc/*` | 需 **fork** 才获新 PID；/proc 视图按“查看者 pidns”渲染                      |
| Mount  | `mnt_namespace`, `mount`           | `fs/namespace.c`, `include/linux/mount.h`             | 共享子树传播；`create_mnt_ns()` 复制视图；**idmapped mount**               |
| Net    | `net` + `pernet_operations`        | `net/core/net_namespace.c` + 各协议子树                    | per-net init/exit 钩子；`sk_net`/`dev_net()`；`cleanup_net()` 逆序释放 |
| UTS    | `uts_namespace`                    | `kernel/utsname.c`                                    | `sethostname()` 只改当前 utsns；owner userns 能力检查                   |
| IPC    | `ipc_namespace`, `ipc_ids`         | `ipc/*.c`, `fs/mqueue/*`                              | SysV 三套 id 空间；mqueue 每 ns 一套                                   |
| User   | `user_namespace`, `cred`           | `kernel/user_namespace.c`, `kernel/cred.c`            | ID 映射 + 能力边界；`ns_capable()` 统一授权                               |
| Cgroup | `cgroup_namespace`                 | `kernel/cgroup/*`, `fs/nsfs.c`                        | **仅视图**相对化；资源隔离由控制器负责                                          |
| Time   | `time_namespace`, `timens_offsets` | `kernel/time/time_namespace.c`                        | 叠加 monotonic/boottime 偏移；不改墙钟                                  |

---

## 15) 附：`nsproxy` 结构（对照速览，简化示意）

```c
struct nsproxy {
    atomic_t count;
    struct uts_namespace    *uts_ns;
    struct ipc_namespace    *ipc_ns;
    struct mnt_namespace    *mnt_ns;
    struct pid_namespace    *pid_ns_for_children;
    struct net              *net_ns;
    struct cgroup_namespace *cgroup_ns;
    struct time_namespace   *time_ns;
};
/* 用户命名空间在 cred： */
struct cred {
    struct user_namespace *user_ns;
    /* 以及 euid/egid/capabilities 等 */
};
```

---

### 推荐阅读顺序（源码入口提示）

1. **通用**：`fs/nsfs.c`、`kernel/fork.c` 中的 `copy_namespaces()/create_new_namespaces()`
2. **挂载**：`fs/namespace.c`（`do_mount/propagate_*`）
3. **网络**：`net/core/net_namespace.c`（`setup_net/cleanup_net` + 各协议目录）
4. **PID**：`kernel/pid_namespace.c`、`kernel/pid.c`、`fs/proc/*`
5. **用户**：`kernel/user_namespace.c`、`kernel/cred.c`
6. **IPC/UTS/Time/Cgroup**：对应子系统目录下的 `*namespace*.c` 与 `fs/mqueue/*`

---

**实操建议**：配合 `nsenter/unshare/setns`、`cat /proc/$$/ns/*` 比对 inode 号，逐章验证行为；遇到 `EPERM`，首先确认 **owner userns** 与 **uid/gid 映射** 是否正确。
