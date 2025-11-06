# Kubernetes 容器模式全景图 - 完整学习笔记

## 1. 容器模式概述

### 1.1 什么是容器模式？
容器模式是指在 Kubernetes Pod 中，多个容器按照特定方式组织和协作的设计模式。每种模式解决特定的架构问题，实现关注点分离和功能复用。

### 1.2 核心价值
- **模块化**：将复杂应用拆分为专注单一功能的容器
- **可复用**：通用功能抽象为独立容器，跨应用复用
- **可维护**：各组件独立开发、升级、扩展
- **灵活性**：通过组合不同模式构建复杂应用

## 2. 容器模式分类体系

```mermaid
graph TB
    A[Kubernetes 容器模式] --> B[按功能角色划分]
    A --> C[按生命周期划分]
    A --> D[特殊用途模式]
    
    B --> B1[主容器]
    B --> B2[Sidecar 容器]
    B --> B3[Adapter 容器]
    B --> B4[Ambassador 容器]
    
    C --> C1[Init 容器]
    C --> C2[Ephemeral 容器]
    
    D --> D1[Probe 容器]
    D --> D2[Batch 容器]
    D --> D3[Worker 容器]
```

## 3. 按功能角色划分的容器模式

### 3.1 主容器（Primary Container）
**核心作用**：运行业务逻辑的主要容器

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: web-application
spec:
  containers:
  # 主容器 - 业务核心
  - name: web-server
    image: nginx:1.25
    ports:
    - containerPort: 80
    resources:
      requests:
        memory: "128Mi"
        cpu: "100m"
      limits:
        memory: "256Mi" 
        cpu: "200m"
    livenessProbe:
      httpGet:
        path: /health
        port: 80
```

**特点**：
- 每个 Pod 至少有一个主容器
- 承载核心业务逻辑
- 定义 Pod 的总体目的和身份

### 3.2 Sidecar 容器模式
**核心作用**：增强或扩展主容器功能

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-sidecar
spec:
  containers:
  # 主容器：业务应用
  - name: main-app
    image: myapp:latest
    volumeMounts:
    - name: shared-logs
      mountPath: /var/log/app
  
  # Sidecar 容器：日志收集
  - name: log-collector
    image: fluentd:latest
    volumeMounts:
    - name: shared-logs
      mountPath: /var/log/app
    - name: config-volume
      mountPath: /etc/fluentd
    command:
    - fluentd
    - -c
    - /etc/fluentd/fluent.conf
  
  volumes:
  - name: shared-logs
    emptyDir: {}
  - name: config-volume
    configMap:
      name: fluentd-config
```

**Sidecar 典型应用场景**：
| 场景 | 功能 | 示例容器 |
|------|------|----------|
| **日志收集** | 收集、转发应用日志 | Fluentd, Filebeat |
| **监控指标** | 暴露应用指标 | Prometheus exporter |
| **配置热更新** | 动态更新配置 | ConfigMap 热加载器 |
| **安全代理** | mTLS、认证 | Istio Proxy, Vault agent |
| **数据同步** | 文件同步、备份 | Rsync, Cloud Sync |

### 3.3 Adapter 容器模式
**核心作用**：标准化输出格式，实现数据转换

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: metrics-adapter
spec:
  containers:
  # 主容器：产生自定义指标格式
  - name: custom-app
    image: custom-metrics-app:latest
    volumeMounts:
    - name: metrics-data
      mountPath: /app/metrics
  
  # Adapter 容器：转换为标准格式
  - name: prometheus-adapter
    image: prometheus-adapter:latest
    volumeMounts:
    - name: metrics-data
      mountPath: /input
    - name: adapted-metrics
      mountPath: /output
    command:
    - /bin/adapter
    - --input-format=custom
    - --output-format=prometheus
    - --input-dir=/input
    - --output-dir=/output
  
  volumes:
  - name: metrics-data
    emptyDir: {}
  - name: adapted-metrics
    emptyDir: {}
```

**Adapter 模式特点**：
- **输入输出转换**：将非标准数据转换为标准格式
- **协议适配**：不同协议间的转换桥接
- **数据标准化**：统一监控、日志、跟踪数据格式

### 3.4 Ambassador 容器模式
**核心作用**：简化外部服务访问，处理网络复杂性

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: database-client
spec:
  containers:
  # 主容器：业务逻辑
  - name: app-server
    image: myapp:latest
    env:
    - name: DATABASE_URL
      value: "localhost:5432"  # 连接本地 Ambassador
  
  # Ambassador 容器：数据库代理
  - name: db-ambassador
    image: postgres-proxy:latest
    env:
    - name: DB_PRIMARY
      value: "postgres-primary:5432"
    - name: DB_REPLICAS
      value: "postgres-replica1:5432,postgres-replica2:5432"
    ports:
    - containerPort: 5432
    # 提供：连接池、故障转移、负载均衡、重试逻辑
```

