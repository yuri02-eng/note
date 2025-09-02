# JavaScript `Object.create()` 简明指南

## 一、基本概念

`Object.create()` 是 JavaScript 中用于**创建新对象并指定其原型**的方法。

```javascript
const newObj = Object.create(proto, [properties]);
```

## 二、核心功能

### 1. 创建原型链
```javascript
const parent = { value: 10 };
const child = Object.create(parent);

console.log(child.value); // 10（通过原型链继承）
```

### 2. 创建纯净对象
```javascript
const pureObj = Object.create(null);
console.log(pureObj.toString); // undefined（无继承方法）
```

### 3. 定义属性特性
```javascript
const obj = Object.create({}, {
    name: {
        value: 'John',
        writable: false,
        enumerable: true
    }
});
```

## 三、实现继承

### 1. 简单原型继承
```javascript
const Animal = { eat() { return "eating"; } };
const Dog = Object.create(Animal);
Dog.bark = function() { return "woof!"; };

const myDog = Object.create(Dog);
console.log(myDog.eat()); // "eating"
```

### 2. 构造函数组合
```javascript
function Person(name) { this.name = name; }
Person.prototype.greet = function() { return `Hello, ${this.name}`; };

function Employee(name, title) {
    Person.call(this, name);
    this.title = title;
}

// 建立原型链
Employee.prototype = Object.create(Person.prototype);
Employee.prototype.constructor = Employee;
```

## 四、实际应用

### 1. 安全对象扩展
```javascript
// 不污染原生原型
const SafeArray = Object.create(Array.prototype);
SafeArray.customMethod = function() { /* ... */ };
```

### 2. 配置对象
```javascript
const config = Object.create(null, {
    apiKey: { value: 'secret', writable: false },
    endpoint: { value: 'https://api.example.com', enumerable: true }
});
```

## 五、注意事项

1. 使用 `Object.create(null)` 创建无原型对象
2. 属性描述符默认不可写、不可枚举、不可配置
3. 原型链不宜过深（建议 ≤ 3 层）
4. 现代类语法底层使用 `Object.create`

## 六、与相关方法对比

| 方法 | 特点 | 适用场景 |
|------|------|----------|
| `Object.create()` | 直接设置原型，支持属性描述符 | 纯净继承，安全对象 |
| `new Constructor()` | 调用构造函数，自动设置原型 | 需要初始化逻辑 |
| `Object.setPrototypeOf()` | 修改已有对象原型 | 动态修改原型链 |

## 七、最佳实践

1. 优先使用 `Object.create(null)` 创建字典对象
2. 使用属性描述符定义不可变属性
3. 保持原型链扁平简洁
4. 结合工厂函数创建对象

`Object.create()` 是 JavaScript 原型系统的核心，理解其机制对于掌握现代 JavaScript 开发至关重要。