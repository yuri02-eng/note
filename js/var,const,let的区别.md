# JavaScript 变量声明：var, let, const 全面解析

## 一、核心区别总结

| 特性             | var                        | let                   | const                              |
| ---------------- | -------------------------- | --------------------- | ---------------------------------- |
| **作用域**       | 函数作用域                 | 块级作用域            | 块级作用域                         |
| **变量提升**     | 提升并初始化为 `undefined` | 提升但不初始化（TDZ） | 提升但不初始化（TDZ）              |
| **重复声明**     | 允许                       | 不允许                | 不允许                             |
| **初始值**       | 可选                       | 可选                  | 必须                               |
| **全局对象属性** | 是（绑定到window对象）     | 否                    | 否                                 |
| **重新赋值**     | 允许                       | 允许                  | 不允许（基本类型）                 |
| **循环中的行为** | 共享同一个变量             | 每次迭代创建新变量    | 每次迭代创建新变量（不可重新赋值） |

## 二、细节解释

### 1. 作用域机制

| 类型      | 作用域类型 | 示例说明                     |
| --------- | ---------- | ---------------------------- |
| **var**   | 函数作用域 | 在函数内声明，整个函数可访问 |
| **let**   | 块级作用域 | 在 `{}`内声明，仅块内可访问  |
| **const** | 块级作用域 | 在 `{}`内声明，仅块内可访问  |

```javascript
// var 的函数作用域
function varScope() {
  if (true) {
    var a = 10;
  }
  console.log(a); // 10（函数内可访问）
}

// let/const 的块级作用域
{
  let b = 20;
  const c = 30;
}
console.log(b); // ReferenceError
console.log(c); // ReferenceError
```

### 2. 声明特性对比

**暂时性死区（TDZ）** 是 JavaScript 中与 `let`和 `const`声明相关的特殊行为区域，指从**进入作用域**到**变量声明**之间的区域，在此区域内访问变量会导致 `ReferenceError`。   

相当于let,const会进行变量提升，这样在变量被赋值前进行访问，例如执行console.log(这个变量)就会输出ReferenceError，有声明但没有值，形成暂时性死区，变量无法使用。

| 特性           | var                        | let                   | const                 |
| -------------- | -------------------------- | --------------------- | --------------------- |
| **变量提升**   | 提升并初始化为 `undefined` | 提升但不初始化（TDZ） | 提升但不初始化（TDZ） |
| **暂时性死区** | 无                         | 有                    | 有                    |
| **重复声明**   | 允许                       | 不允许                | 不允许                |

```javascript
// 变量提升示例
console.log(varValue); // undefined
var varValue = 1;

console.log(letValue); // ReferenceError
let letValue = 2;

// 重复声明示例
var x = 1;
var x = 2; // 允许

let y = 1;
let y = 2; // SyntaxError

const z = 1;
const z = 2; // SyntaxError
```

### 3. 赋值特性对比

| 特性             | var  | let  | const                    |
| ---------------- | ---- | ---- | ------------------------ |
| **初始化要求**   | 可选 | 可选 | 必须                     |
| **重新赋值**     | 允许 | 允许 | 不允许（基本类型）       |
| **对象属性修改** | 允许 | 允许 | 允许                     |
| **完全不变性**   | 无   | 无   | 需配合 `Object.freeze()` |

```javascript
// 初始化要求
var a; // undefined
let b; // undefined
const c; // SyntaxError

// 重新赋值
var d = 1;
d = 2; // 允许

let e = 1;
e = 2; // 允许

const f = 1;
f = 2; // TypeError

// 对象属性修改
const obj = { value: 1 };
obj.value = 2; // 允许
obj = {}; // TypeError

// 完全不变性
const frozen = Object.freeze({ value: 1 });
frozen.value = 2; // 静默失败（严格模式报错）
```

## 三、特殊场景行为分析

### 1. 全局作用域声明

| 类型      | 成为全局对象属性 | 示例                          |
| --------- | ---------------- | ----------------------------- |
| **var**   | 是               | `window.varName`（浏览器）    |
| **let**   | 否               | `window.letName`= undefined   |
| **const** | 否               | `window.constName`= undefined |

```javascript
var globalVar = 1;
let globalLet = 2;
const globalConst = 3;

console.log(window.globalVar); // 1（浏览器环境）
console.log(window.globalLet); // undefined
console.log(window.globalConst); // undefined
```

### 2. 循环中的行为差异

| 循环类型      | var          | let            | const                          |
| ------------- | ------------ | -------------- | ------------------------------ |
| **for 循环**  | 共享同一变量 | 每次迭代新变量 | 每次迭代新变量（不可重新赋值） |
| **for-in/of** | 共享同一变量 | 每次迭代新变量 | 每次迭代新变量                 |

