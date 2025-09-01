# JavaScript Promise 完整指南

## 一、核心概念

### 1. 什么是 Promise？
Promise 是表示异步操作最终完成（或失败）及其结果值的对象。它将异步操作与处理程序关联，使异步方法可以像同步方法一样返回值。

### 2. 三种状态
- **pending**（待定）：初始状态
- **fulfilled**（已兑现）：操作成功完成
- **rejected**（已拒绝）：操作失败

状态一旦改变就不可逆（pending → fulfilled 或 pending → rejected）

## 二、基本用法

### 1. 创建 Promise
```javascript
const promise = new Promise((resolve, reject) => {
  // 异步操作
  if (/* 成功 */) {
    resolve(value); // 状态变为 fulfilled
  } else {
    reject(error); // 状态变为 rejected
  }
});
```

### 2. 使用 Promise
```javascript
promise
  .then(result => {
    // 处理成功结果
  })
  .catch(error => {
    // 处理错误
  })
  .finally(() => {
    // 无论成功失败都会执行
  });
```

## 三、Promise 链
```javascript
doFirstThing()
  .then(result => doSecondThing(result))
  .then(newResult => doThirdThing(newResult))
  .then(finalResult => console.log(finalResult))
  .catch(error => console.error(error));
```

## 四、错误处理策略

### 1. 错误捕获范围差异

#### `.then(success, fail)` 模式
- **只能捕获**：
  - 当前 Promise 的拒绝状态
  - 同一个 `.then()` 中 `success` 回调函数内部抛出的同步错误
- **无法捕获**：
  - 链中前一个 `.then()` 返回的新的 rejected Promise

```javascript
// 局限性示例
asyncFunc()
  .then(
    result => Promise.reject('异步错误'), // 这个错误不会被下一个then的fail捕获
    err => console.log('只处理asyncFunc的错误')
  )
  .then(
    data => console.log(data),
    err => console.error('这里捕获不到上面的异步错误!') // 无法捕获
  );
```

#### `.then(success).catch(fail)` 模式（推荐）
- **能捕获**：
  - 当前 Promise 的拒绝状态
  - 之前所有 `.then()` 或 `.catch()` 中抛出的任何错误（同步/异步拒绝）

```javascript
// 推荐用法
asyncFunc()
  .then(result => {
    throw new Error('同步错误'); // 会被catch捕获
  })
  .then(newResult => Promise.reject('异步错误')) // 也会被catch捕获
  .then(finalResult => console.log(finalResult))
  .catch(error => {
    console.error('捕获所有错误:', error); // 统一处理所有错误
  });
```

### 2. 最佳实践
**始终使用 `.then().catch()` 模式**，在链末尾添加 `.catch()` 作为全局错误屏障。

## 五、Promise 组合方法

### 对比表格

| 方法 | 描述 | 成功条件 | 失败条件 | 返回值（成功） | 返回值（失败） |
|------|------|----------|----------|----------------|----------------|
| **Promise.all** | 全部完成或一个失败 | 所有输入fulfilled | 任一输入rejected | 值数组 | 第一个rejection原因 |
| **Promise.allSettled** | 无论成败悉数汇报 | 所有输入settled | 永不失败 | 状态结果对象数组 | (不适用) |
| **Promise.race** | 胜者为王唯快不破 | 第一个settled的是fulfilled | 第一个settled的是rejected | 第一个成功的值 | 第一个失败的原因 |
| **Promise.any** | 有一个成功即可 | 任一输入fulfilled | 所有输入rejected | 第一个成功的值 | AggregateError |

### 详细说明

#### 1. Promise.all
```javascript
// 全部成功才继续
Promise.all([promise1, promise2, promise3])
  .then(values => console.log(values))
  .catch(error => console.error(error)); // 一个失败就停止
```

#### 2. Promise.allSettled
```javascript
// 等待所有完成，不关心成败
Promise.allSettled([promise1, promise2, promise3])
  .then(results => {
    results.forEach(result => {
      if (result.status === 'fulfilled') {
        console.log('成功:', result.value);
      } else {
        console.log('失败:', result.reason);
      }
    });
  });
```

