# Vue 中的 v-show 与 v-if：作用、原理与优先级详解

## 一、核心作用对比

| 特性         | v-show                      | v-if                          |
|--------------|----------------------------|-------------------------------|
| **核心功能** | 控制元素的显示/隐藏         | 控制元素的创建/销毁            |
| **DOM操作**  | 始终存在DOM中，只修改样式   | 条件为真时创建，为假时销毁     |
| **初始渲染** | 无论条件如何都会渲染        | 条件为假时不会渲染             |
| **切换开销** | 低（仅CSS切换）             | 高（DOM操作+生命周期触发）     |
| **适用场景** | 频繁切换的场景              | 条件变化少的场景               |
| **性能影响** | 初始渲染成本高，切换成本低  | 初始渲染成本低，切换成本高     |

## 二、实现原理剖析

### v-show 原理
```javascript
// 伪代码实现
function vShow(el, value) {
  el.style.display = value ? '' : 'none';
}
```
- **核心机制**：通过修改元素的 `display` CSS 属性
- **执行流程**：
  1. 元素始终被渲染并保留在DOM中
  2. 当条件为真时：`display: [原始值]`
  3. 当条件为假时：`display: none`
- **特点**：
  - 不触发组件生命周期钩子
  - 元素状态（如表单输入值）会被保留
  - 适合频繁切换的场景（如选项卡切换）

### v-if 原理
```javascript
// 伪代码实现
function vIf(el, value) {
  if (value && !el.__vue_component) {
    // 创建并挂载组件
    el.__vue_component = createComponent(el);
    mountComponent(el.__vue_component);
  } else if (!value && el.__vue_component) {
    // 卸载并销毁组件
    unmountComponent(el.__vue_component);
    el.__vue_component = null;
  }
}
```
- **核心机制**：条件性地创建/销毁组件实例
- **执行流程**：
  1. 条件为真时：
     - 创建组件实例
     - 触发 `beforeCreate`, `created`, `beforeMount`, `mounted` 生命周期
  2. 条件为假时：
     - 触发 `beforeUnmount`, `unmounted` 生命周期
     - 销毁组件实例及所有子组件
- **特点**：
  - 真正的条件渲染
  - 切换时触发完整的生命周期
  - 元素状态不会被保留

## 三、优先级规则

### 1. 基础优先级
- **v-if 优先级高于 v-show**
- 当两者同时存在时，v-if 的判定结果决定元素是否会被创建

### 2. 优先级验证示例
```html
<!-- 情况1：v-if为false时 -->
<div v-if="false" v-show="true">
  <!-- 此元素不会被创建，v-show不生效 -->
</div>

<!-- 情况2：v-if为true时 -->
<div v-if="true" v-show="false">
  <!-- 元素会被创建，但被v-show隐藏 -->
</div>
```

### 3. 优先级规则总结表

| 组合情况              | 结果                                  |
|-----------------------|---------------------------------------|
| `v-if="true"` + `v-show="true"` | 元素显示                              |
| `v-if="true"` + `v-show="false"` | 元素创建但隐藏（display: none）       |
| `v-if="false"` + `v-show="true"` | **元素不会被创建**（v-show不生效）    |
| `v-if="false"` + `v-show="false"` | 元素不会被创建                        |

## 四、最佳实践指南

### 何时使用 v-show
1. 需要频繁切换显示状态的元素（如：选项卡内容）
2. 需要保留元素状态的组件（如：包含输入框的表单）
3. 初始渲染成本较低的简单元素
4. 需要CSS过渡动画的场景

```html
<!-- 选项卡切换示例 -->
<div v-show="activeTab === 'profile'">
  <UserProfile />
</div>
<div v-show="activeTab === 'settings'">
  <UserSettings />
</div>
```

### 何时使用 v-if
1. 初始渲染成本较高的组件（如：包含大量子组件的复杂组件）
2. 不需要保留状态的组件（如：静态内容展示）
3. 条件很少变化的场景（如：权限控制）
4. 需要减少初始DOM节点数量的场景

```html
<!-- 权限控制示例 -->
<template v-if="user.isAdmin">
  <AdminDashboard />
</template>
<template v-else>
  <UserDashboard />
</template>
```

### 性能优化策略
1. **首屏优化**：对首屏不需要的内容使用 `v-if`
2. **频繁切换**：对需要频繁切换的内容使用 `v-show`
3. **复杂组件**：对初始化成本高的组件优先使用 `v-if`
4. **组合使用**：嵌套使用两者实现最优性能

```html
<!-- 组合优化示例 -->
<div v-if="shouldRender">
  <!-- 复杂组件 -->
  <ExpensiveComponent v-show="isVisible" />
</div>
```

## 五、高级用法与注意事项

### 1. 与 `<template>` 标签配合
```html
<template v-if="condition">
  <!-- 多个元素的条件渲染 -->
  <div>Content 1</div>
  <div>Content 2</div>
</template>
```

### 2. v-else 和 v-else-if 链式使用
```html
<div v-if="type === 'A'">A</div>
<div v-else-if="type === 'B'">B</div>
<div v-else>C</div>
```

### 3. 与 Transition 组件配合
```html
<Transition>
  <div v-if="show" key="content">...</div>
</Transition>
```

### 4. 常见误区
- **误区1**：认为 v-show 只是 visibility: hidden
  - 实际：v-show 使用 display: none，不占据布局空间
- **误区2**：在同一个元素上同时使用 v-if 和 v-for
  - 解决方案：使用 `<template>` 包裹
  ```html
  <template v-for="item in items" :key="item.id">
    <div v-if="item.isActive">{{ item.name }}</div>
  </template>
  ```
- **误区3**：过度使用 v-show 导致初始渲染性能问题
  - 解决方案：对复杂组件使用 v-if

## 六、总结

| 维度         | v-show                      | v-if                          |
|--------------|----------------------------|-------------------------------|
| **核心区别** | CSS显示切换                | DOM创建/销毁                  |
| **优先级**   | 低于v-if                   | 高于v-show                    |
| **生命周期** | 不触发                     | 触发完整生命周期              |
| **状态保留** | 保留元素状态               | 不保留状态                    |
| **初始渲染** | 总是渲染                   | 条件为真才渲染                |
| **适用场景** | 频繁切换+状态保留          | 条件稳定+性能敏感             |

**最终建议**：
- 优先考虑使用 `v-if`，除非需要频繁切换或保留状态
- 在性能敏感区域避免同时使用两者
- 对于复杂组件，使用 `v-if` 减少不必要的初始渲染
- 在需要平滑过渡的场景，结合 Transition 组件使用 `v-if`