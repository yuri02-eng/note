# Kubernetes 亲和性调度详细指南

## 1. 概述与核心概念

### 1.1 什么是亲和性调度？
Kubernetes 亲和性调度是一组高级调度策略，允许用户精确控制 Pod 在集群中的部署位置。与默认的调度器（基于资源需求和负载均衡）不同，亲和性调度提供了基于标签匹配的细粒度控制。

### 1.2 为什么需要亲和性调度？
- **性能优化**：将通信频繁的应用部署在同一节点或同一区域
- **高可用性**：确保应用副本分布在不同的故障域
- **资源隔离**：隔离关键应用和普通应用
- **硬件特性匹配**：将需要特殊硬件（如GPU）的应用调度到特定节点
- **合规性要求**：满足数据驻留或监管要求

### 1.3 核心组件
- **节点亲和性 (Node Affinity)**：控制 Pod 与节点的关系
- **Pod 亲和性 (Pod Affinity)**：控制 Pod 与其他 Pod 的共处关系
- **Pod 反亲和性 (Pod Anti-Affinity)**：控制 Pod 与其他 Pod 的隔离关系
- **污点与容忍 (Taints & Tolerations)**：节点的"排斥"机制和 Pod 的"忍受"能力

## 2. 基础：nodeSelector

### 2.1 基本语法
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: nginx
spec:
  containers:
  - name: nginx
    image: nginx:1.14.2
  nodeSelector:
    disktype: ssd        # 必须调度到有 disktype=ssd 标签的节点
    environment: prod    # 必须调度到有 environment=prod 标签的节点
```

### 2.2 实际操作
```bash
# 给节点打标签
kubectl label nodes <node-name> disktype=ssd
kubectl label nodes <node-name> environment=prod

# 查看节点标签
kubectl get nodes --show-labels

# 删除标签
kubectl label nodes <node-name> disktype-
```

### 2.3 局限性
- 只能简单匹配，不支持复杂逻辑
- 硬性要求，没有备选方案
- 无法表达"尽量但不必须"的需求

## 3. 节点亲和性 (Node Affinity)

### 3.1 策略类型
| 策略类型 | 说明 | 行为 |
|---------|------|------|
| `requiredDuringSchedulingIgnoredDuringExecution` | 硬策略 | 必须满足条件，否则不调度 |
| `preferredDuringSchedulingIgnoredDuringExecution` | 软策略 | 尽量满足，不满足也可调度 |

### 3.2 操作符详解
| 操作符 | 说明 | 示例 |
|--------|------|------|
| `In` | 标签值在指定列表中 | `values: ["ssd", "fast"]` |
| `NotIn` | 标签值不在指定列表中 | `values: ["hdd", "slow"]` |
| `Exists` | 标签存在（不关心值） | 不需要 `values` |
| `DoesNotExist` | 标签不存在 | 不需要 `values` |
| `Gt` | 标签值大于（数字比较） | `values: ["5"]` |
| `Lt` | 标签值小于（数字比较） | `values: ["10"]` |

### 3.3 完整示例
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-app
  template:
    metadata:
      labels:
        app: web-app
    spec:
      containers:
      - name: web
        image: nginx:1.19
      affinity:
        nodeAffinity:
          # 硬策略：必须满足的条件
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: kubernetes.io/arch
                operator: In
                values:
                - amd64
              - key: environment
                operator: In
                values:
                - production
            - matchExpressions:  # 多个条件满足一个即可
              - key: special
                operator: Exists
          
          # 软策略：优先满足的条件
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 60  # 权重（1-100）
            preference:
              matchExpressions:
              - key: disktype
                operator: In
                values:
                - ssd
          - weight: 40
            preference:
              matchExpressions:
              - key: cpu-type
                operator: In
                values:
                - intel
```

### 3.4 高级匹配模式
```yaml
nodeAffinity:
  requiredDuringSchedulingIgnoredDuringExecution:
    nodeSelectorTerms:
    - matchExpressions:
      - key: topology.kubernetes.io/zone
        operator: In
        values:
        - us-west-2a
        - us-west-2b
    - matchExpressions:  # 第二个条件组（OR逻辑）
      - key: dedicated
        operator: In
        values:
        - web-team
```

## 4. Pod 亲和性与反亲和性

