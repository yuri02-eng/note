# JavaScript 原型与原型链完整指南

在 JavaScript 中，每一个函数都具有原型，**原型（Prototype）** 是一个对象，它是其他对象继承属性和方法的模板。每个 JavaScript 对象都有一个内置的 `[[Prototype]]`属性（在浏览器中可通过 `__proto__`访问），该属性指向它的原型对象，从而形成一条链式的继承结构，即**原型链（Prototype Chain）**。当访问一个对象的属性时，引擎会首先在对象自身查找，若未找到，则会沿其 `__proto__`指针向上遍历整条原型链，直至找到该属性或到达终点 `null`。

而 **class** 是 ES6 引入的一个语法糖，它并未改变 JavaScript 基于原型的本质，而是提供了一种更清晰、类似于传统面向对象语言的语法来编写构造函数和处理继承。其底层实现依然依赖于原型链：类中定义的方法实则被挂载到构造函数的 `prototype`属性上，`extends`关键字则是通过正确设置子类 `prototype`和 `__proto__`的指向来建立继承关系，`super`关键字则提供了访问父类构造函数和方法的便捷途径。


## 一、原型系统的基本概念

### 1. 原型（Prototype）的本质
JavaScript 中的每个对象都有一个内部属性 `[[Prototype]]`（可通过 `__proto__` 访问），它指向另一个对象，即该对象的"原型"。

```javascript
const obj = {};
console.log(obj.__proto__ === Object.prototype); // true
```

### 2. 原型链（Prototype Chain）
当访问对象的属性时，JavaScript 引擎会：
1. 首先在对象自身属性中查找
2. 如果没有找到，则沿着 `[[Prototype]]` 链向上查找
3. 直到找到属性或到达链的末端（`null`）

```javascript
const grandparent = { a: 1 };
const parent = { b: 2 };
const child = { c: 3 };

// 设置原型链
Object.setPrototypeOf(parent, grandparent);
Object.setPrototypeOf(child, parent);

console.log(child.a); // 1 (通过原型链找到)
console.log(child.b); // 2 (通过原型链找到)
console.log(child.c); // 3 (自身属性)
```

## 二、构造函数、原型与实例的关系

### 1. 三要素关系
```javascript
function Person(name) {
  this.name = name;
}

// 1. 构造函数：Person
// 2. 原型对象：Person.prototype
// 3. 实例对象：person

const person = new Person('John');

console.log(person.__proto__ === Person.prototype); // true
console.log(Person.prototype.constructor === Person); // true
```

### 2. new 操作符的内部机制
```javascript
// new Person() 的底层实现大致如下：
function newOperator(Constructor, ...args) {
  // 1. 创建一个新对象，其原型指向构造函数的原型
  const obj = Object.create(Constructor.prototype);
  
  // 2. 执行构造函数，将this绑定到新对象
  const result = Constructor.apply(obj, args);
  
  // 3. 如果构造函数返回对象，则返回该对象；否则返回新对象
  return typeof result === 'object' ? result : obj;
}
```

## 三、原型继承的实现方式

### 1. 原型链继承
```javascript
function Parent() {
  this.parentProp = 'parent';
}

Parent.prototype.parentMethod = function() {
  return 'parent method';
};

function Child() {
  this.childProp = 'child';
}

// 设置原型链继承
Child.prototype = new Parent();

const instance = new Child();
console.log(instance.parentMethod()); // 'parent method'
```

### 2. 构造函数继承
```javascript
function Parent(name) {
  this.name = name;
  this.colors = ['red', 'blue'];
}

function Child(name, age) {
  Parent.call(this, name); // 借用构造函数
  this.age = age;
}

const child1 = new Child('Tom', 10);
const child2 = new Child('Jerry', 8);

// 解决引用类型共享问题
child1.colors.push('green');
console.log(child2.colors); // ['red', 'blue'] (不共享)
```

### 3. 组合继承（最常用）
```javascript
function Parent(name) {
  this.name = name;
}

Parent.prototype.sayName = function() {
  return this.name;
};

function Child(name, age) {
  Parent.call(this, name); // 继承实例属性
  this.age = age;
}

// 继承原型方法
Child.prototype = Object.create(Parent.prototype);
Child.prototype.constructor = Child;

const child = new Child('Tom', 10);
console.log(child.sayName()); // 'Tom'
```

