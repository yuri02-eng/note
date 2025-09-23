好的，这是一份系统且详细的 Vue 3 学习笔记。它从核心概念到高级特性，旨在为你提供一个清晰的学习路径和参考指南。

---

# Vue 3 系统学习笔记

## 一、 Vue 3 简介与设计理念

### 1.1 主要新特性
- **组合式 API (Composition API)**： 解决 Options API 在复杂组件中逻辑关注点分离的问题，提供更好的逻辑复用和代码组织。
- **性能提升**：
    - 使用 `Proxy` 重写响应式系统，带来性能提升并消除 Vue 2 中的一些限制（如无法检测新属性的添加、数组索引修改等）。
    - 编译时优化：静态树提升、静态属性提升、Diff 算法优化等。
- **更好的 TypeScript 支持**： 代码库使用 TypeScript 重写，提供更好的类型推断。
- **新的内置组件**： 如 `<Fragment>`, `<Teleport>`, `<Suspense>` 等。
- **Tree-Shaking 支持**： 更多 API 可以按需引入，减小项目体积。

### 1.2 构建工具：Vite
Vue 官方推荐的构建工具，提供极快的冷启动和模块热更新（HMR）。
```bash
npm create vite@latest my-vue-app -- --template vue
cd my-vue-app
npm install
npm run dev
```

---

## 二、 基础核心概念

### 2.1 应用实例 (Application Instance)
```javascript
import { createApp } from 'vue'
import App from './App.vue'

const app = createApp(App)

// 注册全局组件、指令、插件等
// app.component('ComponentName', Component)
// app.directive('directive-name', directive)
// app.use(plugin)

app.mount('#app') // 挂载到 #app 元素
```

### 2.2 模板语法 (Template Syntax)
- **文本插值**： `{{ message }}`
- **原始 HTML**： `v-html="rawHtml"`
- **属性绑定**： `v-bind:id="dynamicId"` 或 `:id="dynamicId"`
- **JavaScript 表达式**： `{{ number + 1 }}`, `{{ ok ? 'YES' : 'NO' }}`

### 2.3 响应式基础：`ref()` 和 `reactive()`
Vue 3 使用 `ref` 和 `reactive` 来声明响应式数据。

- **`ref()`**: 处理基本数据类型。它接收一个内部值，返回一个响应式的、可更改的 ref 对象，通过 `.value` 属性访问其值。在模板中会自动“解包”。
    ```vue
    <script setup>
    import { ref } from 'vue'
    
    const count = ref(0)
    const increment = () => {
      count.value++ // 在 JS 中需要通过 .value 访问
    }
    </script>
    
    <template>
      <button @click="increment">{{ count }}</button> <!-- 模板中无需 .value -->
    </template>
    ```

- **`reactive()`**: 处理对象类型。返回一个对象的响应式代理。
    ```vue
    <script setup>
    import { reactive } from 'vue'
    
    const state = reactive({ count: 0 })
    const increment = () => {
      state.count++ // 直接访问属性
    }
    </script>
    
    <template>
      <button @click="increment">{{ state.count }}</button>
    </template>
    ```
> **重要区别**：
> - 使用 `reactive()` 的对象，不能使用**解构赋值**，否则会失去响应性。需要使用 `toRefs()` 将其转换为多个 ref。
>   ```javascript
>   import { reactive, toRefs } from 'vue'
>   const state = reactive({ foo: 1, bar: 2 })
>   const { foo, bar } = toRefs(state) // 现在 foo 和 bar 是响应式的 ref
>   ```
> - 优先使用 `ref()`，除非你确定需要 `reactive()` 的便利性。

### 2.4 计算属性 (Computed) 和 监听器 (Watch)
- **`computed()`**: 依赖于其他状态的状态。它会自动追踪其依赖，并在依赖变更时重新计算。
    ```vue
    <script setup>
    import { ref, computed } from 'vue'
    
    const count = ref(0)
    const doubleCount = computed(() => count.value * 2)
    </script>
    ```

