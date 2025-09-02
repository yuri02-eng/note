# 前端解决跨域请求的完整指南

## 什么是跨域问题？

跨域问题是由浏览器的**同源策略**引起的安全限制。当前端应用尝试从一个域（domain）、端口（port）或协议（protocol）请求资源到另一个不同的域时，就会发生跨域请求限制。

### 同源策略要求三要素一致：
- **协议相同**（http/https）
- **域名相同**（example.com）
- **端口相同**（80/443）

## 🎯 前端解决方案全景图

| 解决方案 | 适用场景 | 前端控制度 | 后端依赖度 | 安全等级 |
|---------|---------|-----------|-----------|----------|
| **CORS** | 生产环境API | 中 | 高 | ⭐⭐⭐⭐⭐ |
| **开发服务器代理** | 开发环境 | 高 | 低 | ⭐⭐⭐⭐ |
| **Nginx反向代理** | 生产环境 | 中 | 中 | ⭐⭐⭐⭐⭐ |
| **JSONP** | 传统项目兼容 | 高 | 中 | ⭐⭐ |

## 一、CORS (跨域资源共享) - 主流方案

### 1. 简单请求与复杂请求

**简单请求**（浏览器自动处理）：
```javascript
// 前端代码无需特殊处理
fetch('https://api.example.com/data')
  .then(response => response.json())
  .then(data => console.log(data));
```

**复杂请求**（需要预检请求）：
```javascript
// 浏览器会自动发送OPTIONS预检请求
fetch('https://api.example.com/data', {
  method: 'PUT',
  headers: {
    'Content-Type': 'application/json',
    'X-Custom-Header': 'value'
  },
  body: JSON.stringify({ data: 'test' })
});
```

### 2. 携带凭证的请求
```javascript
// 前端设置
fetch('https://api.example.com/auth', {
  credentials: 'include', // 包含cookies等凭证
  headers: {
    'Authorization': `Bearer ${token}`
  }
});

// Axios配置
axios.get('https://api.example.com/data', {
  withCredentials: true
});
```

### 3. 错误处理与调试
```javascript
async function safeFetch(url, options = {}) {
  try {
    const response = await fetch(url, {
      credentials: 'include',
      ...options
    });
    
    // 检查CORS是否配置正确
    if (response.status === 0) {
      throw new Error('可能的CORS错误：请检查服务器配置');
    }
    
    if (!response.ok) {
      throw new Error(`HTTP错误: ${response.status}`);
    }
    
    return await response.json();
  } catch (error) {
    console.error('请求失败:', error.message);
    // 这里可以添加重试逻辑或用户提示
    throw error;
  }
}
```

## 二、开发环境解决方案

### 1. Webpack DevServer代理（Create React App/Vue CLI）
```javascript
// vue.config.js / 或webpack.config.js
module.exports = {
  devServer: {
    proxy: {
      // 简单代理
      '/api': {
        target: 'https://api.example.com',
        changeOrigin: true,
        secure: false, // 如果目标是https但证书无效
        logLevel: 'debug' // 查看代理日志
      },
      
      // 多路径代理
      '/service': {
        target: 'https://service.example.com',
        changeOrigin: true,
        pathRewrite: {
          '^/service': '/api' // 重写路径
        }
      }
    }
  }
};
```

### 2. Vite代理配置
```javascript
// vite.config.js
export default defineConfig({
  server: {
    proxy: {
      '/api': {
        target: 'http://localhost:3000',
        changeOrigin: true,
        rewrite: (path) => path.replace(/^\/api/, ''),
        configure: (proxy, options) => {
          // 代理事件监听
          proxy.on('error', (err, req, res) => {
            console.log('代理错误:', err);
          });
        }
      }
    }
  }
})
```

### 3. 环境变量配置
```javascript
// src/config/env.js
const config = {
  development: {
    API_BASE_URL: '/api' // 使用代理
  },
  production: {
    API_BASE_URL: 'https://api.example.com' // 真实API地址
  },
  test: {
    API_BASE_URL: 'https://test-api.example.com'
  }
};

export default config[process.env.NODE_ENV || 'development'];
```

## 三、生产环境解决方案

### 1. Nginx反向代理配置
```nginx
# /etc/nginx/conf.d/my-app.conf
server {
    listen 80;
    server_name myapp.com;
    
    # 前端静态资源
    location / {
        root /usr/share/nginx/html;
        index index.html index.htm;
        try_files $uri $uri/ /index.html;
    }
    
    # API代理配置
    location /api/ {
        proxy_pass https://api.example.com/;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        
        # CORS头（如果后端未设置）
        add_header Access-Control-Allow-Origin $http_origin always;
        add_header Access-Control-Allow-Credentials true always;
        add_header Access-Control-Allow-Methods 'GET, POST, PUT, DELETE, OPTIONS' always;
        add_header Access-Control-Allow-Headers 'DNT,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Range,Authorization' always;
        
        # 处理预检请求
        if ($request_method = 'OPTIONS') {
            add_header Access-Control-Allow-Origin $http_origin;
            add_header Access-Control-Allow-Methods 'GET, POST, PUT, DELETE, OPTIONS';
            add_header Access-Control-Allow-Headers 'DNT,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Range,Authorization';
            add_header Access-Control-Max-Age 1728000;
            add_header Content-Type 'text/plain; charset=utf-8';
            add_header Content-Length 0;
            return 204;
        }
    }
}
```

### 2. CDN反向代理（Cloudflare/AWS CloudFront）
```javascript
// Cloudflare Workers示例
addEventListener('fetch', event => {
  event.respondWith(handleRequest(event.request));
});

async function handleRequest(request) {
  const url = new URL(request.url);
  
  // 代理API请求
  if (url.pathname.startsWith('/api/')) {
    const apiUrl = 'https://api.example.com' + url.pathname.replace('/api', '');
    const modifiedRequest = new Request(apiUrl, {
      headers: request.headers,
      method: request.method,
      body: request.body
    });
    
    return fetch(modifiedRequest);
  }
  
  // 返回静态资源
  return fetch(request);
}
```

