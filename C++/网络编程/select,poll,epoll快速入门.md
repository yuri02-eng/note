# Select 与 Poll 详解笔记

## 一、基本概念
**I/O多路复用**：允许单个线程监控多个文件描述符（fd），当任一fd就绪时通知程序。

## 二、Select 机制

### 1. 核心特性
```c
int select(int nfds, fd_set *readfds, fd_set *writefds, 
           fd_set *exceptfds, struct timeval *timeout);
```
- 使用**位图**表示fd集合（fd_set），每个fd对应一个bit
- 默认最大fd数为1024（FD_SETSIZE）
- 参数为**输入-输出共用**，每次调用后被内核修改

### 2. 工作流程
1. 用户：设置位图，指定要监控的fd
2. 内核：遍历0~nfds-1所有fd，检查状态
3. 内核：修改位图，只保留就绪的fd
4. 用户：必须重置位图才能再次调用

### 3. 关键问题
- **必须每次重置**：因为内核会修改传入的fd_set
- **效率问题**：内核需遍历0~最大fd的所有位，即使大部分fd不被监控
- **1024限制**：编译时确定，无法修改

## 三、Poll 机制

### 1. 核心特性
```c
int poll(struct pollfd *fds, nfds_t nfds, int timeout);

struct pollfd {
    int fd;        // 文件描述符
    short events;  // 要监控的事件（输入）
    short revents; // 已发生的事件（输出）
};
```
- 使用**结构体数组**，每个fd一个pollfd结构
- 无理论最大fd数限制
- 输入(events)与输出(revents)**分离**

### 2. 工作流程
1. 用户：初始化pollfd数组，设置events字段
2. 内核：只遍历数组中指定的fd
3. 内核：设置revents字段，不修改events
4. 用户：下次调用无需重置events字段

### 3. 核心优势
- **无需重置events**：内核不会修改输入参数
- **只检查有效fd**：内核只遍历用户传入的数组
- **无数量限制**：数组大小由用户决定

## 四、关键差异对比

| 对比维度 | Select | Poll |
|---------|--------|------|
| **数据结构** | 固定大小位图 | 动态结构体数组 |
| **最大fd数** | 1024（通常） | 无限制 |
| **输入输出** | 共用参数，每次被修改 | events(输入)与revents(输出)分离 |
| **内核遍历** | 0~最大fd的所有位 | 只遍历数组中的fd |
| **重置需求** | 必须每次重置 | events不需重置 |
| **效率特征** | fd稀疏时效率低 | 始终高效 |
| **可移植性** | 所有平台 | 主要Unix-like系统 |

## 五、性能分析

### 遍历效率对比
```
监控fd: 3, 100, 1000（共3个）

Select:
  - 内核遍历: 0~1000 → 1001次检查
  - 用户遍历: 同样1001次
  - 效率: 3/1001 ≈ 0.3%

Poll:
  - 内核遍历: 只检查3个fd
  - 用户遍历: 只检查3个
  - 效率: 100%
```

### 重置开销
```c
// Select必须
FD_ZERO(&set);           // 清空位图
for(每个fd) FD_SET(fd, &set);  // 重新设置

// Poll只需（可选）
for(每个fd) fds[i].revents = 0;  // 简单赋值
```

## 六、使用建议

### 选Select的场景
- 需要最大兼容性（包括Windows）
- 监控的fd < 1024且值较小
- 已有代码基于select，重构成本高

### 选Poll的场景
- fd可能超过1024
- fd值大但数量少（稀疏）
- 需要更简洁的API
- 仅需支持Unix-like系统

### 现代替代方案
- **Linux**: epoll - 事件驱动，O(1)复杂度
- **BSD/macOS**: kqueue - 功能丰富
- **Windows**: IOCP - 真正的异步I/O

## 七、常见陷阱

### Select陷阱
1. **忘记重置fd_set**（最常见错误）
2. **fd值超过1024**（越界，未定义行为）
3. **timeout被修改**（需注意timeval可能被内核修改）

### Poll陷阱
1. **忽略负fd**（poll会立即返回POLLNVAL）
2. **events字段被错误覆盖**（应用 |= 而非 =）
3. **数组过大导致复制开销**

## 八、总结

**核心记忆点**：
1. Select使用**位图**，Poll使用**结构体数组**
2. Select有**1024限制**，Poll**无限制**
3. Select需**每次重置**，Poll**events不需重置**
4. Select内核遍历**所有可能fd**，Poll只遍历**传入的fd**

**演进趋势**：select → poll → epoll/kqueue → io_uring，每一代都在解决前代的性能瓶颈。

**实际应用**：现代高并发服务器多使用epoll/kqueue/IOCP，但理解select/poll是掌握I/O多路复用的基础。