### 4.1 拓扑域 (Topology Domain)
拓扑域定义了"同一位置"的概念，常用拓扑键：
- `kubernetes.io/hostname` - 同一节点
- `topology.kubernetes.io/zone` - 同一可用区
- `topology.kubernetes.io/region` - 同一区域
- `rack` - 同一机柜（自定义标签）

### 4.2 Pod 亲和性 (共处)
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: frontend
spec:
  replicas: 3
  template:
    spec:
      affinity:
        podAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values:
                - cache  # 必须和 cache Pod 在同一拓扑域
            topologyKey: kubernetes.io/hostname
            namespaces: ["default"]  # 可指定命名空间
```

### 4.3 Pod 反亲和性 (隔离)
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-server
spec:
  replicas: 5
  template:
    spec:
      affinity:
        podAntiAffinity:
          # 硬策略：绝对不能在同一拓扑域
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values:
                - web-server  # 自身反亲和，实现高可用
            topologyKey: kubernetes.io/hostname
          
          # 软策略：尽量不在同一拓扑域
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: database
                  operator: In
                  values:
                  - mysql
              topologyKey: topology.kubernetes.io/zone
```

### 4.4 跨命名空间配置
```yaml
podAffinity:
  requiredDuringSchedulingIgnoredDuringExecution:
  - labelSelector:
      matchExpressions:
      - key: security
        operator: In
        values:
        - s1
    topologyKey: topology.kubernetes.io/zone
    namespaceSelector:  # 通过命名空间选择器
      matchExpressions:
      - key: environment
        operator: In
        values:
        - production
```

## 5. 污点 (Taints) 与容忍 (Tolerations)

### 5.1 污点效果
| 效果 | 说明 |
|------|------|
| `NoSchedule` | 新 Pod 不能调度（已存在的不影响） |
| `PreferNoSchedule` | 尽量不调度（软限制） |
| `NoExecute` | 新 Pod 不能调度，已存在的不容忍 Pod 会被驱逐 |

### 5.2 污点管理命令
```bash
# 添加污点
kubectl taint nodes node1 key1=value1:NoSchedule
kubectl taint nodes node2 key2=value2:NoExecute
kubectl taint nodes node3 dedicated=special:PreferNoSchedule

# 查看污点
kubectl describe node <node-name> | grep Taints

# 删除污点
kubectl taint nodes node1 key1:NoSchedule-
kubectl taint nodes node2 key2:NoExecute-

# 删除所有污点
kubectl taint nodes node1 --all
```

### 5.3 容忍配置
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: special-app
spec:
  template:
    spec:
      tolerations:
      # 精确匹配容忍
      - key: "key1"
        operator: "Equal"
        value: "value1"
        effect: "NoSchedule"
      
      # 存在性匹配（只要有这个key就容忍）
      - key: "key2"
        operator: "Exists"
        effect: "NoExecute"
        tolerationSeconds: 3600  # 被驱逐前等待时间（秒）
      
      # 容忍所有 NoSchedule 污点
      - operator: "Exists"
        effect: "NoSchedule"
      
      # 容忍所有效果的所有污点（慎用！）
      - operator: "Exists"
```

### 5.4 系统内置污点
```bash
# Master节点默认污点
node-role.kubernetes.io/master:NoSchedule

# 节点不可用污点（自动添加）
node.kubernetes.io/not-ready:NoExecute
node.kubernetes.io/unreachable:NoExecute
node.kubernetes.io/out-of-disk:NoSchedule
node.kubernetes.io/memory-pressure:NoSchedule
node.kubernetes.io/disk-pressure:NoSchedule
node.kubernetes.io/network-unavailable:NoSchedule
node.kubernetes.io/unschedulable:NoSchedule
```

## 6. 实战应用场景

### 6.1 高可用数据库集群
```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  replicas: 3
  template:
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values:
                - mysql
            topologyKey: topology.kubernetes.io/zone  # 跨可用区部署
      
      tolerations:
      - key: "dedicated"
        operator: "Equal"
        value: "database"
        effect: "NoSchedule"
```

### 6.2 GPU机器学习任务
```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: gpu-training
spec:
  template:
    spec:
      nodeSelector:
        accelerator: gpu
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: gpu-type
                operator: In
                values:
                - nvidia-tesla-v100
      
      tolerations:
      - key: "gpu"
        operator: "Exists"
        effect: "NoSchedule"