## 四、传统与备选方案

### 1. JSONP（仅限GET请求）
```javascript
function jsonp(url, callbackName) {
  return new Promise((resolve, reject) => {
    // 创建全局回调函数
    window[callbackName] = function(data) {
      resolve(data);
      delete window[callbackName];
      document.body.removeChild(script);
    };
    
    // 设置超时
    const timeout = setTimeout(() => {
      reject(new Error('JSONP请求超时'));
      delete window[callbackName];
      if (script.parentNode) {
        document.body.removeChild(script);
      }
    }, 10000);
    
    // 创建script标签
    const script = document.createElement('script');
    script.src = `${url}${url.includes('?') ? '&' : '?'}callback=${callbackName}`;
    script.onerror = () => {
      reject(new Error('JSONP请求失败'));
      clearTimeout(timeout);
    };
    
    document.body.appendChild(script);
  });
}

// 使用示例
jsonp('https://api.example.com/users', 'handleUsers')
  .then(data => console.log(data))
  .catch(error => console.error(error));
```

### 2. 浏览器扩展与调试方案
```javascript
// 仅开发阶段使用的方法

// 1. 浏览器启动参数（Chrome）
// chrome --disable-web-security --user-data-dir=/tmp/chrome-test

// 2. 使用浏览器插件
// - CORS Unblock（开发工具）
// - Moesif Origin & CORS Changer

// 3. 本地HTTPS证书（解决混合内容问题）
// 使用mkcert生成本地证书
```

## 五、实战最佳实践

### 1. 智能请求封装
```javascript
// src/utils/api.js
class ApiClient {
  constructor(baseURL) {
    this.baseURL = baseURL;
  }
  
  async request(endpoint, options = {}) {
    const url = `${this.baseURL}${endpoint}`;
    const config = {
      credentials: 'include',
      headers: {
        'Content-Type': 'application/json',
        ...options.headers,
      },
      ...options,
    };
    
    try {
      const response = await fetch(url, config);
      
      // 处理CORS相关错误
      if (response.type === 'opaque') {
        throw new Error('请求被CORS策略阻止');
      }
      
      if (!response.ok) {
        throw new Error(`HTTP错误: ${response.status}`);
      }
      
      return await response.json();
    } catch (error) {
      if (error.message.includes('CORS')) {
        console.warn('CORS错误，尝试备用方案...');
        return this.fallbackRequest(endpoint, options);
      }
      throw error;
    }
  }
  
  // 备用请求方案
  async fallbackRequest(endpoint, options) {
    // 这里可以实现JSONP或其他备用方案
    console.warn('使用备用请求方案请求:', endpoint);
    // 实现略...
  }
}

// 使用
const api = new ApiClient(process.env.REACT_APP_API_URL);
api.request('/users').then(console.log).catch(console.error);
```

### 2. 环境自适应配置
```javascript
// src/config/api.js
const getApiConfig = () => {
  const isDevelopment = process.env.NODE_ENV === 'development';
  const isProduction = process.env.NODE_ENV === 'production';
  
  // 开发环境使用代理，生产环境使用真实URL
  let baseURL = 'https://api.example.com';
  
  if (isDevelopment) {
    baseURL = ''; // 使用相对路径，由开发服务器代理
  }
  
  if (process.env.REACT_APP_API_BASE_URL) {
    baseURL = process.env.REACT_APP_API_BASE_URL;
  }
  
  return {
    baseURL,
    timeout: 10000,
    withCredentials: true,
    // 其他配置...
  };
};

export const apiConfig = getApiConfig();
```

## 六、安全注意事项

1. **不要随意设置** `Access-Control-Allow-Origin: *`
2. **生产环境**中应指定具体域名：
   ```nginx
   add_header Access-Control-Allow-Origin https://your-domain.com;
   ```
3. **敏感操作**应验证Origin头：
   ```javascript
   // 后端应验证请求来源
   const allowedOrigins = ['https://your-domain.com', 'https://admin.your-domain.com'];
   if (allowedOrigins.includes(req.headers.origin)) {
     res.setHeader('Access-Control-Allow-Origin', req.headers.origin);
   }
   ```
4. **避免**在生产环境使用JSONP

## 七、故障排查指南

### 常见错误与解决方案：
1. **预检请求失败**：检查服务器OPTIONS处理
2. **凭证被拒绝**：确保`Allow-Credentials`为true且Origin不是`*`
3. **HTTPS到HTTP**：浏览器阻止混合内容请求
4. **证书问题**：确保SSL证书有效

### 调试技巧：
```javascript
// 查看详细请求信息
fetch('https://api.example.com/data')
  .then(response => {
    console.log('响应头:', Object.fromEntries(response.headers.entries()));
    console.log('状态:', response.status, response.statusText);
    return response.json();
  })
  .catch(error => {
    console.error('完整错误:', error);
  });
```

## 总结

前端解决跨域问题的选择策略：

1. **开发环境**：使用框架提供的代理功能（Webpack/Vite）
2. **生产环境**：
   - 首选：Nginx反向代理 + 正确CORS配置
   - 备选：CDN边缘函数处理
3. **传统系统**：JSONP（仅限GET且安全要求不高时）
4. **移动端/Node.js**：通常无跨域限制（非浏览器环境）

**最佳实践**：始终优先与后端团队协作，正确配置CORS策略，这既安全又符合Web标准。前端代理方案主要适用于开发环境和特定部署场景。