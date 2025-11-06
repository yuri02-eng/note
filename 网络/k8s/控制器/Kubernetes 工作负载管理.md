# Kubernetes 工作负载管理全面解析

## 一、核心概念体系

### 1.1 Pod：工作负载的基础单元
Pod是Kubernetes中最小的可部署单元，具有以下特性：

**核心特性：**
- 一个或多个容器的组合（通常为1个主容器+辅助容器）
- 容器间共享存储、网络和命名空间
- 是临时性的，会被频繁创建和销毁
- 拥有独立的IP地址，但生命周期不稳定

**Pod设计模式：**
```yaml
# 典型Pod结构示例
apiVersion: v1
kind: Pod
metadata:
  name: web-app
  labels:
    app: web
spec:
  containers:
  - name: main-container
    image: nginx:1.20
    ports:
    - containerPort: 80
  - name: sidecar-container  # Sidecar模式
    image: log-collector:latest
```

### 1.2 工作负载资源层级架构

Kubernetes通过控制器模式管理Pod生命周期，形成清晰的层级关系：

```
工作负载控制器 → 管理 → Pod集合
    ↓
实现扩缩容、更新、自愈等能力
```

## 二、核心工作负载控制器详解

### 2.1 Deployment控制器 - 无状态应用的理想选择

**架构关系：**
```
Deployment → ReplicaSet → Pod副本集群
```

**核心功能：**
- **声明式更新**：定义期望状态，系统自动收敛
- **滚动更新**：零停机部署，支持版本回滚
- **副本管理**：确保指定数量的Pod始终运行

**滚动更新流程：**
1. 创建新的ReplicaSet并逐步扩容
2. 新旧Pod同时服务，确保流量不中断
3. 逐步终止旧ReplicaSet的Pod
4. 更新完成，仅保留新版本Pod

**配置示例：**
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-deployment
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web
  template:
    metadata:
      labels:
        app: web
    spec:
      containers:
      - name: nginx
        image: nginx:1.20
        ports:
        - containerPort: 80
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1        # 最大激增Pod数
      maxUnavailable: 0   # 最大不可用Pod数
```

### 2.2 StatefulSet控制器 - 有状态应用的专用方案

**适用场景：**
- 数据库（MySQL、PostgreSQL等）
- 消息队列（Kafka、RabbitMQ等）
- 分布式存储系统
- 需要稳定标识的任何应用

**核心特性对比：**

| 特性维度 | StatefulSet | Deployment |
|---------|-------------|------------|
| **网络标识** | 稳定、有序（web-0, web-1） | 随机、临时 |
| **存储持久化** | 稳定的专属存储卷 | 临时或共享存储 |
| **扩缩容** | 有序进行（依次创建/删除） | 可并发操作 |
| **更新策略** | 有序滚动更新 | 多种更新策略 |
| **服务发现** | 稳定的DNS记录 | 负载均衡 |

**StatefulSet配置示例：**
```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql-cluster
spec:
  serviceName: "mysql"
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
    spec:
      containers:
      - name: mysql
        image: mysql:8.0
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
  volumeClaimTemplates:  # 存储卷声明模板
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 10Gi
```

### 2.3 DaemonSet控制器 - 节点级守护进程

**典型应用场景：**
- 日志收集代理（Fluentd、Filebeat）
- 监控数据采集（Node Exporter）
- 网络插件组件（Calico、Flannel）
- 存储守护进程

**核心特性：**
- 每个节点运行一个Pod副本
- 新节点加入时自动部署
- 节点移除时自动清理
- 确保关键服务在所有节点可用

**DaemonSet配置示例：**
```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: log-collector
spec:
  selector:
    matchLabels:
      name: log-collector
  template:
    metadata:
      labels:
        name: log-collector
    spec:
      containers:
      - name: fluentd
        image: fluent/fluentd:latest
        volumeMounts:
        - name: varlog
          mountPath: /var/log
      volumes:
      - name: varlog
        hostPath:
          path: /var/log
```

### 2.4 Job与CronJob控制器 - 批处理任务管理

**Job类型与执行模式：**

| 任务类型 | 配置方式 | 适用场景 |
|---------|---------|----------|
| **单次任务** | completions: 1, parallelism: 1 | 数据库迁移、数据导出 |
| **并行任务** | completions: N, parallelism: M | 大规模数据处理、批量计算 |
| **索引任务** | completionMode: Indexed | 分片处理、MapReduce模式 |

**CronJob定时调度：**
```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: daily-backup
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点执行
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: backup
            image: backup-tool:latest
          restartPolicy: OnFailure
```

## 三、工作负载生命周期管理

### 3.1 Pod生命周期状态机

```
创建中（Pending） 
    ↓
运行中（Running） → 成功（Succeeded）
    ↓
失败（Failed）
    ↓