### 4. 寄生组合式继承（最优方案）
```javascript
function inheritPrototype(child, parent) {
  const prototype = Object.create(parent.prototype);
  prototype.constructor = child;
  child.prototype = prototype;
}

function Parent(name) {
  this.name = name;
}

Parent.prototype.sayName = function() {
  return this.name;
};

function Child(name, age) {
  Parent.call(this, name);
  this.age = age;
}

inheritPrototype(Child, Parent);

const child = new Child('Tom', 10);
console.log(child.sayName()); // 'Tom'
```

## 四、Class 语法与原型的关系

### 1. Class 是原型继承的语法糖
```javascript
// Class 语法
class Person {
  constructor(name) {
    this.name = name;
  }
  
  sayName() {
    return this.name;
  }
  
  static staticMethod() {
    return 'Static method';
  }
}

// 等效的原型写法
function Person(name) {
  this.name = name;
}

Person.prototype.sayName = function() {
  return this.name;
};

Person.staticMethod = function() {
  return 'Static method';
};
```

### 2. Class 继承的底层实现
```javascript
class Parent {
  constructor(name) {
    this.name = name;
  }
}

class Child extends Parent {
  constructor(name, age) {
    super(name); // 相当于 Parent.call(this, name)
    this.age = age;
  }
}

// 底层原型链设置
console.log(Child.__proto__ === Parent); // true
console.log(Child.prototype.__proto__ === Parent.prototype); // true
```

### 3. super 关键字的本质
```javascript
class Child extends Parent {
  constructor() {
    super(); // 相当于 Parent.call(this)
  }
  
  method() {
    super.parentMethod(); // 相当于 Parent.prototype.parentMethod.call(this)
  }
}
```

## 五、原型相关方法与属性

### 1. 原型操作方法
```javascript
function Animal() {}
function Dog() {}

// 设置原型
Object.setPrototypeOf(Dog.prototype, Animal.prototype);

// 获取原型
console.log(Object.getPrototypeOf(Dog.prototype) === Animal.prototype); // true

// 检查原型关系
console.log(Animal.prototype.isPrototypeOf(Dog.prototype)); // true
```

### 2. 属性描述符与原型
```javascript
function Person() {}

// 定义不可枚举的原型属性
Object.defineProperty(Person.prototype, 'species', {
  value: 'human',
  writable: false,
  enumerable: false,
  configurable: false
});

const person = new Person();
console.log(person.species); // 'human'

// 尝试修改（静默失败）
person.species = 'alien';
console.log(person.species); // 'human'
```

### 3. 对象创建与原型
```javascript
// 创建没有原型的对象
const objWithoutProto = Object.create(null);
console.log(objWithoutProto.__proto__); // undefined
console.log(Object.getPrototypeOf(objWithoutProto)); // null

// 创建有指定原型的对象
const proto = { method() { return 'proto method'; } };
const obj = Object.create(proto);
console.log(obj.method()); // 'proto method'
```

## 六、属性查找与屏蔽规则

### 1. 属性查找过程
```javascript
const grandparent = { a: 1 };
const parent = { b: 2 };
const child = { c: 3 };

Object.setPrototypeOf(parent, grandparent);
Object.setPrototypeOf(child, parent);

// 查找过程：child → parent → grandparent → Object.prototype → null
console.log(child.a); // 1 (找到于grandparent)
console.log(child.toString); // [Function: toString] (找到于Object.prototype)
```

### 2. 属性屏蔽规则
```javascript
function Proto() {
  this.value = 'proto value';
}

function Obj() {}
Obj.prototype = new Proto();

const obj = new Obj();

// 情况1：原型链上存在可写的数据属性
obj.value = 'new value'; // 在obj上创建新属性，屏蔽原型属性
console.log(obj.value); // 'new value'

// 情况2：原型链上存在只读属性
Object.defineProperty(Obj.prototype, 'readOnly', {
  value: 'cannot change',
  writable: false
});

obj.readOnly = 'try change'; // 严格模式会报错，非严格模式静默失败
console.log(obj.readOnly); // 'cannot change'
```

## 七、原型链的特殊情况

