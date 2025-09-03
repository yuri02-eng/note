前端开发中那些令人眼花缭乱的尺寸属性，特别是围绕宽度（高度同理）的各种概念：`width`, `offsetWidth`, `clientWidth`, `scrollWidth` 等。理解它们之间的区别对于精确控制布局、处理滚动、响应式设计以及动画效果至关重要。

**offsetWidth** = width + padding-left + padding-right + border-left + border-right + vertical scrollbar

**clientWidth** = width + padding-left + padding-right - vertical scrollbar

**scrollWidth** = 实际内容宽度 + padding-left + padding-right

| 属性             | 包含内容 (`content`) | 包含内边距 (`padding`) | 包含边框 (`border`) | 包含滚动条 (占用空间) | 包含外边距 (`margin`) | 是否包含溢出内容 | 主要用途                                     | 值类型     | 可写性 |
| :--------------- | :------------------- | :--------------------- | :------------------ | :-------------------- | :-------------------- | :--------------- | :------------------------------------------- | :--------- | :----- |
| `style.width`    | ✅                   | ❌                     | ❌                  | ❌                    | ❌                    | ❌               | 获取/设置内联样式宽度                        | 字符串     | ✅     |
| `offsetWidth`    | ✅                   | ✅                     | ✅                  | ✅ (垂直)             | ❌                    | ❌ (仅可视区域)  | 元素总布局宽度 (可视区域+边框+内边距+滚动条) | 数字(像素) | ❌     |
| `clientWidth`    | ✅                   | ✅                     | ❌                  | ❌ (减去占用空间)     | ❌                    | ❌ (仅可视区域)  | 元素内部可视宽度 (内容+内边距，不含滚动条)   | 数字(像素) | ❌     |
| `scrollWidth`    | ✅                   | ✅                     | ❌                  | ❌ (减去占用空间)     | ❌                    | ✅               | 元素内容总宽度 (包括溢出的内容+内边距)       | 数字(像素) | ❌     |

# **核心概念：盒子模型**

理解这些属性的关键在于牢记 CSS 盒子模型。一个元素在页面上占据的空间由内到外包括：

1.  **内容区域 (`content`)**: 元素的实际内容（文本、图片等）显示的区域。
2.  **内边距 (`padding`)**: 内容区域周围的透明区域。
3.  **边框 (`border`)**: 围绕内边距和内容的线。
4.  **外边距 (`margin`)**: 元素边框外的透明区域，用于分隔相邻元素。

## **尺寸属性详解**

以下是 JavaScript 中常用的元素尺寸属性（以宽度为例，高度属性如 `height`, `offsetHeight`, `clientHeight`, `scrollHeight` 规则完全一致）：

1.  **`element.style.width` / `HTMLElement.style.width`**
    
    *   **定义**：获取或设置元素的**内联样式**中的 `width` 值（即写在 HTML 标签的 `style` 属性里的值，或者在 JS 中通过 `element.style.width = ...` 设置的值）。
    *   **值类型**：字符串（如 `"100px"`, `"50%"`, `"auto"`）。
    *   **包含内容**：仅指**内容区域 (`content`)** 的宽度。
    *   **特点**：
        *   只能获取或设置通过内联样式 (`style` 属性) 定义的宽度。
        *   如果元素的宽度是通过 CSS 类（外部或内部样式表）设置的，`element.style.width` 通常返回空字符串 (`""`)。
        *   设置此属性会直接影响元素的内联样式。
    *   **用途**：主要用于动态修改元素的内联样式宽度。
    
