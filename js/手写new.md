# JavaScript 的 new 操作符手写实现

## 一、new 操作符的核心机制

当使用 `new` 关键字调用函数时，JavaScript 引擎会执行以下步骤：

1. **创建新对象**：创建一个空的普通 JavaScript 对象
2. **设置原型链**：将新对象的原型链接到构造函数的 prototype 属性
3. **绑定 this 上下文**：将构造函数的 this 绑定到新创建的对象
4. **执行构造函数**：执行构造函数中的代码
5. **返回对象**：
   - 如果构造函数返回一个对象，则返回该对象
   - 否则返回新创建的对象

## 二、手写实现 new 操作符

```javascript
function myNew(constructor, ...args) {
    // 1. 创建一个新对象
    const obj = {};
    
    // 2. 将新对象的原型链接到构造函数的原型
    Object.setPrototypeOf(obj, constructor.prototype);
    // 或者使用：obj.__proto__ = constructor.prototype;
    
    // 3. 执行构造函数，并将 this 绑定到新对象
    const result = constructor.apply(obj, args);
    
    // 4. 判断构造函数是否返回对象
    // 如果返回的是对象则返回该对象，否则返回新创建的对象
    return result instanceof Object ? result : obj;
}
```

## 三、使用示例

### 1. 基本使用

```javascript
function Person(name, age) {
    this.name = name;
    this.age = age;
}

Person.prototype.greet = function() {
    return `Hello, I'm ${this.name}, ${this.age} years old.`;
};

// 使用手写的 myNew
const john = myNew(Person, 'John', 30);

console.log(john.name); // "John"
console.log(john.age); // 30
console.log(john.greet()); // "Hello, I'm John, 30 years old."
```

### 2. 处理构造函数返回值

```javascript
function Car(make, model) {
    this.make = make;
    this.model = model;
    
    // 返回一个对象
    return { type: 'Vehicle' };
}

function Bike(make, model) {
    this.make = make;
    this.model = model;
    
    // 返回非对象值
    return 'This is a bike';
}

const car = myNew(Car, 'Toyota', 'Camry');
console.log(car); // { type: 'Vehicle' } (返回的对象)

const bike = myNew(Bike, 'Honda', 'CBR');
console.log(bike); // Bike { make: 'Honda', model: 'CBR' } (新创建的对象)
```

## 四、实现说明

### 1. 原型链设置方法对比

| 方法 | 说明 | 兼容性 |
|------|------|--------|
| `Object.setPrototypeOf(obj, prototype)` | ES6 标准方法 | 现代浏览器 |
| `obj.__proto__ = prototype` | 非标准但广泛支持 | 大部分环境 |
| `Object.create(prototype)` | 创建时直接设置原型 | IE9+ |

### 2. 返回值处理逻辑

```javascript
// 简化版返回值处理
if (typeof result === 'object' && result !== null) {
    return result;
} else {
    return obj;
}

// 或者更精确的版本
return result instanceof Object ? result : obj;
```

## 五、边界情况处理

### 1. 处理构造函数为箭头函数

```javascript
function myNew(constructor, ...args) {
    if (typeof constructor !== 'function') {
        throw new TypeError(constructor + ' is not a constructor');
    }
    
    // ...其余代码不变
}

// 测试
const ArrowFunc = () => {};
myNew(ArrowFunc); // TypeError: () => {} is not a constructor
```

### 2. 处理基本类型构造函数

```javascript
myNew(123); // TypeError: 123 is not a constructor
myNew('string'); // TypeError: string is not a constructor
```

## 六、完整实现（带边界检查）

```javascript
function myNew(constructor, ...args) {
    // 1. 验证构造函数是否为函数
    if (typeof constructor !== 'function') {
        throw new TypeError(constructor + ' is not a constructor');
    }
    
    // 2. 创建新对象并设置原型
    const obj = Object.create(constructor.prototype);
    
    // 3. 执行构造函数
    const result = constructor.apply(obj, args);
    
    // 4. 处理返回值
    const isObject = result !== null && typeof result === 'object';
    const isFunction = typeof result === 'function';
    
    return isObject || isFunction ? result : obj;
}
```

## 七、实际应用场景

### 1. 自定义对象创建

```javascript
function Product(name, price) {
    this.name = name;
    this.price = price;
}

Product.prototype.getPriceInfo = function() {
    return `${this.name}: $${this.price}`;
};

const laptop = myNew(Product, 'Laptop', 999);
console.log(laptop.getPriceInfo()); // "Laptop: $999"
```

### 2. 实现简单继承

```javascript
function Animal(name) {
    this.name = name;
}

Animal.prototype.speak = function() {
    console.log(`${this.name} makes a noise.`);
};

function Dog(name) {
    Animal.call(this, name);
}

// 设置原型链
Dog.prototype = Object.create(Animal.prototype);
Dog.prototype.constructor = Dog;

Dog.prototype.speak = function() {
    console.log(`${this.name} barks.`);
};

const dog = myNew(Dog, 'Rex');
dog.speak(); // "Rex barks."
```

## 总结

手写实现 `new` 操作符帮助我们深入理解 JavaScript 的对象创建机制：
1. 对象创建和原型链设置是核心
2. 构造函数中的 `this` 绑定是关键步骤
3. 返回值处理需要考虑对象和非对象情况
4. 边界情况处理使实现更健壮

理解这些原理对于掌握 JavaScript 面向对象编程至关重要，也是理解现代框架和类语法的基础。