**Ambassador 功能**：
- **服务发现**：自动发现后端服务实例
- **负载均衡**：在多个后端实例间分发请求
- **故障转移**：自动检测故障并切换到健康实例
- **重试机制**：处理临时故障，提高可靠性

## 4. 按生命周期划分的容器模式

### 4.1 Init 容器模式
**核心作用**：在主容器启动前执行初始化任务

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-init
spec:
  # Init 容器 - 按顺序执行
  initContainers:
  # 1. 等待依赖服务就绪
  - name: wait-for-db
    image: busybox:1.35
    command: ['sh', '-c', 'until nslookup database-service; do echo waiting; sleep 2; done']
  
  # 2. 执行数据库迁移
  - name: run-migrations
    image: db-migrator:latest
    command: ['/bin/run-migrations.sh']
    env:
    - name: DB_URL
      value: "postgresql://user:pass@database-service:5432/app"
  
  # 3. 下载配置文件
  - name: download-config
    image: appropriate/curl
    command: ['sh', '-c', 'curl -o /shared/config.yaml http://config-server/v1/config']
    volumeMounts:
    - name: app-config
      mountPath: /shared

  # 主容器
  containers:
  - name: main-app
    image: myapp:latest
    volumeMounts:
    - name: app-config
      mountPath: /etc/app-config

  volumes:
  - name: app-config
    emptyDir: {}
```

**Init 容器特性**：
- **顺序执行**：按定义顺序串行执行
- **全部成功**：所有 Init 容器必须成功退出（退出码 0）
- **阻塞启动**：主容器等待所有 Init 容器完成后启动
- **独立镜像**：可使用与主容器不同的工具镜像

### 4.2 Ephemeral 容器模式
**核心作用**：临时调试，不用于常规业务逻辑

```bash
# 运行时注入临时调试容器
kubectl debug -it my-pod --image=busybox:1.35 --target=my-pod

# 复制模式调试（Pod 已崩溃时）
kubectl debug -it crashed-pod --image=busybox:1.35 --copy-to=debug-pod --share-processes

# 节点级别调试
kubectl debug -it node/my-node --image=busybox:1.35
```

**Ephemeral 容器使用场景**：
```bash
# 网络诊断
kubectl debug -it network-issue-pod --image=nicolaka/netshoot
# 进入后执行：nslookup, ping, tcpdump, netstat

# 进程诊断  
kubectl debug -it hanging-pod --image=busybox:1.35 --share-processes
# 进入后执行：ps aux, lsof, strace

# 文件系统检查
kubectl debug -it fs-corruption-pod --image=busybox:1.35
# 进入后执行：ls, cat, find, du
```

## 5. 特殊用途容器模式

### 5.1 Probe 容器模式
**核心作用**：执行复杂健康检查逻辑

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-advanced-probes
spec:
  containers:
  # 主容器：简单健康检查
  - name: web-app
    image: myapp:latest
    livenessProbe:
      httpGet:
        path: /health
        port: 8080
  
  # Probe 容器：复杂健康检查
  - name: advanced-health-check
    image: health-checker:latest
    command:
    - /bin/health-check
    - --check-database
    - --check-external-api  
    - --validate-cache
    - --output-file=/health-status/status.json
    volumeMounts:
    - name: health-status
      mountPath: /health-status
    # 将检查结果通过共享卷传递给主容器

  volumes:
  - name: health-status
    emptyDir: {}
```

### 5.2 Batch 容器模式
**核心作用**：与主容器协作处理批处理任务

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: data-processing
spec:
  containers:
  # 流处理主容器
  - name: stream-processor
    image: stream-app:latest
    volumeMounts:
    - name: processed-data
      mountPath: /data/processed
  
  # Batch 容器：定期批处理
  - name: batch-aggregator
    image: batch-processor:latest
    command:
    - /bin/batch-job
    - --input-dir=/data/processed
    - --output-dir=/data/aggregated
    - --schedule=@hourly
    volumeMounts:
    - name: processed-data
      mountPath: /data/processed
    - name: aggregated-data
      mountPath: /data/aggregated

  volumes:
  - name: processed-data
    emptyDir: {}
  - name: aggregated-data
    emptyDir: {}
```

## 6. 容器模式选择决策框架

### 6.1 模式选择决策树
```mermaid
graph TD
    A[选择容器模式] --> B{主要需求是什么？}
    
    B -->|初始化/依赖检查| C[Init 容器]
    B -->|增强主容器功能| D{具体需求？}
    B -->|临时调试| E[Ephemeral 容器]
    B -->|批处理协作| F[Batch 容器]
    B -->|复杂健康检查| G[Probe 容器]
    
    D -->|数据格式转换| H[Adapter 容器]
    D -->|简化外部服务访问| I[Ambassador 容器]
    D -->|通用功能增强| J[Sidecar 容器]
    
    C --> K[串行执行，完成后退出]
    H --> L[数据标准化，格式转换]
    I --> M[网络代理，服务抽象]
    J --> N[功能扩展，并行运行]
    E --> O[临时注入，调试专用]
    F --> P[批处理，定期执行]
    G --> Q[健康检查，状态验证]
