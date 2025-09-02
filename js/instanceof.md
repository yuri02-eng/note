# JavaScript `instanceof` 操作符原理详解

## 一、`instanceof` 的基本概念

### 1. 定义
`instanceof` 是 JavaScript 中的二元操作符，用于**检测构造函数的 `prototype` 属性是否出现在对象的原型链中**。

### 2. 语法
```javascript
object instanceof constructor
```

### 3. 返回值
- 如果 `constructor.prototype` 存在于 `object` 的原型链上，返回 `true`
- 否则返回 `false`

## 二、`instanceof` 的核心原理

### 1. 原型链检测机制
`instanceof` 通过检查对象的整个原型链来确定对象是否是指定构造函数的实例：

```javascript
function myInstanceof(obj, constructor) {
    // 获取对象的原型
    let proto = Object.getPrototypeOf(obj);
    
    // 遍历原型链
    while (proto !== null) {
        // 如果找到构造函数的 prototype
        if (proto === constructor.prototype) {
            return true;
        }
        // 继续向上查找
        proto = Object.getPrototypeOf(proto);
    }
    
    return false;
}
```

### 2. 内部执行步骤
1. 获取对象的原型：`Object.getPrototypeOf(obj)`
2. 获取构造函数的 `prototype` 属性
3. 沿着对象的原型链向上查找：
   - 如果当前原型等于构造函数的 `prototype`，返回 `true`
   - 如果到达原型链末端（`null`），返回 `false`

## 三、原型链图示说明

```
        Object.prototype
              ↑
        Person.prototype
              ↑
        john (Person实例)
```

```javascript
john instanceof Person; // true
john instanceof Object; // true
```

## 四、手写实现 `instanceof`

### 1. 基础实现
```javascript
function myInstanceof(obj, constructor) {
    // 基本类型直接返回 false
    if (obj === null || typeof obj !== 'object') {
        return false;
    }
    
    // 获取对象的原型
    let proto = Object.getPrototypeOf(obj);
    
    while (true) {
        // 到达原型链末端
        if (proto === null) return false;
        
        // 找到匹配的原型
        if (proto === constructor.prototype) return true;
        
        // 继续向上查找
        proto = Object.getPrototypeOf(proto);
    }
}
```

### 2. 完整实现（支持边界情况）
```javascript
function myInstanceof(obj, constructor) {
    // 1. 处理基本类型
    if (obj === null || typeof obj !== 'object' && typeof obj !== 'function') {
        return false;
    }
    
    // 2. 处理构造函数不是函数的情况
    if (typeof constructor !== 'function') {
        throw new TypeError('Right-hand side of instanceof is not callable');
    }
    
    // 3. 获取对象的原型
    let proto = Object.getPrototypeOf(obj);
    
    // 4. 遍历原型链
    while (proto !== null) {
        if (proto === constructor.prototype) {
            return true;
        }
        proto = Object.getPrototypeOf(proto);
    }
    
    return false;
}
```

## 五、关键注意事项

### 1. 基本类型处理
基本类型值（非对象）使用 `instanceof` 总是返回 `false`：

```javascript
'string' instanceof String;    // false
123 instanceof Number;          // false
true instanceof Boolean;        // false
```

### 2. 对象与包装对象
```javascript
const str = new String('hello');
str instanceof String;  // true
str instanceof Object; // true
```

### 3. 原型修改的影响
修改构造函数的 `prototype` 会影响所有后续创建的实例：

```javascript
function Person() {}
const p1 = new Person();
console.log(p1 instanceof Person); // true

// 修改原型
Person.prototype = {};
const p2 = new Person();
console.log(p1 instanceof Person); // false（原型链已断开）
console.log(p2 instanceof Person); // true
```

### 4. 跨框架/窗口问题
不同 iframe 或 window 中的相同构造函数不相等：

```javascript
// 不同 iframe 中的 Array
const iframe = document.createElement('iframe');
document.body.appendChild(iframe);
const iframeArray = iframe.contentWindow.Array;

const arr = [1, 2, 3];
console.log(arr instanceof Array);          // true
console.log(arr instanceof iframeArray);    // false
```

## 六、实际应用场景

### 1. 类型检测
```javascript
function checkType(value) {
    if (value instanceof Array) {
        return 'Array';
    }
    if (value instanceof Date) {
        return 'Date';
    }
    if (value instanceof RegExp) {
        return 'RegExp';
    }
    return 'Other';
}
```

### 2. 自定义类型检测
```javascript
class Animal {}
class Dog extends Animal {}

const dog = new Dog();
console.log(dog instanceof Dog);    // true
console.log(dog instanceof Animal);  // true
```

### 3. 安全类型检查
```javascript
function safeArrayCheck(arr) {
    if (!(arr instanceof Array)) {
        throw new TypeError('Expected an Array');
    }
    // 安全操作数组
}
```

## 七、与 `typeof` 和 `constructor` 的比较

| 方法 | 优点 | 缺点 |
|------|------|------|
| `typeof` | 快速检测基本类型 | 无法区分对象类型（所有对象返回 "object"） |
| `constructor` | 可获取构造函数引用 | 易被修改，不可靠 |
| `instanceof` | 检测原型链关系 | 跨框架问题，无法检测基本类型 |

```javascript
const arr = [];
console.log(typeof arr);            // "object"（不具体）
console.log(arr.constructor);       // Array（但可被修改）
console.log(arr instanceof Array);  // true（原型链检测）
```

## 八、特殊边界情况

### 1. `Object.create(null)`
```javascript
const obj = Object.create(null);
console.log(obj instanceof Object); // false（没有原型链）
```

### 2. 修改 `Symbol.hasInstance`
ES6 允许自定义 `instanceof` 行为：

```javascript
class MyClass {
    static instance {
        return 'custom' in instance;
    }
}

const obj = { custom: true };
console.log(obj instanceof MyClass); // true
```

### 3. 原型为 `null`
```javascript
function Foo() {}
Foo.prototype = null;

const obj = new Foo();
console.log(obj instanceof Foo); 
// 抛出 TypeError: Right-hand side of 'instanceof' is not callable
```

## 九、总结

`instanceof` 的核心原理：
1. 基于原型链的检测机制
2. 检查构造函数的 `prototype` 是否在对象的原型链上
3. 遍历整个原型链直到找到匹配或到达末端

使用注意事项：
- 不适用于基本类型检测
- 原型修改会影响检测结果
- 存在跨框架/窗口的限制
- 可通过 `Symbol.hasInstance` 自定义行为

理解 `instanceof` 的底层原理对于掌握 JavaScript 的原型继承机制至关重要，也是实现自定义类型系统和高级设计模式的基础。