- **`watch()`**: 在状态改变时执行“副作用”。
    ```vue
    <script setup>
    import { ref, watch } from 'vue'
    
    const count = ref(0)
    
    // 监听一个 ref
    watch(count, (newValue, oldValue) => {
      console.log(`count changed from ${oldValue} to ${newValue}`)
    })
    
    // 监听多个源
    watch([fooRef, barRef], ([newFoo, newBar], [oldFoo, oldBar]) => {
      // ...
    })
    </script>
    ```
- **`watchEffect()`**: 立即运行一个函数，并自动追踪其依赖，依赖变更时重新执行。
    ```javascript
    watchEffect(() => {
      console.log('count is:', count.value) // 会自动追踪 count
    })
    ```

### 2.5 条件渲染与列表渲染
- **`v-if` vs `v-show`**: `v-if` 是真正的条件渲染，会销毁/重建组件；`v-show` 只是切换 CSS `display` 属性。
- **`v-for`**: 遍历数组或对象。**必须指定唯一的 `key` attribute**。
    ```vue
    <ul>
      <li v-for="item in items" :key="item.id">
        {{ item.message }}
      </li>
    </ul>
    ```

### 2.6 事件处理 (Event Handling)
- **`v-on:click`** 或 **`@click`**: 监听 DOM 事件。
- **内联事件处理器**： `@click="count++"`
- **方法事件处理器**： `@click="increment"`
- **事件修饰符**： `.stop`, `.prevent`, `.capture`, `.self`, `.once`, `.passive`
    ```vue
    <form @submit.prevent="onSubmit">...</form>
    <a @click.stop.prevent="doThat">...</a>
    ```
- **按键修饰符**： `.enter`, `.tab`, `.esc`, `.up`, `.down`
    ```vue
    <input @keyup.enter="submit" />
    ```

### 2.7 表单输入绑定 (Form Input Bindings)
**`v-model`**: 在表单元素上创建双向数据绑定。
- **文本**： `<input v-model="text" />`
- **复选框**： `<input type="checkbox" v-model="checked" />`
- **单选按钮**： `<input type="radio" v-model="picked" value="One" />`
- **下拉框**：
    ```vue
    <select v-model="selected">
      <option disabled value="">Please select one</option>
      <option>A</option>
      <option>B</option>
    </select>
    ```
- **修饰符**： `.lazy` (监听 `change` 而非 `input` 事件), `.number` (自动转为数字), `.trim` (自动去除首尾空格)

---

## 三、 组件 (Components)

### 3.1 组件基础
- **Props**: 父组件向子组件传递数据。
    ```vue
    <!-- 子组件 ChildComp.vue -->
    <script setup>
    defineProps({
      title: String,
      likes: Number
    })
    </script>
    <template>
      <h2>{{ title }}</h2>
      <p>Likes: {{ likes }}</p>
    </template>
    
    <!-- 父组件 -->
    <ChildComp title="Hello from Parent!" :likes="42" />
    ```
- **Emits**: 子组件向父组件触发事件。
    ```vue
    <!-- 子组件 ChildComp.vue -->
    <script setup>
    const emit = defineEmits(['enlarge-text'])
    
    const buttonClick = () => {
      emit('enlarge-text', 2) // 触发事件，并传递参数
    }
    </script>
    <template>
      <button @click="buttonClick">Enlarge Text</button>
    </template>
    
    <!-- 父组件 -->
    <ChildComp @enlarge-text="(n) => fontSize += n" />
    ```
- **Slots**: 父组件向子组件注入内容。
    ```vue
    <!-- 子组件 FancyButton.vue -->
    <template>
      <button class="fancy-btn">
        <slot></slot> <!-- 插槽出口，内容会被渲染在这里 -->
      </button>
    </template>
    
    <!-- 父组件 -->
    <FancyButton>
      Click me! <!-- 插槽内容 -->
    </FancyButton>
    ```

