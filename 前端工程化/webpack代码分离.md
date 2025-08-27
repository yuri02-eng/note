Webpack 的代码分离（Code Splitting）是优化前端应用性能的核心技术，**通过将代码拆分为多个按需加载的 Bundle，显著提升页面加载速度和用户体验。**以下是三种主要方法及其配置详解：

------

### ⚙️ **一、入口起点（Entry Points）**

**原理**：通过配置多个入口文件，手动分离代码。
 ​**​适用场景​**​：简单项目或需明确分割独立功能模块的场景。
 ​**​配置示例​**​：

```
// webpack.config.js
module.exports = {
  entry: {
    app: './src/app.js',    // 主业务逻辑
    vendor: './src/vendor.js' // 第三方库（如 lodash）
  },
  output: {
    filename: '[name].bundle.js', // 输出 app.bundle.js 和 vendor.bundle.js
    path: path.resolve(__dirname, 'dist')
  }
};
```

**缺点**：

- 入口间重复依赖的模块会**被重复打包**，导致体积冗余；
- 灵活性低，无法动态拆分逻辑代码。

------

### 🔄 **二、防止重复（SplitChunksPlugin）**

**原理**：自动提取公共模块（如第三方库或复用组件），避免重复打包。
 ​**​适用场景​**​：多入口项目或需优化缓存命中率的场景。
 ​**​配置示例​**​：

```
// webpack.config.js
module.exports = {
  optimization: {
    splitChunks: {
      chunks: 'all', // 处理同步和异步代码
      minSize: 20000, // 模块最小体积（20KB）
      cacheGroups: {
        vendors: {
          test: /[\\/]node_modules[\\/]/, // 提取 node_modules 中的模块
          filename: 'vendors.js', // 输出文件名
          priority: -10 // 优先级
        },
        common: {
          minChunks: 2, // 被引用 2 次以上的模块
          filename: 'common.js',
          priority: -20
        }
      }
    }
  }
};
```

**优势**：

- 自动优化重复代码，减少 Bundle 体积；
- 支持按需提取第三方库（如 `lodash`、`react`）。

------

### ⏳ **三、动态导入（Dynamic Imports）**

**原理**：通过 `import()` 语法动态加载模块，实现按需加载（懒加载），预获取/预加载。
 ​**​适用场景​**​：路由级分割（如 React/Vue 单页应用）、交互触发的功能模块。
 ​**​配置示例​**​：

```
// 动态加载模块（点击按钮时加载）
button.addEventListener('click', () => {
  import(/* webpackChunkName: "lodash" */ 'lodash')
    .then(({ default: _ }) => {
      console.log(_.join(['Hello', 'webpack'], ' '));
    });
});
```

**高级技巧**：

1. 魔法注释：

   - `webpackChunkName`：自定义输出文件名（如 `lodash.bundle.js`）；
   - `webpackPrefetch: true`：浏览器空闲时预加载资源，提升后续访问速度。

2. React 懒加载示例：

   ```
   const Home = React.lazy(() => import('./pages/Home'));
   <Suspense fallback={<div>Loading...</div>}>
     <Home />
   </Suspense>
   ```

**优势**：

- **首屏加载更快**：仅加载必要代码；
- **按需加载**：用户操作时再加载非关键模块。

------

### 🧰 **四、补充优化技巧**

1. **CSS 代码分离**：
    使用 `mini-css-extract-plugin` 将 CSS 提取为独立文件，避免 JS Bundle 过大：

   ```
   const MiniCssExtractPlugin = require('mini-css-extract-plugin');
   module: {
     rules: [{ test: /\.css$/, use: [MiniCssExtractPlugin.loader, 'css-loader'] }]
   }
   ```

2. **缓存优化**：
    在输出文件名中添加内容哈希，利用浏览器缓存：

   ```
   output: {
     filename: '[name].[contenthash].js', // 内容变化时哈希更新
     chunkFilename: '[name].[contenthash].chunk.js'
   }
   ```

3. **分析工具**：
    使用 `webpack-bundle-analyzer` 可视化分析包体积，定位优化点。

------

### 💎 **最佳实践总结**

| **方法**          | **适用场景**            | **优化目标**        |
| ----------------- | ----------------------- | ------------------- |
| 入口起点          | 简单项目/独立功能模块   | 基础分割            |
| SplitChunksPlugin | 多入口/公共库提取       | 减少重复代码        |
| 动态导入          | 路由懒加载/交互触发模块 | 按需加载 & 首屏提速 |

**推荐策略**：

- 首选 **动态导入** 实现按需加载；
- 结合 **SplitChunksPlugin** 提取公共代码；
- 对 CSS/第三方库使用专用分离方案。

> 通过合理组合上述方法，可显著提升应用性能。例如：将 `React`/`Vue` 路由组件动态加载，配合提取公共依赖，首屏加载时间可降低 30%-50%。