```

### 6.3 多租户隔离
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: tenant-a-app
spec:
  template:
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: tenant
                operator: In
                values:
                - tenant-a
        
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: tenant
                operator: In
                values:
                - tenant-b
            topologyKey: kubernetes.io/hostname
```

### 6.4 滚动更新保障
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: canary-app
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  template:
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values:
                - canary-app
            topologyKey: kubernetes.io/hostname
```

## 7. 调试与故障排除

### 7.1 查看调度事件
```bash
# 查看Pod调度事件
kubectl describe pod <pod-name>

# 查看调度器日志
kubectl logs -n kube-system -l component=kube-scheduler

# 查看节点资源
kubectl describe node <node-name>

# 查看无法调度的Pod
kubectl get pods --field-selector status.phase=Pending
```

### 7.2 常见问题解决
**问题1：Pod 一直处于 Pending 状态**
```bash
# 查看详细原因
kubectl describe pod <pod-name>

# 常见原因：
# - 节点资源不足
# - 亲和性规则太严格
# - 污点没有相应容忍
# - 节点选择器不匹配
```

**问题2：调度性能问题**
```yaml
# 优化建议：
# 1. 使用软策略替代硬策略
# 2. 简化匹配表达式
# 3. 避免过于复杂的反亲和性规则
# 4. 使用节点选择器先过滤大部分节点
```

## 8. 最佳实践总结

### 8.1 通用原则
1. **优先使用软策略**：`preferredDuringSchedulingIgnoredDuringExecution`
2. **谨慎使用硬策略**：`requiredDuringSchedulingIgnoredDuringExecution`
3. **合理使用权重**：正确设置权重值（1-100）
4. **定期清理无用标签**：避免标签污染

### 8.2 标签管理规范
```yaml
# 推荐标签格式
app.kubernetes.io/name: frontend
app.kubernetes.io/instance: frontend-abcd
app.kubernetes.io/version: "1.0.0"
app.kubernetes.io/component: webserver
app.kubernetes.io/part-of: web-application
app.kubernetes.io/managed-by: helm

# 环境标签
environment: production
tier: backend
release: stable
```

### 8.3 性能优化建议
1. **节点选择器先行**：先用 `nodeSelector` 过滤大部分节点
2. **简化表达式**：避免过于复杂的匹配条件
3. **限制拓扑域范围**：选择合适的拓扑键
4. **监控调度延迟**：关注调度器性能指标

## 9. 完整示例模板

### 9.1 生产环境Web应用
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-production
  labels:
    app.kubernetes.io/name: web
    app.kubernetes.io/instance: web-production
    app.kubernetes.io/version: "2.1.0"
spec:
  replicas: 5
  selector:
    matchLabels:
      app.kubernetes.io/name: web
      app.kubernetes.io/instance: web-production
  template:
    metadata:
      labels:
        app.kubernetes.io/name: web
        app.kubernetes.io/instance: web-production
        environment: production
    spec:
      nodeSelector:
        environment: production
        node-type: general
      affinity:
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 80
            preference:
              matchExpressions:
              - key: topology.kubernetes.io/zone
                operator: In
                values:
                - us-west-2a
                - us-west-2b
          - weight: 20
            preference:
              matchExpressions:
              - key: instance-type
                operator: In
                values:
                - m5.large
                - m5.xlarge
        
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app.kubernetes.io/name
                operator: In
                values:
                - web
            topologyKey: kubernetes.io/hostname
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
              - key: environment
                operator: In
                values:
                - production
              topologyKey: topology.kubernetes.io/zone
      
      tolerations:
      - key: "dedicated"
        operator: "Equal"
        value: "web-team"
        effect: "NoSchedule"
      - key: "node.kubernetes.io/not-ready"
        operator: "Exists"
        effect: "NoExecute"
        tolerationSeconds: 300
      - key: "node.kubernetes.io/unreachable"
        operator: "Exists"
        effect: "NoExecute"
        tolerationSeconds: 300
```

这份详细指南涵盖了 Kubernetes 亲和性调度的所有核心概念和实战技巧，从基础用法到高级配置，从简单示例到复杂场景，帮助你全面掌握 Pod 调度的高级特性。