2.  **`element.offsetWidth` / `HTMLElement.offsetWidth`**
    
    * **定义**：获取元素的**布局宽度**。这是一个只读属性。
    
      ![Image:Dimensions-offset.png](https://developer.mozilla.org/zh-CN/docs/Web/API/HTMLElement/offsetWidth/dimensions-offset.png)
    
    * **值类型**：数字（整数，单位是 CSS 像素）。
    
    *   **包含内容**：
        
        *   内容区域 (`content`)
        *   内边距 (`padding`)
        *   垂直滚动条宽度 (如果存在且占用空间)
        *   边框 (`border`)
        
    *   **不包含**：
        *   外边距 (`margin`)
        
    *   **特点**：
        *   反映元素在页面上实际占据的**总可视宽度**（包括边框、内边距和滚动条）。
        *   是一个整数，单位为像素。
        *   这个值会触发浏览器的 **`重排` (reflow)** 来计算，频繁访问可能影响性能。
        
    *   **用途**：当你需要知道元素在页面上实际占据了多少物理空间（包括边框和内边距）时使用。常用于计算元素相对于其 `offsetParent` 的位置（与 `offsetLeft`, `offsetTop` 配合）。
    
3. **`element.clientWidth` / `HTMLElement.clientWidth`**

   * **定义**：获取元素的**内部宽度**，包含内边距但不包含滚动条、边框和外边距。这是一个只读属性。

     ![img](https://developer.mozilla.org/zh-CN/docs/Web/API/Element/clientWidth/dimensions-client.png)

   * **值类型**：数字（整数，单位是 CSS 像素）。

   *   **包含内容**：
       *   内容区域 (`content`)
       *   内边距 (`padding`)
       
   *   **不包含**：
       *   垂直滚动条宽度 (如果存在且占用空间)
       *   边框 (`border`)
       *   外边距 (`margin`)
       
   *   **特点**：
       *   反映元素**内容区域 + 内边距**的宽度，即可供内容显示的实际区域宽度（不包括滚动条占用的空间）。
       *   如果元素有滚动条，`clientWidth` 会减去滚动条的宽度。
       *   同样是一个整数像素值。
       *   获取此属性也可能触发重排，但通常比 `offsetWidth` 稍轻量（取决于浏览器实现）。
       
   *   **用途**：当你关心元素内部可用于显示内容的实际宽度（不包括滚动条和边框）时使用。常用于计算视口大小（`document.documentElement.clientWidth` 或 `window.innerWidth` 更常用）或可滚动容器的可视区域。

4.  **`element.scrollWidth` / `HTMLElement.scrollWidth`**
    * **定义**：获取元素**内容的总宽度**，包括由于溢出而不可见的部分。这是一个只读属性。
    
    * **值类型**：数字（整数，单位是 CSS 像素）。
    
    *   **包含内容**：
        *   元素内容的总宽度（包括 `padding` 内的所有内容）。
        *   如果内容没有溢出，`scrollWidth` 通常等于 `clientWidth`。
        *   如果内容溢出（水平方向），`scrollWidth` 会大于 `clientWidth`，其值等于内容实际需要的宽度（包括 `padding`）。
        
    *   **不包含**：
        *   垂直滚动条宽度 (通常不影响水平宽度计算)
        *   边框 (`border`)
        *   外边距 (`margin`)
        
    *   **特点**：
        *   反映元素内容的**完整宽度**，无论是否可见。
        *   包括 `padding`，但不包括滚动条、`border` 和 `margin`。
        *   是一个整数像素值。
        *   获取此属性也可能触发重排。
        
    * **用途**：判断元素的内容是否发生了水平溢出 (`scrollWidth > clientWidth`)。计算可滚动内容的总宽度，用于实现自定义滚动条或滚动指示器。
    
      ```javascript
      <!DOCTYPE html>
      <html lang="zh-CN">
      <head>
          <meta charset="UTF-8">
          <meta name="viewport" content="width=device-width, initial-scale=1.0">
          <title>scrollWidth 属性演示</title>
          <style>
              * {
                  box-sizing: border-box;
                  margin: 0;
                  padding: 0;
              }
              
              body {
                  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
                  background: linear-gradient(135deg, #1a2a6c, #b21f1f);
                  color: #333;
                  line-height: 1.6;
                  padding: 20px;
                  min-height: 100vh;
                  display: flex;
                  justify-content: center;
                  align-items: center;
              }
              
              .container {
                  max-width: 800px;
                  width: 100%;
                  background-color: rgba(255, 255, 255, 0.95);
                  border-radius: 15px;
                  box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);
                  padding: 30px;
                  overflow: hidden;
              }
              
              header {
                  text-align: center;
                  margin-bottom: 30px;
                  padding-bottom: 20px;
                  border-bottom: 2px solid #eee;
              }
              
              h1 {
                  color: #1a2a6c;
                  font-size: 2.5rem;
                  margin-bottom: 10px;
              }
              
              .subtitle {
                  color: #555;
                  font-size: 1.2rem;
                  max-width: 600px;
                  margin: 0 auto;
              }
              
              .demo-area {
                  display: flex;
                  flex-direction: column;
                  gap: 20px;
                  margin-bottom: 30px;
              }
              
              .scroll-container {
                  position: relative;
                  height: 250px;
                  background: #f9f9f9;
                  border-radius: 10px;
                  padding: 20px;
                  box-shadow: 0 5px 15px rgba(0, 0, 0, 0.1);
                  overflow: auto;
                  border: 4px solid #4a69bd;
              }
              
              .scroll-content {
                  width: 150%;
                  padding: 20px;
                  background: linear-gradient(to right, #e6e9f0, #eef1f5);
                  border-radius: 8px;
                  min-height: 200px;
              }
              
              .scroll-content p {
                  margin-bottom: 15px;
                  line-height: 1.7;
              }
              
              .properties {
                  background: white;
                  border-radius: 8px;
                  padding: 20px;
                  box-shadow: 0 3px 10px rgba(0, 0, 0, 0.08);
                  text-align: center;
              }
              
              .property {
                  margin: 15px 0;
              }
              
              .property-name {
                  font-weight: bold;
                  color: #1a2a6c;
                  font-size: 1.3rem;
              }
              
              .property-value {
                  font-family: monospace;
                  font-size: 1.5rem;
                  color: #b21f1f;
                  margin-top: 10px;
                  display: inline-block;
                  padding: 8px 15px;
                  background: #f8f9fa;
                  border-radius: 5px;
                  min-width: 150px;
              }
              
              .explanation {
                  margin-top: 30px;
                  background: white;
                  border-radius: 10px;
                  padding: 25px;
                  box-shadow: 0 5px 15px rgba(0, 0, 0, 0.1);
              }
              
              .explanation h2 {
                  color: #1a2a6c;
                  margin-bottom: 15px;
                  text-align: center;
              }
              
              .definition {
                  background: #f8f9fa;
                  padding: 20px;
                  border-left: 4px solid #4a69bd;
                  margin: 20px 0;
                  border-radius: 0 5px 5px 0;
              }
              
              .definition h3 {
                  color: #b21f1f;
                  margin-bottom: 10px;
              }
              
              .definition p {
                  margin-bottom: 10px;
                  line-height: 1.7;
              }
              
              .controls {
                  display: flex;
                  flex-wrap: wrap;
                  gap: 15px;
                  justify-content: center;
                  margin: 20px 0;
              }
              
              .control-group {
                  background: white;
                  padding: 15px;
                  border-radius: 8px;
                  box-shadow: 0 3px 10px rgba(0, 0, 0, 0.08);
                  min-width: 200px;
              }
              
              .control-title {
                  text-align: center;
                  margin-bottom: 10px;
                  color: #1a2a6c;
              }
              
              .slider-container {
                  margin: 10px 0;
              }
              
              label {
                  display: block;
                  margin-bottom: 5px;
                  font-weight: 500;
              }
              
              input[type="range"] {
                  width: 100%;
              }
              
              .value-display {
                  text-align: center;
                  font-weight: bold;
                  margin-top: 5px;
                  color: #1a2a6c;
              }
              
              footer {
                  text-align: center;
                  margin-top: 30px;
                  padding-top: 20px;
                  border-top: 1px solid #ddd;
                  color: #777;
              }
              
              .highlight {
                  background-color: #ffeb3b;
                  padding: 2px 5px;
                  border-radius: 3px;
              }
          </style>
      </head>
      <body>
          <div class="container">
              <header>
                  <h1>scrollWidth 属性演示</h1>
                  <p class="subtitle">本演示专门展示 scrollWidth 属性的含义和用法</p>
              </header>
              
              <div class="demo-area">
                  <div class="scroll-container" id="scrollBox">
                      <div class="scroll-content" id="scrollContent">
                          <h3>scrollWidth 演示内容</h3>
                          <p>scrollWidth 属性返回元素内容的整个宽度，包括由于溢出而不可见的部分。</p>
                          <p>在这个例子中，内容区域的宽度被设置为容器宽度的 <span id="contentPercent">150%</span>，因此内容超出了容器的边界。</p>
                          <p>scrollWidth 的值等于整个内容区域的宽度，包括不可见的部分。</p>
                          <p>尝试滚动此元素（水平方向）并观察 scrollWidth 的值保持不变。</p>
                          <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nullam auctor, nisl eget ultricies tincidunt, nisl nisl aliquam nisl, eget ultricies nisl nisl eget nisl.</p>
                          <p>Sed vitae nisl eget nisl ultricies tincidunt. Nullam auctor, nisl eget ultricies tincidunt, nisl nisl aliquam nisl, eget ultricies nisl nisl eget nisl.</p>
                      </div>
                  </div>
                  
                  <div class="properties">
                      <div class="property">
                          <div class="property-name">scrollWidth</div>
                          <div class="property-value" id="scrollWidthValue">0px</div>
                      </div>
                      <div class="property">
                          <div class="property-name">clientWidth</div>
                          <div class="property-value" id="clientWidthValue">0px</div>
                      </div>
                  </div>
              </div>
              
              <div class="controls">
                  <div class="control-group">
                      <h3 class="control-title">调整内容宽度</h3>
                      <div class="slider-container">
                          <label for="contentSlider">内容宽度百分比:</label>
                          <input type="range" id="contentSlider" min="100" max="300" value="150">
                          <div class="value-display"><span id="contentValue">150</span>%</div>
                      </div>
                  </div>
              </div>
              
              <div class="explanation">
                  <h2>scrollWidth 详解</h2>
                  
                  <div class="definition">
                      <h3>什么是 scrollWidth？</h3>
                      <p><span class="highlight">scrollWidth</span> 属性返回元素内容的整个宽度，包括由于溢出而在屏幕上不可见的部分。</p>
                      <p>这个属性对于检测内容是否溢出非常有用。</p>
                  </div>
                  
                  <div class="definition">
                      <h3>主要特点</h3>
                      <p>• 包括元素的内容、内边距，但不包括边框、外边距或滚动条</p>
                      <p>• 如果内容没有溢出，scrollWidth 通常等于 clientWidth</p>
                      <p>• 值始终大于或等于 clientWidth</p>
                      <p>• 只读属性，不能直接修改</p>
                  </div>
                  
                  <div class="definition">
                      <h3>实际应用场景</h3>
                      <p>• 检测内容是否溢出容器</p>
                      <p>• 创建自定义滚动条或滚动指示器</p>
                      <p>• 实现响应式布局调整</p>
                      <p>• 构建可滚动的选项卡或导航组件</p>
                  </div>
              </div>
              
              <footer>
                  <p>scrollWidth 属性演示 &copy; 2023</p>
              </footer>
          </div>
      
          <script>
              // 获取DOM元素
              const scrollBox = document.getElementById('scrollBox');
              const scrollContent = document.getElementById('scrollContent');
              const scrollWidthValue = document.getElementById('scrollWidthValue');
              const clientWidthValue = document.getElementById('clientWidthValue');
              const contentSlider = document.getElementById('contentSlider');
              const contentValue = document.getElementById('contentValue');
              const contentPercent = document.getElementById('contentPercent');
              
              // 更新宽度显示
              function updateWidths() {
                  scrollWidthValue.textContent = scrollBox.scrollWidth + 'px';
                  clientWidthValue.textContent = scrollBox.clientWidth + 'px';
              }
              
              // 更新内容宽度
              function updateContentWidth() {
                  const percent = contentSlider.value;
                  scrollContent.style.width = percent + '%';
                  contentValue.textContent = percent;
                  contentPercent.textContent = percent + '%';
                  updateWidths();
              }
              
              // 初始更新
              updateContentWidth();
              
              // 添加事件监听器
              contentSlider.addEventListener('input', updateContentWidth);
              
              // 添加滚动事件监听器
              scrollBox.addEventListener('scroll', updateWidths);
          </script>
      </body>
      </html>
      ```
    
      

## **高度属性 (`height`, `offsetHeight`, `clientHeight`, `scrollHeight`)**

高度属性的规则与宽度属性 **完全对称**，只是方向从水平变成了垂直。例如：

* `offsetHeight` = `content height` + `padding` + `horizontal scrollbar height` (if present) + `border`

* `clientHeight` = `content height` + `padding` (excluding horizontal scrollbar height if present)

* `scrollHeight` = 元素内容的总高度（包括溢出的部分）+ `padding`

# 滚动条对元素宽度的影响详解

## 核心结论

**滚动条会占用元素宽度，但具体影响取决于浏览器和操作系统设置。**

## 详细分析

### 1. 默认行为（Windows系统）

在Windows系统中，滚动条会占用元素的实际宽度空间：
- 当出现垂直滚动条时，它会占据元素内容区域的宽度
- 这会导致元素的实际可用内容宽度减少
- 所有宽度属性都会受到影响

### 2. macOS系统行为

在macOS系统中，默认行为有所不同：
- 滚动条默认是"覆盖式"的（不占用空间）
- 只有当用户开始滚动时才会显示
- 可以通过系统设置改为"永久显示"（此时会占用空间）

### 3. 对宽度属性的影响

| 属性         | 是否包含滚动条宽度 | 说明 |
|--------------|-------------------|------|
| `offsetWidth`| ✅ 是             | 包含滚动条宽度（如果占用空间） |
| `clientWidth`| ❌ 否             | 不包含滚动条宽度 |
| `scrollWidth`| ❌ 否             | 不包含滚动条宽度 |
| `style.width`| ❌ 否             | 仅内联样式值 |

### 4. 实际影响示例

假设一个元素：
- 内容宽度：400px
- 内边距：20px
- 边框：5px
- 滚动条宽度：15px

**无滚动条时：**
- offsetWidth = 400 + 20×2 + 5×2 = 450px
- clientWidth = 400 + 20×2 = 440px

**有滚动条时（占用空间）：**
- offsetWidth = 450px（不变）
- clientWidth = 440px - 15px = 425px（内容区域被压缩）

### 5. 检测滚动条是否占用空间

```javascript
function isScrollbarTakingSpace(element) {
  return element.offsetWidth > element.clientWidth;
}
```

### 6. 避免滚动条影响布局的方法

1. **使用CSS技巧**：
   ```css
   .container {
     /* 预留滚动条空间 */
     padding-right: 15px; 
     overflow-y: scroll;
   }
   ```

2. **使用自定义滚动条**：
   ```css
   .container::-webkit-scrollbar {
     width: 8px;
     background: transparent;
   }
   ```

3. **使用overlay滚动条**：
   ```css
   .container {
     overflow-y: overlay;
   }
   ```

## 总结

- 在Windows系统中，滚动条**会占用**元素宽度空间
- 在macOS系统中，滚动条默认**不占用**空间
- 使用`clientWidth`可以获取不包含滚动条的实际内容宽度
- 使用`offsetWidth`可以获取包含滚动条的总布局宽度
- 在开发响应式布局时，需要考虑滚动条对宽度的影响

## **总结表格**

| 属性             | 包含内容 (`content`) | 包含内边距 (`padding`) | 包含边框 (`border`) | 包含滚动条 (占用空间) | 包含外边距 (`margin`) | 是否包含溢出内容 | 主要用途                                     | 值类型     | 可写性 |
| :--------------- | :------------------- | :--------------------- | :------------------ | :-------------------- | :-------------------- | :--------------- | :------------------------------------------- | :--------- | :----- |
| `style.width`    | ✅                   | ❌                     | ❌                  | ❌                    | ❌                    | ❌               | 获取/设置内联样式宽度                        | 字符串     | ✅     |
| `offsetWidth`    | ✅                   | ✅                     | ✅                  | ✅ (垂直)             | ❌                    | ❌ (仅可视区域)  | 元素总布局宽度 (可视区域+边框+内边距+滚动条) | 数字(像素) | ❌     |
| `clientWidth`    | ✅                   | ✅                     | ❌                  | ❌ (减去占用空间)     | ❌                    | ❌ (仅可视区域)  | 元素内部可视宽度 (内容+内边距，不含滚动条)   | 数字(像素) | ❌     |
| `scrollWidth`    | ✅                   | ✅                     | ❌                  | ❌ (减去占用空间)     | ❌                    | ✅               | 元素内容总宽度 (包括溢出的内容+内边距)       | 数字(像素) | ❌     |

**重要注意事项**

1.  **单位**：`offsetWidth`, `clientWidth`, `scrollWidth` 返回的值总是以 **CSS 像素 (px)** 为单位的整数。`style.width` 返回的是字符串，包含单位。
2.  **重排 (Reflow)**：读取 `offsetWidth`, `clientWidth`, `scrollWidth` 等属性通常需要浏览器计算最新的布局信息，这会触发 **重排**。频繁读取这些属性（尤其是在循环或动画中）会显著影响页面性能。优化策略包括批量读取、缓存结果或使用不会触发重排的 API（如 `ResizeObserver`）。
3.  **`box-sizing` 的影响**：CSS 的 `box-sizing` 属性 (`content-box` 或 `border-box`) 会直接影响 `width` 和 `height` 属性的含义：
    *   `box-sizing: content-box` (默认)：`width/height` 仅指内容区域 (`content`) 的尺寸。`offsetWidth = width + padding + border + scrollbar`。
    *   `box-sizing: border-box`：`width/height` 指定了内容区域 (`content`) + 内边距 (`padding`) + 边框 (`border`) 的总尺寸（不包括 `margin`）。此时 `offsetWidth` 通常等于 `width`（假设没有滚动条占用空间，或者滚动条在 `padding` 内）。
4.  **滚动条位置**：`scrollLeft` 和 `scrollTop` 属性用于获取或设置元素内容水平/垂直滚动的距离。
5.  **视口尺寸**：
    *   `window.innerWidth` / `window.innerHeight`：浏览器视口（viewport）的内部宽度/高度（包括垂直/水平滚动条宽度）。
    *   `document.documentElement.clientWidth` / `document.documentElement.clientHeight`：HTML 文档根元素 (`<html>`) 的可视区域宽度/高度（不包括滚动条）。这通常被认为是视口的“净”尺寸，是响应式设计中判断视口大小的更可靠方式（尤其是在考虑滚动条时）。
    *   `window.outerWidth` / `window.outerHeight`：整个浏览器窗口的宽度/高度（包括地址栏、书签栏等浏览器 UI）。

**如何选择？**

*   想动态**设置**元素的宽度？ ➡️ 使用 `element.style.width` (内联样式) 或通过修改 CSS 类。
*   想知道元素在页面上**总共占了多宽**（包括边框、内边距、滚动条）？ ➡️ 用 `offsetWidth`。
*   想知道元素**内部实际可用的内容显示区域有多宽**（不包括边框和滚动条）？ ➡️ 用 `clientWidth`。
*   想知道元素**内容的完整宽度是多少**（包括被滚动隐藏的部分）？ ➡️ 用 `scrollWidth` (常用于检查溢出或计算滚动范围)。

深刻理解这些属性的差异和计算规则，是解决复杂布局问题、实现精确交互效果的基础。务必结合 CSS 盒子模型和 `box-sizing` 属性来综合理解。

## 演示代码

```javascript
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DOM宽度属性对比演示</title>
    <style>
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1a2a6c, #b21f1f, #fdbb2d);
            color: #333;
            line-height: 1.6;
            padding: 20px;
            min-height: 100vh;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background-color: rgba(255, 255, 255, 0.95);
            border-radius: 15px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);
            padding: 30px;
            overflow: hidden;
        }
        
        header {
            text-align: center;
            margin-bottom: 30px;
            padding-bottom: 20px;
            border-bottom: 2px solid #eee;
        }
        
        h1 {
            color: #1a2a6c;
            font-size: 2.5rem;
            margin-bottom: 10px;
        }
        
        .subtitle {
            color: #555;
            font-size: 1.2rem;
            max-width: 800px;
            margin: 0 auto;
        }
        
        .demo-area {
            display: flex;
            flex-wrap: wrap;
            gap: 30px;
            margin-bottom: 40px;
        }
        
        .box-container {
            flex: 1;
            min-width: 300px;
            background: #f9f9f9;
            border-radius: 10px;
            padding: 20px;
            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.1);
        }
        
        .box-title {
            text-align: center;
            margin-bottom: 15px;
            color: #b21f1f;
            font-size: 1.4rem;
        }
        
        .box {
            height: 200px;
            background: linear-gradient(to bottom right, #e6e9f0, #eef1f5);
            border: 5px solid #4a69bd;
            padding: 20px;
            margin: 0 auto 20px;
            overflow: auto;
            border-radius: 8px;
            position: relative;
        }
        
        .box-content {
            width: 150%;
            padding: 10px;
            background: rgba(255, 255, 255, 0.7);
            border-radius: 5px;
        }
        
        .properties {
            background: white;
            border-radius: 8px;
            padding: 15px;
            box-shadow: 0 3px 10px rgba(0, 0, 0, 0.08);
        }
        
        .property {
            display: flex;
            justify-content: space-between;
            padding: 10px 0;
            border-bottom: 1px dashed #ddd;
        }
        
        .property:last-child {
            border-bottom: none;
        }
        
        .property-name {
            font-weight: bold;
            color: #1a2a6c;
        }
        
        .property-value {
            font-family: monospace;
            font-size: 1.1rem;
            color: #b21f1f;
        }
        
        .visualization {
            background: white;
            border-radius: 10px;
            padding: 25px;
            margin-top: 30px;
            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.1);
        }
        
        .vis-title {
            text-align: center;
            margin-bottom: 20px;
            color: #1a2a6c;
            font-size: 1.6rem;
        }
        
        .diagram {
            position: relative;
            height: 300px;
            background: #f8f9fa;
            border: 2px solid #ddd;
            border-radius: 8px;
            margin: 20px 0;
            overflow: hidden;
        }
        
        .element {
            position: absolute;
            top: 50px;
            left: 50px;
            width: 400px;
            height: 150px;
            background: rgba(74, 105, 189, 0.1);
            border: 4px solid #4a69bd;
            padding: 20px;
        }
        
        .content {
            position: absolute;
            top: 20px;
            left: 20px;
            width: 500px;
            height: 300px;
            background: rgba(253, 187, 45, 0.2);
            border: 2px dashed #fdbb2d;
        }
        
        .label {
            position: absolute;
            background: rgba(0, 0, 0, 0.7);
            color: white;
            padding: 5px 10px;
            border-radius: 4px;
            font-size: 0.9rem;
            z-index: 10;
        }
        
        .line {
            position: absolute;
            border: 1px dashed #333;
        }
        
        .offset-line {
            border-color: #e74c3c;
        }
        
        .client-line {
            border-color: #2ecc71;
        }
        
        .scroll-line {
            border-color: #9b59b6;
        }
        
        .width-line {
            border-color: #3498db;
        }
        
        .controls {
            display: flex;
            flex-wrap: wrap;
            gap: 15px;
            justify-content: center;
            margin-top: 20px;
        }
        
        .control-group {
            background: white;
            padding: 15px;
            border-radius: 8px;
            box-shadow: 0 3px 10px rgba(0, 0, 0, 0.08);
            min-width: 200px;
        }
        
        .control-title {
            text-align: center;
            margin-bottom: 10px;
            color: #1a2a6c;
        }
        
        .slider-container {
            margin: 10px 0;
        }
        
        label {
            display: block;
            margin-bottom: 5px;
            font-weight: 500;
        }
        
        input[type="range"] {
            width: 100%;
        }
        
        .explanation {
            margin-top: 30px;
            background: white;
            border-radius: 10px;
            padding: 25px;
            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.1);
        }
        
        .explanation h2 {
            color: #1a2a6c;
            margin-bottom: 15px;
            text-align: center;
        }
        
        .definition {
            background: #f8f9fa;
            padding: 15px;
            border-left: 4px solid #4a69bd;
            margin: 15px 0;
            border-radius: 0 5px 5px 0;
        }
        
        .definition h3 {
            color: #b21f1f;
            margin-bottom: 8px;
        }
        
        footer {
            text-align: center;
            margin-top: 30px;
            padding-top: 20px;
            border-top: 1px solid #ddd;
            color: #777;
        }
        
        @media (max-width: 768px) {
            .demo-area {
                flex-direction: column;
            }
            
            .diagram {
                height: 200px;
            }
            
            .element {
                width: 250px;
            }
            
            .content {
                width: 350px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>DOM元素宽度属性对比</h1>
            <p class="subtitle">本演示直观展示了offsetWidth、clientWidth、scrollWidth和CSS width属性之间的区别，帮助您理解这些常用但容易混淆的属性</p>
        </header>
        
        <div class="demo-area">
            <div class="box-container">
                <h2 class="box-title">带边框和内边距的元素</h2>
                <div class="box" id="box1">
                    <div class="box-content">
                        <p>这是一个带有边框和内边距的盒子元素。</p>
                        <p>CSS width: 300px</p>
                        <p>边框: 5px</p>
                        <p>内边距: 20px</p>
                    </div>
                </div>
                <div class="properties">
                    <div class="property">
                        <span class="property-name">offsetWidth:</span>
                        <span class="property-value" id="offsetWidth1">0</span>
                    </div>
                    <div class="property">
                        <span class="property-name">clientWidth:</span>
                        <span class="property-value" id="clientWidth1">0</span>
                    </div>
                    <div class="property">
                        <span class="property-name">scrollWidth:</span>
                        <span class="property-value" id="scrollWidth1">0</span>
                    </div>
                    <div class="property">
                        <span class="property-name">CSS width:</span>
                        <span class="property-value" id="cssWidth1">0</span>
                    </div>
                </div>
            </div>
            
            <div class="box-container">
                <h2 class="box-title">带滚动内容的元素</h2>
                <div class="box" id="box2">
                    <div class="box-content">
                        <p>这个元素的内容超出了其边界，因此可以滚动。</p>
                        <p>实际内容宽度比元素宽度大50%</p>
                        <p>尝试滚动此元素并观察scrollWidth的变化</p>
                        <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nullam auctor, nisl eget ultricies tincidunt, nisl nisl aliquam nisl, eget ultricies nisl nisl eget nisl.</p>
                        <p>Sed vitae nisl eget nisl ultricies tincidunt. Nullam auctor, nisl eget ultricies tincidunt, nisl nisl aliquam nisl, eget ultricies nisl nisl eget nisl.</p>
                    </div>
                </div>
                <div class="properties">
                    <div class="property">
                        <span class="property-name">offsetWidth:</span>
                        <span class="property-value" id="offsetWidth2">0</span>
                    </div>
                    <div class="property">
                        <span class="property-name">clientWidth:</span>
                        <span class="property-value" id="clientWidth2">0</span>
                    </div>
                    <div class="property">
                        <span class="property-name">scrollWidth:</span>
                        <span class="property-value" id="scrollWidth2">0</span>
                    </div>
                    <div class="property">
                        <span class="property-name">CSS width:</span>
                        <span class="property-value" id="cssWidth2">0</span>
                    </div>
                </div>
            </div>
        </div>
        
        <div class="controls">
            <div class="control-group">
                <h3 class="control-title">调整边框</h3>
                <div class="slider-container">
                    <label for="borderSlider">边框宽度: <span id="borderValue">5</span>px</label>
                    <input type="range" id="borderSlider" min="0" max="20" value="5">
                </div>
            </div>
            
            <div class="control-group">
                <h3 class="control-title">调整内边距</h3>
                <div class="slider-container">
                    <label for="paddingSlider">内边距: <span id="paddingValue">20</span>px</label>
                    <input type="range" id="paddingSlider" min="0" max="50" value="20">
                </div>
            </div>
            
            <div class="control-group">
                <h3 class="control-title">调整内容宽度</h3>
                <div class="slider-container">
                    <label for="contentSlider">内容宽度: <span id="contentValue">150</span>%</label>
                    <input type="range" id="contentSlider" min="100" max="300" value="150">
                </div>
            </div>
        </div>
        
        <div class="visualization">
            <h2 class="vis-title">属性测量范围可视化</h2>
            <div class="diagram">
                <div class="element">
                    <div class="content"></div>
                </div>
                
                <!-- 标签和线条 -->
                <div class="label" style="top: 20px; left: 250px;">offsetWidth (边框+内边距+内容)</div>
                <div class="line offset-line" style="top: 45px; left: 50px; width: 400px;"></div>
                
                <div class="label" style="top: 90px; left: 270px;">clientWidth (内边距+内容)</div>
                <div class="line client-line" style="top: 115px; left: 54px; width: 392px;"></div>
                
                <div class="label" style="top: 160px; left: 290px;">CSS width (内容宽度)</div>
                <div class="line width-line" style="top: 185px; left: 54px; width: 392px;"></div>
                
                <div class="label" style="top: 230px; left: 350px;">scrollWidth (实际内容宽度)</div>
                <div class="line scroll-line" style="top: 255px; left: 20px; width: 500px;"></div>
            </div>
        </div>
        
        <div class="explanation">
            <h2>属性定义与区别</h2>
            
            <div class="definition">
                <h3>offsetWidth</h3>
                <p>元素在页面中占据的总宽度，包括：内容宽度 + 内边距 + 边框 + 垂直滚动条（如果存在）。不包括外边距。</p>
                <p><strong>公式：</strong> offsetWidth = border-left + padding-left + width + padding-right + border-right</p>
            </div>
            
            <div class="definition">
                <h3>clientWidth</h3>
                <p>元素内部可视区域的宽度，包括：内容宽度 + 内边距。不包括边框、外边距和垂直滚动条。</p>
                <p><strong>公式：</strong> clientWidth = padding-left + width + padding-right</p>
            </div>
            
            <div class="definition">
                <h3>scrollWidth</h3>
                <p>元素内容的实际宽度（包括由于溢出而不可见的部分）。如果内容没有溢出，则等于clientWidth。</p>
                <p>这个属性对于检测内容是否溢出非常有用。</p>
            </div>
            
            <div class="definition">
                <h3>CSS width</h3>
                <p>元素内容区域的宽度，由CSS的width属性设置。不包括内边距、边框或外边距。</p>
                <p>在JavaScript中可以通过getComputedStyle(element).width获取。</p>
            </div>
        </div>
        
        <footer>
            <p>DOM元素宽度属性对比演示 &copy; 2023</p>
        </footer>
    </div>

    <script>
        // 获取DOM元素
        const box1 = document.getElementById('box1');
        const box2 = document.getElementById('box2');
        const boxContent = document.querySelectorAll('.box-content');
        
        // 获取显示属性值的元素
        const offsetWidth1 = document.getElementById('offsetWidth1');
        const clientWidth1 = document.getElementById('clientWidth1');
        const scrollWidth1 = document.getElementById('scrollWidth1');
        const cssWidth1 = document.getElementById('cssWidth1');
        
        const offsetWidth2 = document.getElementById('offsetWidth2');
        const clientWidth2 = document.getElementById('clientWidth2');
        const scrollWidth2 = document.getElementById('scrollWidth2');
        const cssWidth2 = document.getElementById('cssWidth2');
        
        // 获取控制滑块
        const borderSlider = document.getElementById('borderSlider');
        const paddingSlider = document.getElementById('paddingSlider');
        const contentSlider = document.getElementById('contentSlider');
        
        // 获取滑块值显示元素
        const borderValue = document.getElementById('borderValue');
        const paddingValue = document.getElementById('paddingValue');
        const contentValue = document.getElementById('contentValue');
        
        // 更新所有宽度显示
        function updateWidths() {
            // 第一个盒子
            offsetWidth1.textContent = box1.offsetWidth + 'px';
            clientWidth1.textContent = box1.clientWidth + 'px';
            scrollWidth1.textContent = box1.scrollWidth + 'px';
            cssWidth1.textContent = parseInt(getComputedStyle(box1).width) + 'px';
            
            // 第二个盒子
            offsetWidth2.textContent = box2.offsetWidth + 'px';
            clientWidth2.textContent = box2.clientWidth + 'px';
            scrollWidth2.textContent = box2.scrollWidth + 'px';
            cssWidth2.textContent = parseInt(getComputedStyle(box2).width) + 'px';
        }
        
        // 初始化滑块事件监听
        borderSlider.addEventListener('input', function() {
            const borderWidth = this.value + 'px';
            box1.style.borderWidth = borderWidth;
            box2.style.borderWidth = borderWidth;
            borderValue.textContent = this.value;
            updateWidths();
        });
        
        paddingSlider.addEventListener('input', function() {
            const padding = this.value + 'px';
            box1.style.padding = padding;
            box2.style.padding = padding;
            paddingValue.textContent = this.value;
            updateWidths();
        });
        
        contentSlider.addEventListener('input', function() {
            const contentWidth = this.value + '%';
            boxContent.forEach(content => {
                content.style.width = contentWidth;
            });
            contentValue.textContent = this.value;
            updateWidths();
        });
        
        // 初始更新
        updateWidths();
        
        // 添加滚动事件监听器来更新第二个盒子的scrollWidth显示
        box2.addEventListener('scroll', updateWidths);
    </script>
</body>
</html>
```

