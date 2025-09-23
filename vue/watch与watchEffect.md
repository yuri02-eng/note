### Vue 3 响应式侦听：`watch` vs `watchEffect` 核心笔记

#### **一、核心区别总结**
| **特性**         | **`watch`**                          | **`watchEffect`**                      |
|-------------------|--------------------------------------|----------------------------------------|
| **侦听目标**      | 显式指定特定响应式源                 | 自动追踪回调函数内的响应式依赖         |
| **执行时机**      | 默认惰性（首次不执行）               | 立即执行（首次自动运行）               |
| **依赖收集**      | 需手动声明侦听源                     | 自动收集依赖（类似计算属性）           |
| **旧值获取**      | 可获取变化前的值（`(newVal, oldVal)`）| 无法获取旧值                           |
| **适用场景**      | 精确控制侦听源；需要旧值对比         | 依赖关系复杂；无需旧值的副作用操作     |

---

#### **二、详细对比解析**
**1. 依赖侦听方式**
```javascript
// watch：显式声明依赖
watch([refA, () => obj.key], ([a, key], [oldA, oldKey]) => {
  /* 逻辑 */
})

// watchEffect：自动收集依赖
watchEffect(() => {
  console.log(refA.value, obj.key) // 自动追踪 refA 和 obj.key
})
```

**2. 执行时机控制**
```javascript
// watch：默认惰性（可配置 immediate）
watch(source, callback, { immediate: true })

// watchEffect：立即执行（可通过 onTrigger 调试）
watchEffect(callback, {
  flush: 'post' // 控制回调触发时机（pre|post|sync）
})
```

**3. 旧值访问能力**
```javascript
// watch 可获取旧值
watch(refCount, (newVal, oldVal) => {
  console.log(`从 ${oldVal} → 变为 ${newVal}`)
})

// watchEffect 无法获取旧值
watchEffect(() => {
  console.log(`当前值: ${refCount.value}`) // 只有最新值
})
```

**4. 停止侦听机制（通用）**
```javascript
const stop = watch(...) 或 watchEffect(...)
stop() // 手动停止侦听
```

---

#### **三、最佳实践场景**
**✅ 使用 `watch` 当：**
- 需要对比新旧值（如表单修改前后差异）
- 需惰性执行（如路由变化时加载数据）
- 侦听特定数据组合（如 `[userId, projectId]`）
```javascript
watch(
  () => route.params.id,
  (newId) => fetchData(newId)
)
```

**✅ 使用 `watchEffect` 当：**
- 执行副作用（如根据状态操作DOM/发送日志）
- 依赖项动态变化（如响应式对象的多属性联动）
- 简化代码（避免重复声明依赖）
```javascript
watchEffect(() => {
  document.title = `${unreadCount.value} 条未读消息`
})
```

---

#### **四、高级技巧**
**1. 性能优化**
```javascript
// 减少深度侦听开销
watch(() => obj, callback, { deep: false })

// 防抖控制（watchEffect 需配合外部工具）
watch(source, _.debounce(callback, 1000))
```

**2. 清理副作用**
```javascript
watchEffect((onCleanup) => {
  const timer = setInterval(doSomething, 1000)
  onCleanup(() => clearInterval(timer)) // 组件卸载时清理
})
```

**3. 调试技巧**
```javascript
watchEffect(
  () => { /* 逻辑 */ },
  {
    onTrack(e) { debugger }, // 依赖被追踪时
    onTrigger(e) { debugger } // 依赖变化时
  }
)
```

---

#### **五、总结选择策略**
| **条件**                     | **推荐**       |
|------------------------------|----------------|
| 需要旧值 / 精确控制依赖源    | ✅ `watch`     |
| 立即执行 + 自动依赖追踪      | ✅ `watchEffect` |
| 复杂对象的部分属性变化       | ✅ `watch(() => obj.prop)` |
| 多依赖联动操作（无旧值需求） | ✅ `watchEffect` |

> 💡 **黄金法则**：  
> - 需要 **"当A变化时做B"** → 选 `watch`  
> - 需要 **"在A/B/C变化时自动执行"** → 选 `watchEffect`