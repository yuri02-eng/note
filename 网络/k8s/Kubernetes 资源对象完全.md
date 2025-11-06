Kubernetes 资源对象完全指南 - 架构师笔记

一、核心架构理解

Kubernetes 设计哲学

• 声明式API：描述"期望状态"，系统自动驱动当前状态向期望状态收敛

• 控制器模式：通过控制循环确保系统状态与声明的一致

• 解耦设计：应用与基础设施解耦，开发与运维关注点分离

对象管理层次


集群级别对象 (Cluster)
├── 命名空间级别对象 (Namespace)  
│   ├── 工作负载对象 (Workloads)
│   ├── 服务发现对象 (Service Discovery)
│   └── 配置对象 (Configuration)
└── 存储与网络对象 (Storage/Network)


二、工作负载对象 - 应用载体

1. Pod - 最小调度单元

核心特性：
• 一个或多个紧密关联容器的组合

• 共享网络命名空间（同一IP）、存储卷、IPC

• 临时性存在，故障后重新调度

设计模式：
• 单容器模式：主流用法

• 边车模式：辅助容器（日志收集、代理）

• 适配器模式：数据格式转换

• 大使模式：代理外部服务

2. Deployment - 无状态应用标准管理器

核心能力：
• 声明式更新：滚动更新、回滚、暂停/继续

• 副本管理：通过ReplicaSet维护指定数量的Pod副本

• 健康检查：结合探针确保应用可用性

适用场景：
• Web应用、API服务、无状态微服务

• 需要平滑发布、版本管理的应用

3. StatefulSet - 有状态应用专家

核心特性：
• 稳定网络标识：Pod名称有序、稳定（web-0, web-1...）

• 持久化存储：每个Pod独立的PVC，生命周期与Pod绑定

• 有序部署：按顺序启停（0→1→2...），逆序删除

适用场景：
• 数据库集群（MySQL、MongoDB）

• 消息队列（Kafka、RabbitMQ）

• 需要稳定标识和持久化存储的应用

4. DaemonSet - 节点级守护进程

核心特性：
• 每个节点运行一个Pod副本

• 新节点加入时自动部署

• 适合系统级服务

适用场景：
• 日志收集器（Fluentd、Filebeat）

• 监控代理（Node Exporter）

• 网络插件（Calico、Flannel）

• 存储插件（Ceph、GlusterFS）

5. Job/CronJob - 批处理任务

核心特性：
• Job：运行至完成的一次性任务

• CronJob：基于时间表的定时任务

• 支持并行处理、重试策略

适用场景：
• 数据迁移、备份任务

• 报表生成、批量处理

• 定期维护任务

三、服务与网络 - 连通性保障

1. Service - 服务发现与负载均衡

四种类型对比：

类型 暴露范围 适用场景 特点

ClusterIP 集群内部 微服务间通信 默认类型，虚拟IP

NodePort 节点级别 开发测试 通过节点IP+端口访问

LoadBalancer 外部访问 生产环境 云提供商负载均衡器

Headless 集群内部 有状态服务 直接Pod IP，无负载均衡

服务发现机制：
• DNS解析：<service>.<namespace>.svc.cluster.local

• 环境变量注入：SERVICE_HOST、SERVICE_PORT

• 端点监控：实时更新后端Pod地址列表

2. Ingress - L7流量路由

核心功能：
• 基于主机名和路径的路由

• SSL/TLS终止

• 负载均衡、会话保持

• 认证、限流等高级功能

与Service关系：

Internet → Ingress Controller → Service → Pods


常见控制器：
• Nginx Ingress：功能丰富，社区活跃

• Traefik：动态配置，容器原生

• HAProxy Ingress：高性能，企业级

四、配置与存储 - 数据管理

1. ConfigMap - 应用配置

使用方式：
• 环境变量：整体或单个键值注入

• 卷挂载：以文件形式挂载到容器

• 命令行参数：动态生成启动参数

最佳实践：
• 配置与镜像分离

• 支持热更新（卷挂载方式）

• 按环境分类管理

2. Secret - 敏感信息管理

类型分类：
• Opaque：用户自定义数据（base64编码）

• docker-registry：镜像仓库认证

• tls：TLS证书文件

• service-account：服务账户令牌

安全注意：
• base64不是加密，需要配合RBAC、加密存储

• 考虑使用外部密钥管理方案（HashiCorp Vault等）

3. 存储抽象体系

核心概念：

PersistentVolume (PV) - 集群存储资源
PersistentVolumeClaim (PVC) - 用户存储请求  
StorageClass (SC) - 动态供给模板


访问模式：
• RWO：ReadWriteOnce - 单节点读写

• ROX：ReadOnlyMany - 多节点只读

• RWX：ReadWriteMany - 多节点读写

持久化数据流向：

Pod → PVC → PV → 实际存储设备


五、集群管理 - 资源组织

1. Namespace - 虚拟集群

核心作用：
• 资源隔离：不同项目、环境、团队隔离

• 资源配额：限制命名空间资源消耗

• 权限控制：RBAC权限作用域

系统命名空间：
• kube-system：系统组件

