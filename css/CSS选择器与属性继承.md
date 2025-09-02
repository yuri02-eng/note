### CSS 选择器与可继承属性笔记

---

#### **一、CSS 选择器分类**
**1. 基础选择器**
- **元素选择器**：`p { }`  
- **类选择器**：`.class { }`  
- **ID 选择器**：`#id { }`  
- **通配符选择器**：`* { }`（选择所有元素）  
- **属性选择器**：  
  - `[attr]`（存在属性）  
  - `[attr="value"]`（精确匹配）  
  - `[attr^="val"]`（开头匹配）  
  - `[attr$="ue"]`（结尾匹配）  
  - `[attr*="alu"]`（包含匹配）  

**2. 组合选择器**
- **后代选择器**：`div p`（所有嵌套的 `<p>`）  
- **子选择器**：`div > p`（直接子元素 `<p>`）  
- **相邻兄弟选择器**：`h1 + p`（紧接在 `h1` 后的第一个 `<p>`）  
- **通用兄弟选择器**：`h1 ~ p`（`h1` 后的所有同级 `<p>`）  

**3. 伪类选择器**
- **状态伪类**：  
  `:hover`, `:active`, `:focus`, `:visited`, `:link`  
- **结构伪类**：  
  `:first-child`, `:last-child`, `:nth-child(n)`, `:nth-of-type(n)`, `:not(selector)`  
- **表单伪类**：  
  `:checked`, `:disabled`, `:valid`  

**4. 伪元素选择器**
- `::before`, `::after`（插入内容）  
- `::first-line`, `::first-letter`（文本首行/首字母）  
- `::selection`（用户选中的文本）  

---

#### **二、可继承的 CSS 属性**
**继承规则**：子元素默认继承父元素的某些样式（除非自身显式覆盖）。

##### **1. 文本与字体属性**
- `font-family`（字体族）  
- `font-size`（字体大小）  
- `font-weight`（字重，如 `bold`）  
- `font-style`（斜体，如 `italic`）  
- `text-align`（文本对齐）  
- `line-height`（行高）  
- `color`（文本颜色）  
- `letter-spacing`（字符间距）  
- `word-spacing`（单词间距）  
- `text-indent`（首行缩进）  

##### **2. 列表属性**
- `list-style-type`（列表标记类型，如 `circle`）  
- `list-style-position`（标记位置，如 `inside`）  

##### **3. 其他可继承属性**
- `visibility`（元素可见性）  
- `cursor`（鼠标指针样式）  
- `quotes`（引号样式）  

---

#### **三、不可继承的常见属性**
- **盒模型属性**：  
  `width`, `height`, `margin`, `padding`, `border`  
- **定位与布局**：  
  `position`, `top/right/bottom/left`, `display`, `float`, `flex`, `grid`  
- **背景与边框**：  
  `background`, `background-color`, `border-radius`  
- **其他**：  
  `overflow`, `z-index`, `opacity`, `transform`  

---

#### **四、继承的特殊控制**
1. **强制继承**：  
   ```css
   .child {
     color: inherit; /* 强制继承父元素的 color */
   }
   ```
2. **重置为默认值**：  
   ```css
   .child {
     font-size: initial; /* 使用浏览器默认值 */
   }
   ```

---

#### **五、示例对比**
```html
<div class="parent">
  父元素文本
  <p class="child">子元素文本</p>
</div>
```

```css
.parent {
  color: blue;        /* 可继承 → 子元素变蓝 */
  padding: 20px;      /* 不可继承 → 子元素无内边距 */
  font-weight: bold;  /* 可继承 → 子元素加粗 */
}

.child {
  color: red;         /* 覆盖继承的蓝色 */
}
```

---

**总结**：  
- **选择器**用于精准定位元素，优先掌握组合选择器与伪类。  
- **可继承属性**集中在文本、字体、列表等样式，布局属性通常不可继承。  
- 善用 `inherit` 和 `initial` 显式控制继承行为。
- 可继承的CSS属性主要与文本相关，常见的有：

**字体相关属性：**font-family（字体系列）、font-size（字体大小）、font-weight（字体粗细）、font-style（字体样式，如斜体）、font-variant（字体变体，如小型大写字母）。例如，设置<body>元素的font-family: Arial, sans-serif;，其所有后代元素若未单独设置font-family，将继承该字体。
**文本排版属性：**color（文本颜色）、text-align（文本对齐方式）、text-indent（首行缩进）、line-height（行高）、word-spacing（单词间距）、letter-spacing（字母间距）。如在<body>上设置color: #333;，页面内所有文本元素（<p>、<h1> - <h6>等）的文本颜色若未特别指定，将继承该颜色。
**列表相关属性：**list-style-type（列表项标记类型，如圆点、数字等）、list-style-image（列表项使用的图像）、list-style-position（列表项标记的位置）。当在父元素（如<ul>或<ol>）上设置这些属性，其子列表项（<li>）会继承。
**不可继承的属性通常与布局和盒模型相关**，如width、height、margin、padding、border、display、position等。例如，<div>元素设置的width不会被其内部的<p>元素继承，<p>元素需单独设置width属性来确定自身宽度。