### 3.2 组件生命周期
Vue 3 的生命周期钩子需要从 `vue` 中导入，并写在 `setup()` 或 `<script setup>` 中。
| Vue 2 Option | Vue 3 Hook (Composition API) | 触发时机 |
| :--- | :--- | :--- |
| `beforeCreate` | `setup()` | 组件实例初始化后，props 解析之前 |
| `created` | `setup()` | 组件实例创建完成，props 解析完毕 |
| `beforeMount` | `onBeforeMount` | 组件挂载到 DOM 之前 |
| `mounted` | `onMounted` | 组件挂载到 DOM 之后 |
| `beforeUpdate` | `onBeforeUpdate` | 响应式数据变更，DOM 更新之前 |
| `updated` | `onUpdated` | 响应式数据变更，DOM 更新之后 |
| `beforeUnmount` | `onBeforeUnmount` | 组件实例卸载之前 |
| `unmounted` | `onUnmounted` | 组件实例卸载之后 |
| `errorCaptured` | `onErrorCaptured` | 捕获到来自后代组件的错误时 |

```vue
<script setup>
import { onMounted, onUnmounted } from 'vue'

onMounted(() => {
  console.log('组件已挂载')
  // 在这里可以操作 DOM 或发起网络请求
})

onUnmounted(() => {
  console.log('组件即将卸载')
  // 在这里清理定时器、取消事件监听等
})
</script>
```

---

## 四、 组合式 API (Composition API) 深入

### 4.1 `setup()` 函数与 `<script setup>`
- **`setup()` 函数**： 是组合式 API 的入口。它接收 `props` 和 `context` 参数。**必须返回一个对象**，该对象上的属性才能在模板中使用。
    ```javascript
    export default {
      props: ['title'],
      setup(props, context) {
        console.log(props.title)
        // context 包含 attrs, slots, emit 等属性
        const count = ref(0)
        return { count } // 返回的对象会暴露给模板
      }
    }
    ```
- **`<script setup>` 语法糖**： 编译时语法糖，极大简化了组合式 API 的使用。**推荐使用**。
    - 顶层的绑定（变量、函数、import）自动暴露给模板。
    - 使用 `defineProps` 和 `defineEmits` 来声明 props 和 emits。
    - 使用 `useAttrs()` 和 `useSlots()` 来获取 attrs 和 slots。

### 4.2 逻辑复用：自定义组合式函数 (Composables)
**核心优势**： 将可复用的逻辑提取为函数。
```javascript
// composables/useMouse.js
import { ref, onMounted, onUnmounted } from 'vue'

export function useMouse() {
  const x = ref(0)
  const y = ref(0)

  function update(event) {
    x.value = event.pageX
    y.value = event.pageY
  }

  onMounted(() => window.addEventListener('mousemove', update))
  onUnmounted(() => window.removeEventListener('mousemove', update))

  return { x, y } // 返回响应式状态
}
```
```vue
<!-- 在组件中使用 -->
<script setup>
import { useMouse } from './composables/useMouse'

const { x, y } = useMouse()
</script>

<template>
  Mouse position is at: {{ x }}, {{ y }}
</template>
```

---

## 五、 高级特性和内置组件

### 5.1 Teleport
将组件的一部分模板“传送”到 DOM 中的其他位置（如 body 末尾），常用于模态框、弹窗、提示框等。
```vue
<template>
  <button @click="open = true">Open Modal</button>
  <Teleport to="body"> <!-- 将内容传送到 body 标签内 -->
    <div v-if="open" class="modal">
      <p>Hello from the modal!</p>
      <button @click="open = false">Close</button>
    </div>
  </Teleport>
</template>
```

### 5.2 Suspense (实验性)
用于协调异步依赖的组件树（如异步组件），可以在等待时显示加载状态。
```vue
<!-- 父组件 -->
<Suspense>
  <template #default>
    <AsyncComponent /> <!-- 一个异步组件 -->
  </template>
  <template #fallback>
    <div>Loading...</div> <!-- 加载时显示的内容 -->
  </template>
</Suspense>
```