未知（Unknown）
```

### 3.2 健康检查探针体系

**三种探针类型对比：**

| 探针类型 | 检测目标 | 失败后果 | 配置建议 |
|---------|---------|----------|----------|
| **启动探针** | 应用启动完成 | 延迟其他探针 | 慢启动应用 |
| **存活探针** | 容器健康状态 | 重启容器 | 必须配置 |
| **就绪探针** | 服务就绪状态 | 从Endpoint移除 | 流量管理 |

**探针配置示例：**
```yaml
containers:
- name: web
  image: nginx:latest
  livenessProbe:
    httpGet:
      path: /health
      port: 80
    initialDelaySeconds: 30
    periodSeconds: 10
  readinessProbe:
    httpGet:
      path: /ready
      port: 80
    initialDelaySeconds: 5
    periodSeconds: 5
  startupProbe:
    httpGet:
      path: /startup
      port: 80
    failureThreshold: 30
    periodSeconds: 10
```

### 3.3 水平Pod自动扩缩容（HPA）

**HPA工作流程：**
1. 监控目标资源的指标数据
2. 与期望值比较计算所需副本数
3. 调整Deployment/StatefulSet的副本数量
4. 持续监控并自动调整

**HPA配置示例：**
```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-deployment
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 50
```

## 四、高级工作负载模式

### 4.1 多容器协作模式

**Init容器模式：**
```yaml
spec:
  initContainers:
  - name: init-db
    image: db-migrate:latest
    command: ['sh', '-c', 'until nslookup mysql; do echo waiting; sleep 2; done']
  containers:
  - name: main-app
    image: app:latest
```

**Sidecar容器模式：**
```yaml
containers:
- name: main-app
  image: app:latest
- name: log-agent          # Sidecar容器
  image: log-collector:latest
  volumeMounts:
  - name: shared-logs
    mountPath: /var/log
```

### 4.2 Pod中断预算（PDB）管理

**PDB配置策略：**
```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-pdb
spec:
  minAvailable: 2        # 保证最少2个Pod可用
  # 或使用 maxUnavailable: 1
  selector:
    matchLabels:
      app: web
```

**中断类型管理：**
- **自愿中断**：受PDB保护（节点维护、手动删除）
- **非自愿中断**：不受PDB保护（节点故障、资源不足）

## 五、最佳实践指南

### 5.1 资源管理与调度优化

**资源请求与限制配置：**
```yaml
resources:
  requests:
    cpu: "100m"
    memory: "128Mi"
  limits:
    cpu: "500m" 
    memory: "512Mi"
```

**资源管理原则：**
- 所有容器必须设置资源请求
- 关键应用设置合理的资源限制
- 避免资源超售导致的性能问题
- 使用LimitRange和ResourceQuota进行命名空间级限制

### 5.2 高可用性配置策略

**多维度高可用方案：**
```yaml
# 1. 多副本部署
replicas: 3

# 2. Pod反亲和性
affinity:
  podAntiAffinity:
    preferredDuringSchedulingIgnoredDuringExecution:
    - weight: 100
      podAffinityTerm:
        labelSelector:
          matchExpressions:
          - key: app
            operator: In
            values:
            - web
        topologyKey: kubernetes.io/hostname

# 3. PDB配置
minAvailable: 2
```

### 5.3 扩缩容策略矩阵

| 扩缩容类型 | 实现机制 | 适用场景 | 配置要点 |
|----------|---------|----------|----------|
| **水平扩缩** | HPA + Deployment | 无状态应用、流量波动 | 基于CPU/内存或自定义指标 |
| **垂直扩缩** | VPA | 有状态应用、资源需求变化 | 需要重启Pod，谨慎使用 |
| **集群扩缩** | Cluster Autoscaler | 整体资源容量管理 | 与云提供商集成 |

### 5.4 工作负载控制器选择指南

**决策流程图：**
```
应用是否需要持久化存储或稳定标识？
    ↓
是 → 选择StatefulSet（数据库、有状态服务）
    ↓
否 → 应用是否需要长期运行？
        ↓
    是 → 选择Deployment（Web服务、API）
        ↓
    否 → 应用是否需要定时或一次性执行？
            ↓
        是 → 选择Job/CronJob（批处理任务）
            ↓
        否 → 是否需要每个节点运行一个副本？
                ↓
            是 → 选择DaemonSet（日志收集、监控代理）
```

## 六、总结

Kubernetes工作负载管理体系提供了从基础Pod管理到高级编排能力的完整解决方案。关键要点包括：

1. **正确选择控制器**：根据应用特性选择最适合的工作负载资源
2. **完善生命周期管理**：通过探针、PDB等机制保障应用稳定性
3. **自动化运维**：利用HPA、滚动更新等实现智能化运维
4. **资源优化**：合理配置资源请求和限制，提高集群利用率

通过深入理解各控制器的特性和最佳实践，可以构建出高效、稳定、可扩展的Kubernetes应用部署体系。