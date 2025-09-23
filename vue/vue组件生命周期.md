# Vue 3 生命周期系统详解

## 一、生命周期概述

Vue 组件从创建到销毁的整个过程被称为生命周期，在这个过程中 Vue 提供了一系列的"钩子函数"（Lifecycle Hooks），允许开发者在特定阶段添加自己的代码。

![组件生命周期图示](https://cn.vuejs.org/assets/lifecycle_zh-CN.W0MNXI0C.png)

## 二、生命周期图示与阶段划分

### 完整生命周期流程

```
初始化
  │
  ├── 初始化事件和生命周期
  │     │
  │     └── beforeCreate
  │
  ├── 初始化注入和响应性
  │     │
  │     └── created
  │
  ├── 编译模板（如果有）
  │
  ├── 挂载阶段
  │     │
  │     ├── beforeMount
  │     │
  │     ├── 创建虚拟DOM并挂载
  │     │
  │     └── mounted
  │
  ├── 更新阶段（循环，可能多次执行）
  │     │
  │     ├── 数据变化
  │     │
  │     ├── beforeUpdate
  │     │
  │     ├── 重新渲染和修补虚拟DOM
  │     │
  │     └── updated
  │
  └── 卸载阶段
        │
        ├── beforeUnmount
        │
        ├── 卸载组件和清理
        │
        └── unmounted
```

## 三、选项式API生命周期钩子详解

### 1. beforeCreate
- **调用时机**：实例初始化之后，数据观测和事件配置之前
- **状态**：
  - 数据：未初始化
  - DOM：未生成
  - 事件：未绑定
- **使用场景**：极少使用，可用于一些不依赖数据的初始化

```javascript
export default {
  beforeCreate() {
    console.log('beforeCreate:', this.$data); // undefined
    console.log('beforeCreate:', this.$el); // undefined
  }
}
```

### 2. created
- **调用时机**：实例创建完成，响应式数据、计算属性、方法等已配置
- **状态**：
  - 数据：已初始化（响应式）
  - DOM：未生成
  - 事件：已绑定
- **使用场景**：最常用的钩子，适合发起异步请求、初始化非响应式数据

```javascript
export default {
  data() {
    return {
      message: 'Hello Vue'
    }
  },
  created() {
    console.log('created:', this.message); // "Hello Vue"
    console.log('created:', this.$el); // undefined
    this.fetchData(); // 发起API请求
  },
  methods: {
    fetchData() {
      // 获取数据
    }
  }
}
```

### 3. beforeMount
- **调用时机**：挂载开始之前，render函数首次被调用
- **状态**：
  - 数据：已初始化
  - DOM：未生成（但模板已编译）
- **使用场景**：极少使用，可在服务端渲染时使用

```javascript
export default {
  beforeMount() {
    console.log('beforeMount:', this.$el); // 原始的DOM元素（未渲染）
  }
}
```

### 4. mounted
- **调用时机**：实例挂载完成后
- **状态**：
  - 数据：已初始化
  - DOM：已生成并可访问
- **使用场景**：操作DOM、使用第三方库、发起依赖于DOM的请求
- **注意**：不能保证所有子组件都已挂载，如需等待使用`$nextTick`

```javascript
export default {
  mounted() {
    console.log('mounted:', this.$el); // 已渲染的DOM元素
    this.initChart(); // 初始化图表库
    this.bindEvents(); // 绑定DOM事件
    
    // 如果需要等待所有子组件挂载完成
    this.$nextTick(() => {
      // 所有视图都已渲染
    });
  }
}
```

### 5. beforeUpdate
- **调用时机**：响应式数据变化后，虚拟DOM重新渲染和打补丁之前
- **使用场景**：访问更新前的DOM状态、移除手动添加的事件监听器

```javascript
export default {
  data() {
    return {
      count: 0
    }
  },
  beforeUpdate() {
    console.log('beforeUpdate:', this.count);
    // 保存当前的滚动位置等
  }
}
```

### 6. updated
- **调用时机**：数据更改导致的虚拟DOM重新渲染和打补丁之后
- **使用场景**：执行依赖于更新后DOM的操作
- **注意**：避免在此修改状态，可能导致无限更新循环

```javascript
export default {
  updated() {
    console.log('updated:', this.count);
    // 根据新的DOM状态执行操作
    
    // 危险示例（可能导致循环）：
    // this.count = this.count + 1; // 不要这样做！
  }
}
```

### 7. beforeUnmount
- **调用时机**：卸载组件实例之前（Vue 2中的beforeDestroy）
- **使用场景**：清理工作，如定时器、事件监听、取消网络请求

```javascript
export default {
  data() {
    return {
      timer: null,
      apiRequest: null
    }
  },
  created() {
    this.timer = setInterval(() => {
      console.log('Timer tick');
    }, 1000);
    
    // 模拟API请求
    this.apiRequest = this.fetchData();
  },
  beforeUnmount() {
    // 清除定时器
    clearInterval(this.timer);
    
    // 取消API请求（如果使用可取消的请求库如Axios）
    if (this.apiRequest && this.apiRequest.cancel) {
      this.apiRequest.cancel('Component unmounted');
    }
    
    // 移除事件监听器
    window.removeEventListener('resize', this.handleResize);
  }
}
```

### 8. unmounted
- **调用时机**：卸载组件实例后（Vue 2中的destroyed）
- **使用场景**：执行最终的清理工作

```javascript
export default {
  unmounted() {
    console.log('Component has been unmounted');
    // 执行最终的清理
  }
}
```

### 9. errorCaptured
- **调用时机**：捕获来自后代组件的错误时
- **参数**：错误对象、发生错误的组件实例、错误来源信息
- **返回值**：如果返回false，阻止错误继续向上传播

```javascript
export default {
  errorCaptured(err, vm, info) {
    console.error('Error captured:', err);
    console.log('Component:', vm);
    console.log('Error info:', info);
    
    // 发送错误到日志服务
    this.logErrorToService(err);
    
    // 阻止错误继续向上传播
    return false;
  }
}
```

## 四、组合式API生命周期钩子

在setup()函数中使用生命周期函数：

```javascript
import {
  onBeforeMount,
  onMounted,
  onBeforeUpdate,
  onUpdated,
  onBeforeUnmount,
  onUnmounted,
  onErrorCaptured,
  ref
} from 'vue';

export default {
  setup() {
    const count = ref(0);
    
    // created阶段的替代方案 - 直接在setup中执行代码
    console.log('This runs in the created phase equivalent');
    const apiData = ref(null);
    
    // 模拟异步请求
    fetchData().then(data => {
      apiData.value = data;
    });
    
    onBeforeMount(() => {
      console.log('onBeforeMount');
    });
    
    onMounted(() => {
      console.log('onMounted');
      // DOM操作和第三方库初始化
    });
    
    onBeforeUpdate(() => {
      console.log('onBeforeUpdate', count.value);
    });
    
    onUpdated(() => {
      console.log('onUpdated', count.value);
    });
    
    onBeforeUnmount(() => {
      console.log('onBeforeUnmount');
      // 清理工作
    });
    
    onUnmounted(() => {
      console.log('onUnmounted');
    });
    
    onErrorCaptured((err, vm, info) => {
      console.error('Error captured:', err);
      return false;
    });
    
    return {
      count,
      apiData
    };
  }
};
```

## 五、父子组件生命周期顺序

### 挂载过程
1. 父组件 beforeCreate
2. 父组件 created
3. 父组件 beforeMount
4. 子组件 beforeCreate
5. 子组件 created
6. 子组件 beforeMount
7. 子组件 mounted
8. 父组件 mounted

### 更新过程
1. 父组件 beforeUpdate
2. 子组件 beforeUpdate
3. 子组件 updated
4. 父组件 updated

### 卸载过程
1. 父组件 beforeUnmount
2. 子组件 beforeUnmount
3. 子组件 unmounted
4. 父组件 unmounted

## 六、实用技巧与最佳实践

### 1. 异步请求的位置
- 使用场景决定位置：
  - 不依赖DOM：created或setup()
  - 依赖DOM：mounted或onMounted
- SSR考虑：在服务端渲染时，mounted不会执行

### 2. 清理工作的重要性
```javascript
// 选项式API示例
export default {
  data() {
    return {
      timer: null,
      eventHandler: null
    }
  },
  mounted() {
    // 添加事件监听
    this.eventHandler = this.handleEvent.bind(this);
    window.addEventListener('resize', this.eventHandler);
    
    // 设置定时器
    this.timer = setInterval(this.doSomething, 1000);
  },
  beforeUnmount() {
    // 必须清理！
    window.removeEventListener('resize', this.eventHandler);
    clearInterval(this.timer);
  }
}

// 组合式API示例
import { onMounted, onBeforeUnmount } from 'vue';

export default {
  setup() {
    let timer = null;
    const eventHandler = () => {
      // 处理事件
    };
    
    onMounted(() => {
      window.addEventListener('resize', eventHandler);
      timer = setInterval(() => {
        // 定时操作
      }, 1000);
    });
    
    onBeforeUnmount(() => {
      window.removeEventListener('resize', eventHandler);
      clearInterval(timer);
    });
  }
}
```

### 3. 避免在updated中修改状态
```javascript
// 错误示例 - 可能导致无限循环
export default {
  updated() {
    if (this.value > 10) {
      this.value = 0; // 触发新的更新
    }
  }
}

// 正确做法 - 在数据源处控制
export default {
  methods: {
    updateValue(newValue) {
      this.value = newValue > 10 ? 0 : newValue;
    }
  }
}
```

### 4. 使用nextTick处理DOM更新后操作
```javascript
export default {
  methods: {
    updateMessage() {
      this.message = 'Updated';
      this.$nextTick(() => {
        // 现在DOM已更新
        console.log('DOM updated:', this.$el.textContent);
      });
    }
  }
}
```

## 七、生命周期钩子的调试和性能分析

### 1. 使用生命周期钩子进行调试
```javascript
export default {
  created() {
    console.log('Component created:', this.$options.name);
  },
  mounted() {
    console.log('Component mounted:', this.$el);
  },
  updated() {
    console.log('Component updated');
  },
  unmounted() {
    console.log('Component unmounted');
  }
}
```

### 2. 性能监控
```javascript
export default {
  data() {
    return {
      createdTime: 0,
      mountedTime: 0
    }
  },
  created() {
    this.createdTime = performance.now();
  },
  mounted() {
    this.mountedTime = performance.now();
    console.log(`创建到挂载耗时: ${this.mountedTime - this.createdTime}ms`);
  }
}
```

## 总结

Vue 3的生命周期提供了组件从创建到销毁的完整控制流程。理解每个钩子的调用时机和适用场景对于开发高质量Vue应用至关重要。选项式API和组合式API提供了不同的使用方式，但底层生命周期流程是一致的。合理使用生命周期钩子，特别是正确进行资源清理，可以避免内存泄漏和其他常见问题。

通过掌握生命周期，您可以更精确地控制组件行为，优化性能，并创建更健壮的Vue应用程序。