### 5.3 片段 (Fragments)
Vue 3 组件支持多个根节点，无需再用一个 div 包裹。
```vue
<template>
  <header>...</header>
  <main>...</main>
  <footer>...</footer>
</template>
```

### 5.4 自定义指令 (Custom Directives)
创建自己的指令来对普通 DOM 元素进行底层操作。
```javascript
// 全局指令：使元素自动获得焦点
app.directive('focus', {
  mounted(el) {
    el.focus()
  }
})
// 局部指令 (在 <script setup> 中)
const vFocus = {
  mounted: (el) => el.focus()
}
```
```vue
<input v-focus />
```

---

## 六、 生态与状态管理

### 6.1 路由：Vue Router 4
```bash
npm install vue-router@4
```
```javascript
// router/index.js
import { createRouter, createWebHistory } from 'vue-router'
import Home from '../views/Home.vue'

const routes = [
  { path: '/', component: Home },
  { path: '/about', component: () => import('../views/About.vue') } // 懒加载
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

export default router
```
在组件中使用：
```vue
<script setup>
import { useRouter, useRoute } from 'vue-router'

const router = useRouter() // 用于编程式导航 router.push(...)
const route = useRoute()   // 用于获取当前路由信息 route.params...
</script>
<template>
  <router-link to="/">Home</router-link>
  <router-view></router-view>
</template>
```

### 6.2 状态管理：Pinia (推荐)
Vue 官方新一代状态管理库，替代 Vuex。
```bash
npm install pinia
```
```javascript
// stores/counter.js
import { defineStore } from 'pinia'

export const useCounterStore = defineStore('counter', {
  state: () => ({ count: 0 }),
  getters: {
    doubleCount: (state) => state.count * 2
  },
  actions: {
    increment() {
      this.count++
    }
  }
})
```
在组件中使用：
```vue
<script setup>
import { useCounterStore } from '@/stores/counter'

const counter = useCounterStore()
</script>

<template>
  <div>{{ counter.count }}</div>
  <div>{{ counter.doubleCount }}</div>
  <button @click="counter.increment()">+</button>
</template>
```

---

## 七、 TypeScript 集成

Vue 3 对 TypeScript 提供一流的支持。

### 7.1 为 Props 定义类型
在 `<script setup>` 中使用泛型：
```vue
<script setup lang="ts">
interface Props {
  title: string
  likes?: number // 可选属性
}

const props = defineProps<Props>()
</script>
```
或使用运行时声明：
```vue
<script setup lang="ts">
defineProps({
  title: { type: String, required: true },
  likes: { type: Number, default: 0 }
})
</script>
```

### 7.2 为 Emits 定义类型
```vue
<script setup lang="ts">
const emit = defineEmits<{
  (e: 'change', id: number): void
  (e: 'update', value: string): void
}>()
</script>
```

---

## 八、 最佳实践与总结

1.  **优先使用 `<script setup>`**： 语法更简洁，类型推导更好。
2.  **合理使用组合式函数 (Composables)**： 将复杂逻辑抽离，提高代码可读性和可维护性。
3.  **正确的响应式用法**：
    - 使用 `ref()` 处理基本类型，使用 `reactive()` 处理对象。
    - 解构 `reactive()` 对象时，使用 `toRefs()` 保持响应性。
4.  **性能考量**：
    - 使用 `v-if` 和 `v-for` 时，避免在同一元素上使用（`v-for` 优先级更高）。
    - 对于大型列表，考虑使用虚拟滚动（如 `vue-virtual-scroller`）。
    - 使用 `computed` 和 `watch` 时，避免产生不必要的副作用或重复计算。
5.  **状态管理选择**： 简单应用可以用 `reactive` 创建全局状态，复杂应用强烈推荐 **Pinia**。

这份笔记涵盖了 Vue 3 的核心概念和大部分常用特性。要真正掌握，请务必结合官方文档和实际项目进行练习。