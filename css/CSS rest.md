# CSS Reset 笔记总结

## 一、什么是 CSS Reset？

CSS Reset（样式重置）是一种**消除浏览器默认样式差异**的技术。它通过一套预设的 CSS 规则，将不同浏览器的默认样式重置为一致的基础状态。

### 核心目的：
1. **解决浏览器样式差异**：不同浏览器对 HTML 元素的默认样式不同（如边距、字体大小等）

2. **提供一致的起点**：让开发者从"零"开始构建样式，避免默认样式干扰

3. **提高跨浏览器一致性**：确保页面在不同浏览器中呈现效果一致

    [Normalize.css](https://necolas.github.io/normalize.css/)：为了增强跨浏览器渲染的一致性，一个CSS 重置样式库。

## 二、为什么需要 CSS Reset？

| 问题 | 示例 | 影响 |
|------|------|------|
| 默认边距不同 | `<body>`、`<p>`、`<h1>`等元素的默认边距 | 布局不一致 |
| 字体样式差异 | 字体大小、行高、字体系列不同 | 排版不一致 |
| 列表样式差异 | `<ul>`、`<ol>`的缩进和标记样式 | 列表显示不一致 |
| 表单元素差异 | `<input>`、`<select>`、`<button>`样式 | 表单外观不一致 |
| 盒模型差异 | 某些浏览器盒模型计算方式不同 | 尺寸计算不一致 |

## 三、主流 CSS Reset 方案

### 1. Eric Meyer's Reset CSS（经典重置）
```css
/* http://meyerweb.com/eric/tools/css/reset/ 
   v2.0 | 20110126
   License: none (public domain)
*/

html, body, div, span, applet, object, iframe,
h1, h2, h3, h4, h5, h6, p, blockquote, pre,
a, abbr, acronym, address, big, cite, code,
del, dfn, em, img, ins, kbd, q, s, samp,
small, strike, strong, sub, sup, tt, var,
b, u, i, center,
dl, dt, dd, ol, ul, li,
fieldset, form, label, legend,
table, caption, tbody, tfoot, thead, tr, th, td,
article, aside, canvas, details, embed, 
figure, figcaption, footer, header, hgroup, 
menu, nav, output, ruby, section, summary,
time, mark, audio, video {
    margin: 0;
    padding: 0;
    border: 0;
    font-size: 100%;
    font: inherit;
    vertical-align: baseline;
}
/* HTML5 display-role reset for older browsers */
article, aside, details, figcaption, figure, 
footer, header, hgroup, menu, nav, section {
    display: block;
}
body {
    line-height: 1;
}
ol, ul {
    list-style: none;
}
blockquote, q {
    quotes: none;
}
blockquote:before, blockquote:after,
q:before, q:after {
    content: '';
    content: none;
}
table {
    border-collapse: collapse;
    border-spacing: 0;
}
```

**特点**：
- 激进的重置方式
- 将所有元素的边距、内边距设为0
- 移除列表样式、引号样式等
- 需要开发者重新定义所有样式

### 2. Normalize.css（现代主流方案）
```css
/*! normalize.css v8.0.1 | MIT License | github.com/necolas/normalize.css */

/* Document
   ========================================================================== */

/**
 * 1. Correct the line height in all browsers.
 * 2. Prevent adjustments of font size after orientation changes in iOS.
 */
html {
  line-height: 1.15; /* 1 */
  -webkit-text-size-adjust: 100%; /* 2 */
}

/* Sections
   ========================================================================== */

/**
 * Remove the margin in all browsers.
 */
body {
  margin: 0;
}

/**
 * Render the `main` element consistently in IE.
 */
main {
  display: block;
}

/* ... 其他元素标准化规则 ... */
```

**特点**：
- 不是重置，而是"标准化"
- 保留有用的浏览器默认样式
- 修复浏览器的不一致问题
- 模块化结构，易于扩展
- 现代项目推荐使用

### 3. 自定义 CSS Reset（推荐）
```css
/* ===== 现代CSS重置 ===== */

/* 1. 使用更直观的盒模型 */
*, *::before, *::after {
  box-sizing: border-box;
}

/* 2. 移除默认边距 */
body, h1, h2, h3, h4, h5, h6, p, figure, blockquote, dl, dd {
  margin: 0;
}

/* 3. 设置核心默认值 */
body {
  min-height: 100vh;
  line-height: 1.5;
  text-rendering: optimizeSpeed;
}

/* 4. 移除列表样式 */
ul, ol {
  list-style: none;
  padding: 0;
  margin: 0;
}

/* 5. 媒体元素更易使用 */
img, picture, video, canvas, svg {
  display: block;
  max-width: 100%;
}

/* 6. 继承表单元素的字体 */
input, button, textarea, select {
  font: inherit;
}

/* 7. 避免文本溢出 */
p, h1, h2, h3, h4, h5, h6 {
  overflow-wrap: break-word;
}

/* 8. 减少运动偏好 */
@media (prefers-reduced-motion: reduce) {
  *,
  *::before,
  *::after {
    animation-duration: 0.01ms !important;
    animation-iteration-count: 1 !important;
    transition-duration: 0.01ms !important;
    scroll-behavior: auto !important;
  }
}
```

## 四、CSS Reset 核心内容

### 1. 盒模型重置
```css
*, *::before, *::after {
  box-sizing: border-box;
}
```
- 统一使用 `border-box` 盒模型
- 更直观的尺寸计算方式

### 2. 边距重置
```css
body, h1, h2, h3, h4, h5, h6, p, figure, blockquote, dl, dd {
  margin: 0;
}

ul, ol {
  padding: 0;
  margin: 0;
}
```
- 移除常见元素的默认边距
- 提供一致的起点

### 3. 列表样式重置
```css
ul, ol {
  list-style: none;
}
```
- 移除列表的项目符号
- 需要时再自定义样式

### 4. 媒体元素处理
```css
img, picture, video, canvas, svg {
  display: block;
  max-width: 100%;
  height: auto;
}
```
- 防止图片溢出容器
- 保持宽高比
- 避免行内元素下方的间隙

### 5. 表单元素重置
```css
input, button, textarea, select {
  font: inherit;
  color: inherit;
}

button {
  background: none;
  border: none;
  padding: 0;
  cursor: pointer;
}
```
- 继承字体样式
- 移除默认样式
- 统一交互体验

### 6. 排版重置
```css
body {
  line-height: 1.5;
}

h1, h2, h3, h4, h5, h6 {
  font-size: inherit;
  font-weight: inherit;
}
```
- 设置合理的默认行高
- 移除标题元素的默认样式

## 五、CSS Reset 最佳实践

1. **选择合适方案**：
   - 新项目：使用 Normalize.css + 自定义重置
   - 旧项目维护：保持原有重置方案

2. **放在CSS文件开头**：
   ```html
   <link rel="stylesheet" href="reset.css">
   <link rel="stylesheet" href="styles.css">
   ```

3. **按需定制**：
   - 根据项目需求添加/移除重置规则
   - 避免过度重置

4. **与现代CSS特性结合**：
   ```css
   /* 使用CSS变量定义主题 */
   :root {
     --color-primary: #3498db;
     --spacing-base: 1rem;
   }
   
   /* 在重置中使用 */
   body {
     color: var(--color-text);
     font-family: var(--font-primary);
   }
   ```

5. **考虑可访问性**：
   ```css
   /* 保留焦点样式 */
   :focus-visible {
     outline: 2px solid var(--color-primary);
     outline-offset: 2px;
   }
   
   /* 减少运动偏好 */
   @media (prefers-reduced-motion: reduce) {
     * {
       animation-duration: 0.01ms !important;
       transition-duration: 0.01ms !important;
     }
   }
   ```

## 六、CSS Reset vs Normalize.css

| 特性 | CSS Reset | Normalize.css |
|------|-----------|---------------|
| 目标 | 清除所有默认样式 | 保留有用的默认样式 |
| 方法 | 将所有样式归零 | 修复浏览器不一致 |
| 结果 | 从零开始构建样式 | 提供一致的基础样式 |
| 适合场景 | 高度定制化设计 | 需要保留部分默认样式 |
| 文件大小 | 通常较小 | 稍大但更全面 |
| 现代使用 | 结合两者优势 | 作为基础使用 |

## 七、现代 CSS Reset 趋势

1. **使用 CSS 变量**：
   ```css
   :root {
     --font-sans: system-ui, sans-serif;
     --color-text: #333;
     --spacing-base: 1rem;
   }
   
   body {
     font-family: var(--font-sans);
     color: var(--color-text);
     line-height: 1.6;
   }
   ```

2. **逻辑属性**：
   ```css
   body {
     margin-block: 0;
     margin-inline: auto;
     padding-inline: var(--spacing-base);
   }
   ```

3. **响应式基础**：
   ```css
   html {
     font-size: 100%;
   }
   
   @media (min-width: 768px) {
     html {
       font-size: 112.5%;
     }
   }
   ```

4. **暗黑模式支持**：
   ```css
   :root {
     --color-bg: #fff;
     --color-text: #333;
   }
   
   @media (prefers-color-scheme: dark) {
     :root {
       --color-bg: #121212;
       --color-text: #f0f0f0;
     }
   }
   
   body {
     background-color: var(--color-bg);
     color: var(--color-text);
   }
   ```

## 八、总结

CSS Reset 是现代前端开发的基石，它解决了浏览器默认样式的差异问题。根据项目需求选择合适的重置方案：

1. **经典重置**：完全清除默认样式，适合高度定制项目
2. **Normalize.css**：修复浏览器不一致，保留有用默认样式
3. **自定义重置**：结合项目需求，创建最适合的解决方案

现代最佳实践：
- 使用 `box-sizing: border-box`
- 设置合理的默认行高和字体
- 处理媒体元素的自适应
- 使用 CSS 变量增强灵活性
- 考虑可访问性和响应式设计

```css
/* 现代CSS重置示例 */
:root {
  --font-sans: system-ui, sans-serif;
  --color-text: #333;
  --color-bg: #fff;
  --spacing-base: 1rem;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}

body {
  margin: 0;
  min-height: 100vh;
  font-family: var(--font-sans);
  line-height: 1.6;
  color: var(--color-text);
  background-color: var(--color-bg);
}

img, picture, video, canvas, svg {
  display: block;
  max-width: 100%;
}

input, button, textarea, select {
  font: inherit;
}

@media (prefers-reduced-motion: reduce) {
  *,
  *::before,
  *::after {
    animation-duration: 0.01ms !important;
    transition-duration: 0.01ms !important;
  }
}
```