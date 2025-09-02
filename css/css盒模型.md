# CSS盒模型对比笔记

## 基本概念
CSS盒模型是网页布局的基础框架，定义了元素如何计算宽度和高度。主要分为两种模型：

### 标准盒模型 (W3C盒模型)
- **CSS属性**: `box-sizing: content-box;`
- **宽度计算**: width = 内容宽度
- **高度计算**: height = 内容高度
- **元素实际尺寸**: width + padding + border
- **默认行为**: 现代浏览器的默认模式

### IE盒模型 (怪异盒模型)
- **CSS属性**: `box-sizing: border-box;`
- **宽度计算**: width = 内容宽度 + padding + border
- **高度计算**: height = 内容高度 + padding + border
- **元素实际尺寸**: 设置的width和height
- **默认行为**: IE5及更早版本的默认模式

## 对比表格
| 特性 | 标准盒模型 (W3C盒模型) | IE盒模型 (怪异盒模型) |
|------|----------------------|---------------------|
| **CSS属性** | `box-sizing: content-box;` | `box-sizing: border-box;` |
| **宽度计算** | width = 内容宽度 | width = 内容宽度 + padding + border |
| **高度计算** | height = 内容高度 | height = 内容高度 + padding + border |
| **元素实际尺寸** | width + padding + border | 设置的width和height |
| **默认行为** | 现代浏览器的默认模式 | IE5及更早版本的默认模式 |

## 计算示例

### 标准盒模型计算
```css
.element {
  box-sizing: content-box;
  width: 200px;
  height: 100px;
  padding: 20px;
  border: 5px solid black;
}
/* 
实际宽度 = 200px + 20px×2 + 5px×2 = 250px
实际高度 = 100px + 20px×2 + 5px×2 = 150px
*/
```

### IE盒模型计算
```css
.element {
  box-sizing: border-box;
  width: 200px;
  height: 100px;
  padding: 20px;
  border: 5px solid black;
}
/* 
内容区域宽度 = 200px - 20px×2 - 5px×2 = 150px
内容区域高度 = 100px - 20px×2 - 5px×2 = 50px
总尺寸仍为200px×100px
*/
```

## 实践建议

1. **现代开发推荐使用IE盒模型**（border-box），因为它更直观且易于控制布局
2. **全局设置IE盒模型**：
   ```css
   *, *::before, *::after {
     box-sizing: border-box;
   }
   ```
3. **注意事项**：
   - 外边距(margin)在两种模型中都不包含在width/height计算中
   - 某些第三方库可能依赖于特定盒模型，需注意兼容性
   - 使用`border-box`时，内容区域尺寸会随padding和border变化

## 浏览器兼容性
- 所有现代浏览器都支持两种盒模型
- `box-sizing`属性在IE8+中完全支持
- 在需要支持旧版IE时，需注意其默认使用怪异模式

理解这两种盒模型的差异对于精确控制网页布局至关重要，特别是在响应式设计中。