• kube-public：公开可读资源

• kube-node-lease：节点心跳

2. Label - 对象标签

核心价值：
• 标识属性：环境（dev/test/prod）、版本、层级

• 选择器基础：服务选择、部署匹配

• 操作筛选：批量操作、监控分组

选择器类型：
• 等值选择器：=, !=, in, notin

• 集合选择器：exists, doesnotexist

3. Node - 工作节点

节点状态：
• Ready：节点健康状态

• MemoryPressure：内存压力

• DiskPressure：磁盘压力

• PIDPressure：进程ID压力

节点管理：
• 污点与容忍：Pod调度约束

• 节点亲和性：调度偏好设置

• 资源预留：系统组件资源保障

六、安全与权限 - 访问控制

1. ServiceAccount - 身份认证

核心作用：
• Pod身份标识：与API Server通信的凭证

• 权限载体：RBAC权限绑定的主体

• 自动化管理：默认自动创建和挂载

2. RBAC - 基于角色的访问控制

核心组件：

角色定义：Role（命名空间级）、ClusterRole（集群级）
权限绑定：RoleBinding、ClusterRoleBinding
主体：ServiceAccount、User、Group


权限模型：
• 最小权限原则：按需授权

• 角色继承：通过绑定组合权限

• 审计追踪：操作记录可追溯

3. SecurityContext - 安全上下文

容器级别：
• 用户身份：runAsUser、runAsGroup

• 权限控制：allowPrivilegeEscalation、capabilities

• SELinux/AppArmor：安全模块标签

Pod级别：
• 共享进程命名空间

• 文件系统限制：readOnlyRootFilesystem

• 特权模式控制：privileged

七、资源管理 - 配额与弹性

1. ResourceQuota - 资源配额

限制维度：
• 计算资源：CPU、内存请求和限制

• 存储资源：存储请求总量

• 对象数量：Pod、Service、PVC等数量

多层级配额：
• 命名空间级别：总资源上限

• 租户级别：跨命名空间聚合限制

2. LimitRange - 默认限制

作用范围：
• 容器默认资源：自动设置请求和限制

• 存储默认大小：PVC默认存储容量

• 比例约束：限制与请求的比例关系

3. HPA - 水平Pod自动扩缩容

扩缩容指标：
• CPU利用率：最常用指标

• 内存利用率：有状态应用

• 自定义指标：QPS、消息队列长度等

算法原理：

期望副本数 = ceil[当前副本数 × (当前指标值 / 期望指标值)]


八、扩展性 - 自定义能力

1. CRD - 自定义资源定义

扩展模式：
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: applications.example.com
spec:
  group: example.com
  versions: [...]
  scope: Namespaced  # 集群级别或命名空间级别
  names:
    plural: applications
    singular: application
    kind: Application


使用场景：
• 领域特定应用：数据库、中间件操作抽象

• 运维自动化：封装复杂运维逻辑

• 平台扩展：构建自定义PaaS平台

2. Operator模式

核心思想：
• 自定义资源：描述应用期望状态

• 自定义控制器：驱动当前状态向期望状态收敛

• 领域知识编码：将运维经验转化为代码

典型实现：
• etcd-operator：etcd集群管理

• prometheus-operator：监控栈管理

• 数据库Operator：MySQL、PostgreSQL集群管理

九、对象关系与生命周期

对象依赖关系

    ├── ResourceQuota, LimitRange
    ├── ConfigMap, Secret
    ├── ServiceAccount
    │   └── RoleBinding → Role
    ├── Deployment → ReplicaSet → Pod
    │   ├── Service → Endpoints
    │   └── PersistentVolumeClaim → PersistentVolume
    ├── StatefulSet → Pod
    ├── DaemonSet → Pod  
    └── Ingress → Service


控制器协作模式


API Server (etcd)
    ├── Deployment Controller → ReplicaSet Controller → Pod
    ├── StatefulSet Controller → Pod
    ├── DaemonSet Controller → Pod
    ├── Job Controller → Pod
    ├── Service Controller → Endpoints
    ├── Ingress Controller → Load Balancer
    └── PersistentVolume Controller → Storage


十、最佳实践总结

工作负载设计原则

1. 无状态优先：优先使用Deployment，简化运维
2. 有状态抽象：StatefulSet提供稳定网络和存储
3. 节点服务隔离：DaemonSet用于系统级服务
4. 批量任务标准化：Job/CronJob统一任务管理

网络设计原则

1. 服务发现内部化：ClusterIP Service内部通信
2. 入口流量统一：Ingress集中管理外部访问
3. 网络策略细化：按需实施最小权限网络控制

配置管理原则

1. 配置外置：ConfigMap/Secret管理配置
2. 安全分离：敏感信息专用Secret管理
3. 版本控制：配置与代码一同版本化

资源管理原则

1. 资源限制：所有容器设置资源请求和限制
2. 自动扩缩：HPA根据业务负载动态调整
3. 配额管理：ResourceQuota防止资源滥用

这份笔记涵盖了Kubernetes核心资源对象的架构思想和实践要点，可作为日常运维和系统设计的参考指南。