```

### 6.2 模式组合策略
**复杂应用的多模式组合示例**：
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: complex-application
spec:
  # 1. 初始化阶段
  initContainers:
  - name: init-dependencies
    image: busybox:1.35
    command: ['sh', '-c', 'until ready-check.sh; do sleep 2; done']
  
  # 2. 运行阶段 - 多模式组合
  containers:
  # 主业务容器
  - name: main-business-app
    image: business-app:latest
  
  # Sidecar：日志收集
  - name: log-sidecar
    image: fluentd:latest
  
  # Adapter：指标格式转换
  - name: metrics-adapter
    image: prometheus-adapter:latest
  
  # Ambassador：外部服务代理
  - name: service-ambassador
    image: envoyproxy/envoy:latest
    ports:
    - containerPort: 8080
```

## 7. 最佳实践与反模式

### 7.1 容器模式最佳实践
1. **单一职责原则**
   - 每个容器专注一个特定功能
   - 避免"上帝容器"反模式

2. **松耦合设计**
   - 容器间通过标准接口通信（文件、网络）
   - 避免硬编码依赖

3. **资源合理分配**
   - 为每个容器设置适当的资源请求和限制
   - 考虑容器间的资源竞争

4. **生命周期管理**
   - 理解不同容器的启动顺序和依赖关系
   - 配置适当的优雅终止策略

### 7.2 常见反模式
```yaml
# 反模式1：上帝容器（一个容器做所有事）
containers:
- name: god-container
  image: do-everything:latest
  # 同时处理：业务逻辑、日志收集、监控、配置管理...

# 反模式2：硬编码依赖
containers:
- name: app
  image: myapp:latest
  command:
  - /bin/sh
  - -c
  - "sleep 10 && /app/start.sh"  # 硬编码等待依赖
```

## 8. 实战案例：完整应用架构

### 8.1 微服务应用完整示例
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: microservice-pod
  labels:
    app: user-service
    version: v1.2.3
spec:
  # 初始化容器
  initContainers:
  - name: wait-for-dependencies
    image: busybox:1.35
    command: ['sh', '-c', 'until nslookup redis-service && nslookup db-service; do sleep 2; done']
  
  - name: load-config
    image: appropriate/curl
    command: ['sh', '-c', 'curl -o /shared/config/app.yaml $CONFIG_SERVER/config']
    env:
    - name: CONFIG_SERVER
      value: "http://config-service:8080"
    volumeMounts:
    - name: app-config
      mountPath: /shared/config

  # 运行期容器
  containers:
  # 主业务容器
  - name: user-service
    image: user-service:1.2.3
    ports:
    - containerPort: 8080
    volumeMounts:
    - name: app-config
      mountPath: /app/config
    livenessProbe:
      httpGet:
        path: /health
        port: 8080
    readinessProbe:
      httpGet:
        path: /ready
        port: 8080

  # Sidecar：日志收集
  - name: log-collector
    image: fluentd:latest
    volumeMounts:
    - name: app-logs
      mountPath: /var/log/app
    - name: fluentd-config
      mountPath: /etc/fluentd

  # Adapter：指标转换
  - name: metrics-adapter
    image: prometheus-adapter:latest
    volumeMounts:
    - name: metrics-data
      mountPath: /metrics

  # Ambassador：数据库访问
  - name: db-ambassador
    image: pgbouncer:latest
    ports:
    - containerPort: 5432
    env:
    - name: DB_URL
      value: "postgresql://user:pass@db-service:5432/users"

  volumes:
  - name: app-config
    emptyDir: {}
  - name: app-logs
    emptyDir: {}
  - name: metrics-data
    emptyDir: {}
  - name: fluentd-config
    configMap:
      name: fluentd-config
```

## 9. 总结

### 9.1 模式对比表
| 模式 | 核心功能 | 生命周期 | 典型场景 |
|------|----------|----------|----------|
| **主容器** | 业务逻辑 | 长期运行 | Web 服务、API 服务 |
| **Sidecar** | 功能增强 | 与主容器并行 | 日志、监控、安全 |
| **Adapter** | 数据转换 | 与主容器并行 | 格式标准化、协议转换 |
| **Ambassador** | 网络代理 | 与主容器并行 | 服务发现、负载均衡 |
| **Init 容器** | 初始化 | 主容器之前 | 依赖检查、数据准备 |
| **Ephemeral** | 临时调试 | 运行时注入 | 故障诊断、性能分析 |

### 9.2 学习要点
1. **理解每种模式的设计意图**和适用场景
2. **掌握模式组合策略**，构建复杂应用架构
3. **遵循最佳实践**，避免常见反模式
4. **结合实际业务需求**选择合适的模式组合

通过掌握这些容器模式，您将能够设计出更加**模块化、可维护、可扩展**的 Kubernetes 应用架构。