```javascript
// for 循环
for (var i = 0; i < 3; i++) {
  setTimeout(() => console.log(i)); // 3,3,3
}

for (let j = 0; j < 3; j++) {
  setTimeout(() => console.log(j)); // 0,1,2
}

// for-in/of 循环
const arr = [10, 20, 30];

for (var value of arr) {
  setTimeout(() => console.log(value)); // 30,30,30
}

for (let value of arr) {
  setTimeout(() => console.log(value)); // 10,20,30
}

for (const value of arr) {
  console.log(value); // 10,20,30（值不可改但每次迭代新绑定）
}
```

### 3. 函数作用域的特殊情况

```javascript
// 立即执行函数表达式（IIFE）
(function() {
  var privateVar = "secret";
})();
console.log(privateVar); // ReferenceError

// 块级函数声明（ES6）
{
  function blockFunc() { return "block"; }
}
console.log(blockFunc()); // "block"（非严格模式）
```

## 四、最佳实践指南

### 1. 声明选择策略

| 场景                   | 推荐声明 | 示例                          |
| ---------------------- | -------- | ----------------------------- |
| **常量值**             | const    | `const PI = 3.14;`            |
| **循环计数器**         | let      | `for (let i = 0; ...)`        |
| **需要重新赋值的变量** | let      | `let counter = 0; counter++;` |
| **避免使用**           | var      | 仅在遗留代码中使用            |

### 2. 作用域管理策略

```javascript
// 使用块作用域限制变量可见性
{
  const temp = calculateValue();
  // 只在当前块使用 temp
}

// 函数内使用块作用域
function processData(data) {
  // 步骤1：数据准备
  {
    const cleaned = cleanData(data);
    // ...
  }
  
  // 步骤2：数据处理
  {
    const transformed = transformData(data);
    // ...
  }
}
```

### 3. 不变性管理策略

```javascript
// 基本类型常量
const MAX_SIZE = 100;

// 对象常量（属性可变）
const config = {
  apiUrl: "https://api.example.com",
  timeout: 5000
};

// 完全不可变对象
const immutableConfig = Object.freeze({
  apiUrl: "https://api.example.com",
  timeout: 5000
});
```

### 4. 迁移和重构策略

```javascript
// 重构前（使用 var）
function oldFunction() {
  var count = 0;
  for (var i = 0; i < 5; i++) {
    count += i;
  }
  return count;
}

// 重构后（使用 const/let）
function newFunction() {
  let count = 0;
  for (let i = 0; i < 5; i++) {
    count += i;
  }
  return count;
}
```

## 五、高级主题

### 1. 暂时性死区（TDZ）原理

```javascript
// TDZ 开始
console.log(value); // ReferenceError

// 变量声明（TDZ 结束）
let value = 42;
```

### 2. 闭包中的变量捕获

```javascript
// var 的问题
for (var i = 0; i < 3; i++) {
  setTimeout(() => console.log(i), 100); // 3,3,3
}

// let 的解决方案
for (let j = 0; j < 3; j++) {
  setTimeout(() => console.log(j), 100); // 0,1,2
}

// 使用闭包模拟块作用域
for (var k = 0; k < 3; k++) {
  (function(index) {
    setTimeout(() => console.log(index), 100); // 0,1,2
  })(k);
}
```

### 3. 模块模式中的使用

```javascript
// 模块模式
const MyModule = (function() {
  // 私有变量（使用 const/let）
  const privateVar = "secret";
  let counter = 0;
  
  // 公有API
  return {
    increment: function() {
      counter++;
      return counter;
    },
    getSecret: function() {
      return privateVar;
    }
  };
})();
```

## 六、总结对比表

| 特性           | var                        | let         | const            |
| -------------- | -------------------------- | ----------- | ---------------- |
| **作用域**     | 函数级                     | 块级        | 块级             |
| **提升行为**   | 提升并初始化为 `undefined` | 提升（TDZ） | 提升（TDZ）      |
| **重复声明**   | 允许                       | 禁止        | 禁止             |
| **初始化要求** | 可选                       | 可选        | 必须             |
| **重新赋值**   | 允许                       | 允许        | 禁止（基本类型） |
| **全局属性**   | 是                         | 否          | 否               |
| **循环行为**   | 共享变量                   | 每次新变量  | 每次新变量       |
| **推荐指数**   | ★☆☆☆☆                      | ★★★★☆       | ★★★★★            |

## 七、迁移路线图

1. **识别所有 var 声明**

2. **确定变量是否需要重新赋值**
   - 不需要重新赋值 → 改为 const
   - 需要重新赋值 → 改为 let

3. **检查作用域范围**
   - 确保块级作用域变量不会泄漏

4. **处理循环变量**
   - 将 for 循环中的 var 改为 let
   - 将 for-in/of 循环中的 var 改为 const 或 let

5. **添加静态检查**

```javascript
// .eslintrc.json
{
  "rules": {
    "no-var": "error",
    "prefer-const": "error"
  }
}
```

通过系统化理解 var、let 和 const 的特性差异，开发者可以编写出更安全、更可维护的 JavaScript 代码，有效避免变量提升、作用域泄漏等常见问题。