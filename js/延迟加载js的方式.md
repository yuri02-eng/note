好的，这是关于 JavaScript 延迟加载的整理笔记。

# JavaScript 延迟加载 (Lazy Loading) 笔记

## 1. 核心概念

**延迟加载**：一种优化网页性能的策略，指不在页面加载初期就加载所有 JavaScript 文件，而是推迟到**真正需要它的时候**再加载。

**核心目标**：减少初始页面加载时间（特别是**阻塞时间**），降低初始网络请求数量和数据量，从而提升用户体验（如更快的首屏加载速度、交互准备就绪时间）。

---

## 2. 为什么需要延迟加载 JS？

### 阻塞性 (Render-Blocking)
浏览器解析 HTML 构建 DOM 和 CSSOM 时，如果遇到普通的 `<script>` 标签（无 `defer` 或 `async`），它会：
1.  **停止文档解析**（停止构建 DOM）。
2.  **下载**脚本文件。
3.  **执行**脚本。
4.  执行完毕后，**继续解析** HTML 文档。

这种阻塞行为会显著拖慢页面的首次渲染速度。

### 解决方案
延迟加载将非关键的、不影响首屏内容的脚本（如：第三方分析、广告、非首屏交互组件、大型库）推迟加载，让浏览器优先处理核心内容和关键脚本。

---

## 3. 主要实现方法

### 方法一：使用 `async` 和 `defer` 属性 (推荐)
这两个属性都用于**外部脚本** (`<script src="...">`)，告诉浏览器不要边下载边阻塞解析。

| 特性         | `<script>` (默认)         | `<script async>`                          | `<script defer>`                          |
| :----------- | :------------------------ | :---------------------------------------- | :---------------------------------------- |
| **执行顺序** | 在文档中的顺序执行        | **加载顺序优先**。谁先下载完谁先执行，不保证顺序 | **保持文档中的顺序**执行                      |
| **DOMContentLoaded** | 在 `DOMContentLoaded` 事件**之前**执行 | 在 `DOMContentLoaded` 事件**前后都可能**执行 | 在 `DOMContentLoaded` 事件**之前**执行（按顺序） |
| **适用场景** | 关键、依赖 DOM 的脚本     | 完全独立、不依赖其他脚本的第三方库（如 analytics） | 非关键且可能相互依赖的脚本                  |

**图解关系**：
（想象一个 HTML 文档解析时间线）
*   **默认 `<script>`**：解析 -> 遇到脚本 -> **停止解析** -> 下载 -> 执行 -> **继续解析**。
*   **`async`**：解析 -> 遇到脚本 -> **异步下载** (不阻塞) -> 下载完成 -> **立即执行** (此时可能阻塞解析) -> (继续解析)。
*   **`defer`**：解析 -> 遇到脚本 -> **异步下载** (不阻塞) -> 整个文档解析完毕 -> **按顺序执行** -> 触发 `DOMContentLoaded`。

### 方法二：动态脚本注入 (Dynamic Script Injection)
通过 JavaScript 代码在**运行时**创建并插入 `<script>` 标签。

```javascript
function loadScript(src, onloadCallback = null) {
  const script = document.createElement('script');
  script.src = src;
  script.async = false; // 通常保持默认的 async=true 行为，设为 false 则不异步
  if (onloadCallback) {
    script.onload = onloadCallback;
    // 也可以处理 onerror
  }
  document.body.appendChild(script); // 添加到文档后开始加载
}

// 使用示例
loadScript('path/to/your-script.js', function() {
  console.log('Script loaded!');
  // 可以在这里调用刚加载脚本里的函数
});
```

**特点**：
*   完全由开发者控制加载时机（例如在点击事件、滚动到特定位置、某个条件满足后触发）。
*   天然是**异步**的（类似于 `async` 行为）。
*   如果需要保证多个动态脚本的**执行顺序**，需要复杂的回调或 Promise 链来管理。

### 方法三：利用 `Intersection Observer API` (针对基于视口的加载)
非常适合用于延迟加载那些只有当用户**滚动到附近**才需要的内容所关联的脚本（如图片懒加载库、评论区组件、动画库）。

```javascript
// 1. 观察一个占位符元素（比如一个空的div）
const target = document.querySelector('#lazy-component-trigger');

const observer = new IntersectionObserver((entries, observer) => {
  entries.forEach(entry => {
    if (entry.isIntersecting) {
      // 当占位元素进入视口
      loadScript('path/to/lazy-component.js'); // 加载所需脚本
      observer.unobserve(entry.target); // 加载后停止观察
    }
  });
}, { rootMargin: '50px' }); // 提前50px触发加载

observer.observe(target);
```

---

## 4. 如何选择？

| 场景                                                                 | 推荐方法                                   |
| :------------------------------------------------------------------- | :----------------------------------------- |
| **独立的第三方脚本** (分析、广告)                                      | `async`                                    |
| **有依赖关系且非关键的后台脚本**                                       | `defer`                                    |
| **用户交互后才会需要的功能** (弹窗、复杂图表、选项卡)                     | **动态注入**（在点击/触摸事件中触发）            |
| **位于页面底部或需要滚动才能看到的内容** (评论、社交媒体插件、无限滚动) | **动态注入** + **`Intersection Observer`** |

---

## 5. 注意事项与最佳实践

1.  **关键路径脚本不要延迟**：对于渲染首屏内容、实现核心交互（如菜单、按钮）所**必需**的脚本，应使用普通 `<script>` 或内联，并尽量精简（Code Splitting）。
2.  **注意依赖关系**：`async` 脚本之间不能有依赖，因为执行顺序不确定。有依赖的脚本使用 `defer` 或动态加载回调。
3.  **优雅降级/加载指示**：如果延迟加载的组件很重要（如评论区），可以考虑显示一个“加载中”的占位符，提升用户体验。
4.  **与模块打包器结合**：现代前端工具（Webpack, Rollup, Vite）支持**代码分割 (Code Splitting)** 和**动态导入 (`import()`)**，它们底层会自动处理脚本的延迟加载和依赖。
    ```javascript
    // 动态导入返回一个 Promise
    button.addEventListener('click', () => {
      import('./large-module.js')
        .then((module) => {
          module.someFunction();
        })
        .catch((err) => {
          console.error('Module loading failed:', err);
        });
    });
    ```
5.  **性能衡量**：使用 Lighthouse、WebPageTest 等工具测量延迟加载前后的性能变化（如 Speed Index, Time to Interactive）。

---

## 总结

| 方法                     | 控制力 | 顺序保证 | 实现难度 | 典型用例                                 |
| :----------------------- | :----- | :------- | :------- | :--------------------------------------- |
| **`async` / `defer`**    | 低     | 低/高    | 非常简单 | 初始加载的非关键脚本                     |
| **动态注入**             | 高     | 低       | 中等     | 用户交互或条件触发后的功能               |
| **`IntersectionObserver`** | 高     | 低       | 中等     | 基于滚动位置加载的内容和功能             |
| **打包器动态导入**       | 高     | 高       | 中等     | 现代SPA应用，按路由或组件拆分代码        |

**核心思想**：将资源的加载与执行时机与用户的实际需求对齐，避免不必要的网络请求和处理开销。