### 1. 内置对象的原型链
```javascript
const arr = [1, 2, 3];
// 原型链：arr → Array.prototype → Object.prototype → null
console.log(arr.__proto__ === Array.prototype); // true
console.log(Array.prototype.__proto__ === Object.prototype); // true

const func = function() {};
// 原型链：func → Function.prototype → Object.prototype → null
console.log(func.__proto__ === Function.prototype); // true
console.log(Function.prototype.__proto__ === Object.prototype); // true
```

### 2. 函数的多重角色
```javascript
function Person() {}

// 函数作为构造函数
const person = new Person();

// 函数作为对象
Person.customProperty = 'custom';

console.log(Person.__proto__ === Function.prototype); // true
console.log(Person.prototype.__proto__ === Object.prototype); // true
console.log(person.__proto__ === Person.prototype); // true
```

## 八、最佳实践与常见问题

### 1. 性能优化
```javascript
// 避免过长的原型链
function createDeepChain(depth) {
  let current = {};
  for (let i = 0; i < depth; i++) {
    current = Object.create(current);
  }
  return current;
}

const deepObj = createDeepChain(1000);

// 属性查找在长原型链上性能较差
// 解决方案：缓存常用方法引用
const neededMethod = deepObj.someMethod;
neededMethod.call(deepObj);
```

### 2. 安全实践
```javascript
// 1. 使用Object.create()而不是__proto__
const proto = { method() {} };
const obj = Object.create(proto); // ✅ 推荐

// 2. 使用Object.getPrototypeOf()而不是__proto__
console.log(Object.getPrototypeOf(obj)); // ✅ 推荐

// 3. 避免修改内置对象的原型
// ❌ 危险
Array.prototype.customMethod = function() {};

// ✅ 安全
class MyArray extends Array {
  customMethod() {}
}
```

### 3. 常见问题解决方案
```javascript
// 1. 原型链污染防护
function safeMerge(target, source) {
  for (let key in source) {
    if (source.hasOwnProperty(key) && key !== '__proto__') {
      target[key] = source[key];
    }
  }
  return target;
}

// 2. 构造函数指向修复
function Parent() {}
function Child() {}

Child.prototype = new Parent();
console.log(Child.prototype.constructor === Parent); // true ❌

// 修复构造函数指向
Child.prototype.constructor = Child;
console.log(Child.prototype.constructor === Child); // true ✅
```

## 九、现代 JavaScript 中的原型

### 1. Class 语法的最佳实践
```javascript
class Person {
  constructor(name) {
    this.name = name;
  }
  
  // 实例方法（添加到原型）
  sayName() {
    return this.name;
  }
  
  // 静态方法（添加到构造函数）
  static createAnonymous() {
    return new Person('Anonymous');
  }
  
  // 私有字段（ES2022+）
  #privateField = 'secret';
  
  getSecret() {
    return this.#privateField;
  }
}

const person = new Person('John');
console.log(person.sayName()); // 'John'
console.log(Person.createAnonymous()); // Person { name: 'Anonymous' }
```

### 2. 组合使用原型与现代特性
```javascript
// 使用Symbol定义"私有"方法
const internalMethod = Symbol('internalMethod');

class MyClass {
  constructor() {
    this.publicProperty = 'public';
  }
  
   {
    return 'internal logic';
  }
  
  publicMethod() {
    return this;
  }
}

const instance = new MyClass();
console.log(instance.publicMethod()); // 'internal logic'
// console.log(instance); // 理论上可以访问，但通过Symbol增加了难度
```

## 十、总结与选择指南

### 核心要点
1. **原型是JavaScript的继承机制**：每个对象都有指向原型的链接
2. **原型链决定属性查找**：查找沿链向上直到找到或到达null
3. **Class是语法糖**：底层仍然是基于原型的继承
4. **多种继承方式**：从原型链继承到ES6 class继承

### 选择指南
- **简单对象结构**：使用对象字面量 `{}`
- **需要继承**：使用 `class` 和 `extends`
- **需要灵活原型**：使用 `Object.create()`
- **需要最大性能**：在关键代码中直接使用原型操作
- **团队协作**：优先使用Class语法，更易理解

### 性能提示
- 避免过长的原型链
- 缓存常用方法引用
- 在性能关键代码中，直接操作原型可能更快

原型机制是JavaScript的核心特性，理解它对于掌握JavaScript的面向对象编程至关重要。随着ES6 class语法的普及，原型操作变得更加直观和安全，但底层机制仍然是基于原型链的。