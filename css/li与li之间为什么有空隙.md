### li元素间空白间隔的原因

当`<li>`元素设置为`inline`或`inline-block`时，HTML代码中的**换行符和缩进空格**会被浏览器解析为文本节点，形成约4px的空白间隙。这是浏览器默认行为：将HTML中的空白字符（包括换行）渲染为空格。

```html
<ul>
  <li>项目1</li>  <!-- 这里的换行符会被解析为空格 -->
  <li>项目2</li>
  <li>项目3</li>
</ul>
```

### 5种解决方案（附代码示例）

#### 1. 移除HTML空白（简单直接）
```html
<ul><li>项目1</li><li>项目2</li><li>项目3</li></ul>
```
**缺点**：降低代码可读性

#### 2. 使用负边距（兼容性好）
```css
ul {
  font-size: 0; /* 清除空格影响 */
}
li {
  display: inline-block;
  font-size: 16px; /* 重置字体大小 */
  margin-right: -4px; /* 关键：消除间隙 */
}
```

#### 3. Flexbox布局（现代推荐方案）
```css
ul {
  display: flex; /* 启用弹性布局 */
  gap: 0; /* 可选：明确设置间隙为0 */
}
li {
  display: inline-block; /* 或直接使用flex项特性 */
}
```

#### 4. 浮动布局（传统方案）
```css
li {
  float: left;
  display: block;
}
ul::after {
  content: "";
  display: table;
  clear: both; /* 清除浮动 */
}
```

#### 5. 设置父元素font-size:0（高效方案）
```css
ul {
  font-size: 0; /* 消除空白节点 */
}
li {
  display: inline-block;
  font-size: 16px; /* 必须重置字体大小 */
}
```

### 最佳实践建议
1. **优先使用Flexbox**（方案3），现代浏览器支持完善且无需计算尺寸
2. 需要兼容旧浏览器时，选择**负边距**（方案2）或**font-size:0**（方案5）
3. 避免使用浮动布局（方案4），除非维护旧项目

> 注意：这些方法同样适用于其他`inline-block`元素（如`<span>`、`<div>`等）的间隙问题。Flexbox方案已成为2023年后的行业标准解决方案。