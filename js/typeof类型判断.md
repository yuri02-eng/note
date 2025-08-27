# JavaScript `typeof`操作符完全指南

## 一、基础概念与语法

### 1. 定义

`typeof`是 JavaScript 的一元操作符，用于检测变量或表达式的数据类型。

### 2. 语法

```
typeof operand
typeof(operand) // 括号可选
```

### 3. 返回值

返回表示操作数类型的字符串值。

## 二、返回值类型对照表

| 操作数类型 | 返回值                  | 示例                                          |
| ---------- | ----------------------- | --------------------------------------------- |
| Undefined  | "undefined"             | `typeof undefined`                            |
| Null       | **"object"** (历史遗留) | `typeof null`                                 |
| Boolean    | "boolean"               | `typeof true`                                 |
| Number     | "number"                | `typeof 42`                                   |
| BigInt     | "bigint"                | `typeof 42n`                                  |
| String     | "string"                | `typeof "hello"`                              |
| Symbol     | "symbol"                | `typeof Symbol()`                             |
| Function   | **"function"**          | `typeof function() {}`                        |
| 其他对象   | "object"                | `typeof {}`, `typeof []`, `typeof new Date()` |

## 三、特殊行为详解

### 1. Null 的 "object" 问题

```javascript
typeof null // "object"
```

**原因**：JavaScript 的早期设计错误，为保持兼容性未修复

**解决方案**：

```javascript
const isNull = value => value === null;
```

### 2. 函数的 "function" 类型

```javascript
typeof function() {} // "function"
typeof class {} // "function"
```

**说明**：函数是特殊的可调用对象，`typeof`单独处理

### 3. 未声明变量

```javascript
typeof undeclaredVar // "undefined" (不会报错)
```

**应用**：安全检查变量是否存在

```javascript
if (typeof jQuery !== 'undefined') {
  // 安全使用 jQuery
}
```

### 4. 数组和对象

```javascript
typeof [] // "object"
typeof {} // "object"
typeof new Date() // "object"
```

**区分方法**：

```javascript
Array.isArray([]) // true
{} instanceof Object // true
```

## 四、实际应用场景

### 1. 类型检查

```javascript
function processValue(value) {
  if (typeof value === 'string') {
    return value.toUpperCase();
  }
  if (typeof value === 'number') {
    return value * 2;
  }
  throw new Error('Unsupported type');
}
```

### 2. 函数参数验证

```javascript
function fetchData(url, callback) {
  if (typeof url !== 'string') {
    throw new TypeError('URL must be a string');
  }
  if (typeof callback !== 'function') {
    throw new TypeError('Callback must be a function');
  }
  // ...执行逻辑
}
```

### 3. 特性检测

```javascript
// 检测浏览器是否支持 WebAssembly
if (typeof WebAssembly === 'object') {
  // 使用 WebAssembly
} else {
  // 回退方案
}
```

### 4. 可选链安全访问

```javascript
// 安全访问深层属性
const userName = typeof user?.profile?.name === 'string' 
  ? user.profile.name 
  : 'Anonymous';
```

## 五、局限性及解决方案

### 1. 无法区分对象类型

**问题**：

```javascript
typeof [] // "object"
typeof {} // "object"
typeof new Date() // "object"
```

**解决方案**：

```javascript
// 使用 Object.prototype.toString
Object.prototype.toString.call([]) // "[object Array]"
Object.prototype.toString.call({}) // "[object Object]"
Object.prototype.toString.call(new Date()) // "[object Date]"

// 封装工具函数
function getType(value) {
  return Object.prototype.toString.call(value)
    .slice(8, -1)
    .toLowerCase();
}

getType([]) // "array"
getType(null) // "null"
getType(undefined) // "undefined"
```

### 2. 无法检测自定义类实例

```javascript
class Person {}
const john = new Person();

typeof john // "object"
```

**解决方案**：

```javascript
john instanceof Person // true
john.constructor.name // "Person"
```

### 3. 包装对象问题

```
typeof new String("hello") // "object"
typeof String("hello") // "string"
```

**最佳实践**：避免使用包装对象

## 六、进阶类型检测技术

### 1. 组合检测方法

```javascript
function typeOf(value) {
  if (value === null) return 'null';
  const type = typeof value;
  if (type !== 'object') return type;
  
  // 处理特殊对象类型
  if (Array.isArray(value)) return 'array';
  if (value instanceof Date) return 'date';
  if (value instanceof RegExp) return 'regexp';
  
  return 'object';
}
```

