# JavaScript 构造函数与 new 机制 - 学习笔记

## 一、构造函数

### 1. 什么是构造函数
构造函数是用于创建特定类型对象的特殊函数，通常以大写字母开头（命名约定）。

```javascript
// 构造函数示例
function Person(name, age) {
    this.name = name;
    this.age = age;
    this.greet = function() {
        return `Hello, my name is ${this.name}`;
    };
}
```

### 2. 构造函数的特点
- 通常使用帕斯卡命名法（首字母大写）
- 使用 `this` 关键字来设置属性
- 不需要显式返回对象（由 `new` 自动处理）

## 二、new 关键字机制

### 1. new 操作符的作用
当使用 `new` 调用函数时，会发生以下步骤：

```javascript
const person = new Person('John', 30);
```

### 2. new 操作符的内部机制（4个步骤）

1. **创建新对象**：创建一个空对象 `{}`
2. **设置原型**：将新对象的 `__proto__` 指向构造函数的 `prototype`
3. **绑定this**：将构造函数内的 `this` 指向新创建的对象
4. **返回对象**：如果构造函数没有返回对象，则返回新创建的对象

```javascript
// new 关键字的模拟实现
function myNew(constructor, ...args) {
    // 1. 创建新对象
    const obj = {};
    
    // 2. 设置原型链
    Object.setPrototypeOf(obj, constructor.prototype);
    
    // 3. 执行构造函数，绑定this
    const result = constructor.apply(obj, args);
    
    // 4. 返回对象（如果构造函数返回对象则使用该对象）
    return result instanceof Object ? result : obj;
}
```

## 三、构造函数与原型

### 1. 原型方法
为了节省内存，方法通常定义在原型上而非构造函数内部：

```javascript
function Person(name, age) {
    this.name = name;
    this.age = age;
}

// 在原型上添加方法
Person.prototype.greet = function() {
    return `Hello, my name is ${this.name}`;
};

Person.prototype.getAge = function() {
    return this.age;
};
```

### 2. 检测对象类型

```javascript
const person = new Person('John', 30);

console.log(person instanceof Person); // true
console.log(person.constructor === Person); // true
```

## 四、构造函数返回值

### 1. 默认情况
构造函数通常不返回值，`new` 会自动返回新创建的对象。

### 2. 返回对象
如果构造函数返回一个对象，则该对象会替代默认创建的对象：

```javascript
function Person(name) {
    this.name = name;
    return { custom: 'object' }; // 返回这个对象而不是新创建的Person实例
}

const person = new Person('John');
console.log(person); // { custom: 'object' }
```

### 3. 返回非对象
如果返回的是非对象值，则忽略返回值，仍然返回新创建的对象：

```javascript
function Person(name) {
    this.name = name;
    return 'some string'; // 被忽略
}

const person = new Person('John');
console.log(person); // Person { name: 'John' }
```

## 五、注意事项

### 1. 忘记使用 new 关键字

```javascript
// 错误用法
const person = Person('John', 30); // 没有使用new
console.log(name); // 'John' (污染全局命名空间)
console.log(person); // undefined

// 安全措施：防止忘记new
function Person(name, age) {
    if (!(this instanceof Person)) {
        return new Person(name, age);
    }
    this.name = name;
    this.age = age;
}
```

### 2. 箭头函数不能作为构造函数
箭头函数没有自己的 `this` 绑定，不能使用 `new` 调用：

```javascript
const Person = (name) => {
    this.name = name; // 错误：箭头函数没有this
};

// 会抛出错误
const person = new Person('John');
```

## 六、实际应用示例

### 1. 创建自定义对象类型

```javascript
function Car(make, model, year) {
    this.make = make;
    this.model = model;
    this.year = year;
}

Car.prototype.getInfo = function() {
    return `${this.year} ${this.make} ${this.model}`;
};

const myCar = new Car('Toyota', 'Camry', 2020);
console.log(myCar.getInfo()); // "2020 Toyota Camry"
```

### 2. 继承实现（ES5方式）

```javascript
// 父构造函数
function Animal(name) {
    this.name = name;
}

Animal.prototype.speak = function() {
    console.log(`${this.name} makes a noise.`);
};

// 子构造函数
function Dog(name, breed) {
    Animal.call(this, name); // 调用父构造函数
    this.breed = breed;
}

// 设置原型链
Dog.prototype = Object.create(Animal.prototype);
Dog.prototype.constructor = Dog;

Dog.prototype.speak = function() {
    console.log(`${this.name} barks.`);
};

const dog = new Dog('Rex', 'German Shepherd');
dog.speak(); // "Rex barks."
```

## 总结

- 构造函数是用于创建对象的特殊函数，通常以大写字母开头
- `new` 关键字会自动处理对象创建、原型绑定和返回对象
- 方法应定义在原型上以提高内存效率
- 注意处理忘记使用 `new` 的情况
- 构造函数是 JavaScript 面向对象编程的基础，理解其机制对掌握 JS 至关重要