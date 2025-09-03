# JavaScript 引入方式总结

## 目录
1. #内联方式
2. #外部文件引入
3. #模块化引入
4. #动态加载
5. #注意事项
6. #总结对比

## 内联方式

### 1. 直接在HTML元素中
```html
<button onclick="alert('Hello World!')">点击我</button>
```

### 2. 在`<script>`标签中
```html
<!DOCTYPE html>
<html>
<head>
  <script>
    // JavaScript代码直接写在这里
    function sayHello() {
      alert('Hello World!');
    }
  </script>
</head>
<body>
  <button onclick="sayHello()">点击我</button>
</body>
</html>
```

## 外部文件引入

### 1. 基本外部引入
```html
<!DOCTYPE html>
<html>
<head>
  <!-- 通常放在head中 -->
  <script src="path/to/your-script.js"></script>
</head>
<body>
  <!-- 或者放在body末尾 -->
  <script src="path/to/another-script.js"></script>
</body>
</html>
```

### 2. 使用属性控制加载行为

#### async - 异步加载
```html
<script src="demo_async.js" async></script>
```
- 下载脚本时不阻塞HTML解析
- 下载完成后立即执行，可能会阻塞HTML渲染
- 执行顺序不确定

#### defer - 延迟执行
```html
<script src="demo_defer.js" defer></script>
```
- 下载脚本时不阻塞HTML解析
- 等待HTML解析完成后按顺序执行

#### type="module" - ES6模块
```html
<script type="module" src="main.js"></script>
```
- 支持ES6模块语法
- 默认使用defer行为

## 模块化引入

### 1. ES6模块 (现代标准)
```html
<script type="module">
  import { functionName } from './module.js';
  // 使用导入的函数
</script>
```

### 2. CommonJS (Node.js环境)
```javascript
// 导入
const module = require('./module');
// 导出
module.exports = { /* 导出内容 */ };
```

### 3. AMD (异步模块定义)
```javascript
// 使用RequireJS
define(['dependency'], function(dependency) {
  return { /* 模块内容 */ };
});

require(['module'], function(module) {
  // 使用模块
});
```

## 动态加载

### 1. 动态创建script标签
```javascript
function loadScript(src, callback) {
  const script = document.createElement('script');
  script.src = src;
  script.onload = () => callback(null, script);
  script.onerror = () => callback(new Error(`脚本加载失败: ${src}`));
  document.head.appendChild(script);
}

// 使用
loadScript('path/to/script.js', (err, script) => {
  if (err) {
    // 处理错误
  } else {
    // 脚本加载成功
  }
});
```

### 2. import() 动态导入
```javascript
// 动态导入返回Promise
import('./module.js')
  .then(module => {
    // 使用模块
  })
  .catch(err => {
    // 处理错误
  });

// 或在async函数中使用
async function loadModule() {
  try {
    const module = await import('./module.js');
    // 使用模块
  } catch (err) {
    // 处理错误
  }
}
```

## 注意事项

1. **脚本位置**：
   - 放在`<head>`中：可能阻塞页面渲染
   - 放在`</body>`前：避免阻塞渲染，但DOM加载完成后才能交互

2. **执行顺序**：
   - 普通脚本按文档顺序执行
   - async脚本下载完立即执行，顺序不确定
   - defer脚本按顺序在DOMContentLoaded前执行

3. **跨域问题**：
   - 使用CORS策略处理跨域脚本
   - 添加`crossorigin`属性

4. **性能优化**：
   - 合并脚本减少HTTP请求
   - 使用async/defer避免渲染阻塞
   - 压缩和缓存脚本文件

## 总结对比

| 引入方式 | 优点 | 缺点 | 适用场景 |
|---------|------|------|---------|
| 内联脚本 | 简单直接 | 难以维护，不可缓存 | 小型脚本或演示 |
| 外部脚本 | 可缓存，易维护 | 增加HTTP请求 | 大多数场景 |
| async | 不阻塞渲染 | 执行顺序不确定 | 独立不依赖的脚本 |
| defer | 不阻塞渲染，顺序执行 | 延迟执行 | 依赖DOM或其他脚本 |
| ES6模块 | 模块化，作用域隔离 | 浏览器兼容性 | 现代Web应用 |
| 动态加载 | 按需加载，减少初始负载 | 代码复杂度增加 | 大型应用，优化性能 |

```html
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <title>JavaScript引入方式示例</title>
  <!-- 外部脚本 -->
  <script src="https://code.jquery.com/jquery-3.6.0.min.js"></script>
  
  <!-- 内联脚本 -->
  <script>
    console.log("内联脚本已执行");
  </script>
</head>
<body>
  <h1>JavaScript引入方式示例</h1>
  
  <!-- 元素内联 -->
  <button onclick="alert('按钮被点击!')">点击我</button>
  
  <!-- 模块脚本 -->
  <script type="module">
    import { helperFunction } from './modules/helper.js';
    console.log("ES6模块已加载");
  </script>
  
  <!-- 延迟脚本 -->
  <script src="deferred-script.js" defer></script>
  
  <!-- 异步脚本 -->
  <script src="async-script.js" async></script>
</body>
</html>
```

这份笔记涵盖了JavaScript的主要引入方式及其特点，可以根据具体需求选择最适合的方式。