### 2. ES6+ 类型检测

```javascript
// 使用 Symbol.toStringTag
class CustomClass {
  get [Symbol.toStringTag]() {
    return 'CustomClass';
  }
}

Object.prototype.toString.call(new CustomClass()) // "[object CustomClass]"
```

### 3. 类型检查库推荐

```javascript
// 使用第三方库
import { is } from 'typly';

is.array([]); // true
is.promise(Promise.resolve()); // true
is.iterable(new Map()); // true
```

## 七、新增类型检测

### 1. BigInt 检测

```javascript
typeof 123n // "bigint"
typeof BigInt(123) // "bigint"
```

### 2. Symbol 检测

```javascript
typeof Symbol('id') // "symbol"
typeof Symbol.iterator // "symbol"
```

### 3. 异步函数检测

```javascript
async function fetchData() {}
typeof fetchData // "function"
```

### 4. Generator 函数检测

```javascript
function* gen() {}
typeof gen // "function"
```

## 八、最佳实践总结

1. **基本类型检查**：优先使用 `typeof`

   ```javascript
   if (typeof value === 'string') { ... }
   ```

2. **Null 检查**：使用严格相等

   ```javascript
   if (value === null) { ... }
   ```

3. **数组检查**：使用 `Array.isArray()`

```javascript
   if (Array.isArray(value)) { ... }
```

4. **自定义类型**：使用 `instanceof`

```javascript
   if (value instanceof MyClass) { ... }
```

5. **精确类型检测**：使用 `Object.prototype.toString.call()`

   ```javascript
   if (Object.prototype.toString.call(value) === '[object Date]') { ... }
   ```

6. **安全特性检测**：结合 `typeof`和存在性检查

   ```javascript
   if (typeof IntersectionObserver !== 'undefined') {
     // 安全使用
   }
   ```

7. **避免过时方法**：

   ```javascript
   // 不要使用
   value.constructor === Array
   
   // 使用
   Array.isArray(value)
   ```

8. **ES6+ 优先**：

   ```javascript
   // 使用
   Number.isInteger(value)
   
   // 而不是
   typeof value === 'number' && value % 1 === 0
   ```

## 九、综合示例：高级类型检测函数

```javascript
function getType(value) {
  // 处理 null
  if (value === null) return 'null';
  
  // 获取基本类型
  const type = typeof value;
  
  // 处理非对象类型
  if (type !== 'object') return type;
  
  // 处理特殊对象类型
  if (Array.isArray(value)) return 'array';
  if (value instanceof Date) return 'date';
  if (value instanceof RegExp) return 'regexp';
  if (value instanceof Promise) return 'promise';
  if (value instanceof Map) return 'map';
  if (value instanceof Set) return 'set';
  
  // 处理 Symbol 对象
  if (typeof value.valueOf() === 'symbol') return 'symbol';
  
  // 处理自定义类
  if (value.constructor && value.constructor.name !== 'Object') {
    return value.constructor.name.toLowerCase();
  }
  
  // 默认返回 object
  return 'object';
}

// 测试
getType(null) // "null"
getType(undefined) // "undefined"
getType(42) // "number"
getType(42n) // "bigint"
getType("text") // "string"
getType(true) // "boolean"
getType(Symbol()) // "symbol"
getType([]) // "array"
getType({}) // "object"
getType(new Date()) // "date"
getType(/regex/) // "regexp"
getType(() => {}) // "function"
getType(new Map()) // "map"
getType(Promise.resolve()) // "promise"

class Person {}
getType(new Person()) // "person"
```

## 十、总结

`typeof`操作符是 JavaScript 类型系统的基础，但有其局限性：

- 快速检测基本类型的首选工具
- 无法区分对象的具体类型
- `null`返回 "object" 是历史遗留问题
- 函数返回 "function" 是特殊处理

在实际开发中：

1. 理解 `typeof`的行为和限制
2. 结合其他方法进行精确类型检测
3. 优先使用 ES6+ 提供的类型检测方法
4. 在性能敏感场景选择合适的方法
5. 对于复杂类型系统，考虑使用 TypeScript

掌握 `typeof`及其补充方法，能够帮助开发者编写更健壮、可维护的 JavaScript 代码。