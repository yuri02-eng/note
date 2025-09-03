# CSS引入方式全面指南

## CSS引入方式总结表格

| 引入方式 | 使用方法 | 优先级 | 维护性 | 复用性 | 性能影响 |
|---------|---------|-------|-------|-------|---------|
| **内联样式** | `<div style="color: red;">` | ★★★★★ | ★☆☆☆☆ | ☆☆☆☆☆ | 增加HTML文件大小 |
| **内部样式表** | `<style> p {color: blue;} </style>` | ★★★★☆ | ★★★☆☆ | ★☆☆☆☆ | 增加页面加载时间 |
| **外部样式表** | `<link rel="stylesheet" href="styles.css">` | ★★★☆☆ | ★★★★★ | ★★★★★ | 可缓存，性能最佳 |
| **@import** | `@import url("styles.css");` | ★★★☆☆ | ★★★★☆ | ★★★★☆ | 可能阻塞渲染 |

## 详细解析

### 1. 内联样式 (Inline Styles)
直接在HTML元素的`style`属性中编写CSS样式。

```html
<p style="color: blue; font-size: 16px; margin: 10px;">
  这是一个使用内联样式的段落
</p>
```

**特点**：
- ⚠️ **最高优先级**：覆盖其他所有样式定义
- 🚫 **维护困难**：样式分散在HTML各处，难以统一修改
- 📉 **无复用性**：样式仅应用于单个元素
- ⚡ **适用场景**：临时样式调整、动态样式（JS操作）

### 2. 内部样式表 (Internal Style Sheet)
在HTML文档的`<head>`部分使用`<style>`标签定义样式。

```html
<!DOCTYPE html>
<html>
<head>
  <style>
    body {
      font-family: 'Arial', sans-serif;
      background-color: #f8f9fa;
    }
    
    .container {
      max-width: 1200px;
      margin: 0 auto;
      padding: 20px;
    }
  </style>
</head>
<body>
  <div class="container">内容区域</div>
</body>
</html>
```

**特点**：
- 🔒 **作用域限制**：仅影响当前页面
- 🔄 **中等维护性**：比内联样式更易维护
- ⏱️ **性能影响**：增加HTML文件大小和页面加载时间
- 🎯 **适用场景**：单页应用、小型项目、快速原型

### 3. 外部样式表 (External Style Sheet)
使用`<link>`标签引入独立的CSS文件（最佳实践）。

```html
<head>
  <link rel="stylesheet" href="styles/main.css">
  <link rel="stylesheet" href="styles/theme.css">
</head>
```

**styles/main.css**:
```css
/* 重置默认样式 */
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

/* 全局排版 */
body {
  font-family: 'Segoe UI', system-ui, sans-serif;
  line-height: 1.6;
  color: #333;
}

/* 响应式布局 */
.container {
  width: 100%;
  max-width: 1200px;
  margin: 0 auto;
  padding: 0 15px;
}
```

**特点**：
- ♻️ **最佳复用性**：多个页面可共享同一CSS文件
- 🛠️ **易于维护**：样式集中管理，修改方便
- ⚡ **性能优化**：浏览器可缓存CSS文件，提高加载速度
- 📦 **模块化**：可按功能拆分多个CSS文件
- 🌐 **适用场景**：中大型项目、多页面网站、团队协作

### 4. @import方式
在CSS文件或`<style>`标签中使用`@import`引入其他CSS文件。

```html
<style>
  @import url("reset.css");
  @import url("components/buttons.css");
  
  body {
    background-color: #f0f2f5;
  }
</style>
```

**特点**：
- 🧩 **模块化**：方便组织CSS结构
- ⚠️ **渲染阻塞**：需等待父文件加载完成
- ⏳ **性能影响**：可能增加页面加载时间
- 🔗 **依赖管理**：适合组织CSS依赖关系
- 🧰 **适用场景**：CSS模块化组织、框架集成

## 优先级规则详解

CSS优先级遵循以下规则（从高到低）：
1. `!important`声明
2. 内联样式（style属性）
3. ID选择器（#id）
4. 类选择器、属性选择器、伪类（.class, [type="text"], :hover）
5. 元素选择器、伪元素（div, ::before）
6. 通配符、继承样式（*, 继承的属性）

**示例**：
```css
/* 特异性值：0,0,1,0 */
p { color: black; }

/* 特异性值：0,0,1,1 */
div p { color: blue; }

/* 特异性值：0,1,1,0 */
.container p { color: red; }

/* 特异性值：1,0,0,0 */
#main-content { color: green; }

/* 内联样式：最高优先级 */
<p style="color: purple;">文本颜色</p>
```

## 现代CSS开发实践

### 性能优化技巧
1. **关键CSS内联**：首屏关键样式内联到HTML中
2. **CSS压缩**：使用工具压缩CSS文件大小
3. **文件合并**：减少HTTP请求数量
4. **按需加载**：基于路由加载CSS模块

```html
<head>
  <!-- 关键CSS内联 -->
  <style>
    /* 首屏必要样式 */
    body { font-family: sans-serif; }
    .hero { background: #f8f9fa; }
  </style>
  
  <!-- 异步加载非关键CSS -->
  <link rel="preload" href="non-critical.css" as="style" onload="this.rel='stylesheet'">
  <noscript><link rel="stylesheet" href="non-critical.css"></noscript>
</head>
```

### CSS模块化方案
```css
/* 基础重置 */
@import "base/reset.css";

/* 工具类 */
@import "utilities/spacing.css";
@import "utilities/flex.css";

/* 组件 */
@import "components/buttons.css";
@import "components/cards.css";

/* 页面特定样式 */
@import "pages/home.css";
@import "pages/contact.css";
```

## 浏览器支持情况

所有现代浏览器均支持全部四种引入方式：
- Chrome, Firefox, Safari, Edge: 完全支持
- Internet Explorer 9+：支持所有方式（IE8部分支持）

## 总结建议

1. **首选外部样式表**：用于大多数样式定义
2. **谨慎使用内联样式**：仅用于动态样式或覆盖
3. **内部样式表适合**：单页应用或小型项目
4. **@import适用于**：CSS模块化组织
5. **性能优先**：关键CSS内联，非关键CSS异步加载

```html
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CSS引入方式最佳实践</title>
  
  <!-- 关键CSS内联 -->
  <style>
    :root {
      --primary: #4361ee;
      --secondary: #3f37c9;
    }
    
    body {
      margin: 0;
      font-family: 'Segoe UI', system-ui, sans-serif;
      line-height: 1.6;
      color: #333;
    }
    
    .container {
      width: 100%;
      max-width: 1200px;
      margin: 0 auto;
      padding: 0 20px;
    }
  </style>
  
  <!-- 外部样式表 -->
  <link rel="stylesheet" href="styles/main.css">
  
  <!-- 页面特定样式 -->
  <style>
    .hero {
      background: linear-gradient(135deg, var(--primary), var(--secondary));
      color: white;
      padding: 80px 0;
      text-align: center;
    }
  </style>
</head>
<body>
  <header class="hero">
    <div class="container">
      <h1>CSS引入方式指南</h1>
      <p>掌握不同CSS引入方式的适用场景与最佳实践</p>
    </div>
  </header>
  
  <main class="container">
    <section>
      <h2>项目实践建议</h2>
      <p>大型项目使用外部样式表，小型工具使用内部样式表，动态样式使用内联样式</p>
    </section>
  </main>
</body>
</html>
```