#### 3. Promise.race
```javascript
// 竞速模式，取第一个完成的结果
const timeout = new Promise((_, reject) => 
  setTimeout(() => reject('超时'), 5000));
Promise.race([fetchData(), timeout])
  .then(data => console.log(data))
  .catch(error => console.error(error));
```

#### 4. Promise.any
```javascript
// 取第一个成功的结果
Promise.any([api1, api2, api3])
  .then(firstSuccess => console.log(firstSuccess))
  .catch(errors => console.error('全部失败:', errors.errors));
```

## 六、静态方法

### 1. Promise.resolve()
```javascript
// 创建立即解决的Promise
Promise.resolve('立即值')
  .then(value => console.log(value));
```

### 2. Promise.reject()
```javascript
// 创建立即拒绝的Promise
Promise.reject('错误原因')
  .catch(error => console.error(error));
```

## 七、最佳实践与常见陷阱

### 1. 避免Promise地狱
```javascript
// 错误：嵌套使用
getUser().then(user => {
  getPosts(user.id).then(posts => {
    // 更深嵌套...
  });
});

// 正确：链式调用
getUser()
  .then(user => getPosts(user.id))
  .then(posts => processPosts(posts))
  .catch(error => console.error(error));
```

### 2. 不要忘记return
```javascript
// 错误：忘记return，下一个then会立即执行
getData()
  .then(data => {
    processData(data); // 没有return！
  })
  .then(result => {
    // result是undefined！
  });

// 正确：返回Promise或值
getData()
  .then(data => processData(data)) // 返回处理结果
  .then(result => console.log(result));
```

### 3. 错误处理统一化
```javascript
asyncOperation()
  .then(step1)
  .then(step2)
  .then(step3)
  .catch(error => {
    // 统一处理所有步骤的错误
    console.error('操作失败:', error);
  });
```

## 八、实用示例

### 1. 超时控制
```javascript
function withTimeout(promise, timeoutMs) {
  const timeoutPromise = new Promise((_, reject) => {
    setTimeout(() => reject(new Error('操作超时')), timeoutMs);
  });
  return Promise.race([promise, timeoutPromise]);
}
```

### 2. 顺序执行异步操作
```javascript
function executeSequentially(promises) {
  return promises.reduce((chain, promise) => {
    return chain.then(() => promise);
  }, Promise.resolve());
}
```

### 3. 批量处理 with 重试
```javascript
function fetchWithRetry(url, retries = 3) {
  return fetch(url).catch(error => {
    return retries > 0 
      ? fetchWithRetry(url, retries - 1)
      : Promise.reject(error);
  });
}
```

## 九、总结

Promise 是现代 JavaScript 异步编程的核心，提供了比回调函数更清晰、更强大的异步控制能力。掌握：

1. **基本用法**：创建、使用、链式调用
2. **错误处理**：理解 `.then(success, fail)` 与 `.then(success).catch(fail)` 的关键差异，**始终使用后者**
3. **组合方法**：根据场景选择合适的组合策略（all/allSettled/race/any）
4. **最佳实践**：避免常见陷阱，编写清晰可靠的异步代码

Promise 是 async/await 的基础，深入理解 Promise 对于掌握现代 JavaScript 开发至关重要。

#### **十、 总结**

| 概念 | 描述 |
| :--- | :--- |
| **状态** | `pending` -> `fulfilled` 或 `pending` -> `rejected`，**不可逆**。 |
| **创建** | `new Promise((resolve, reject) => { ... })` |
| **消费** | `.then(onFulfilled, onRejected)` `.catch(onRejected)` `.finally(onFinally)` |
| **链式调用** | 每次调用 `.then`/`.catch` 都返回**新 Promise**，允许顺序执行异步任务。 |
| **错误处理** | 使用 `.catch` 在链的末尾统一捕获错误，或使用 `.then` 的第二个参数。 |
| **并行处理** | `Promise.all` (全成功)、`Promise.allSettled` (全完成)、`Promise.race` (竞速)、`Promise.any` (第一个成功) |

Promise 是现代 JavaScript 异步编程的基石，是学习 `async/await` 语法的基础。掌握好 Promise 至关重要。