# JavaScript 作用域链的确定机制

## 一、作用域链的本质

作用域链是 JavaScript 中变量和函数查找的机制，它决定了代码在执行时如何访问标识符（变量、函数名等）。作用域链的确定基于**词法作用域**（静态作用域）原则。

## 二、词法作用域 vs 动态作用域

### 1. 词法作用域（JavaScript采用）
- **定义时确定**：函数的作用域在函数定义时就已经确定
- **静态的**：基于代码的书写位置
- **可预测的**：通过查看代码结构就能确定作用域关系

### 2. 动态作用域（JavaScript不采用）
- **运行时确定**：函数的作用域在函数调用时才确定
- **动态的**：基于函数的调用位置
- **不可预测**：需要知道函数的调用方式才能确定作用域

```javascript
// 词法作用域示例
var x = 10;

function foo() {
  console.log(x); // 总是访问全局的x，与调用方式无关
}

function bar() {
  var x = 20;
  foo(); // 输出10，而不是20
}

bar();
```

## 三、作用域链的创建过程

### 1. 函数定义阶段：记录外部引用

当函数被定义时，它会**保存对其外部词法环境的引用**，这个引用就是函数的作用域链基础。

```javascript
var globalVar = 'global';

function outer() {
  var outerVar = 'outer';
  
  // inner函数定义时，会记录outer的作用域作为其外部引用
  function inner() {
    var innerVar = 'inner';
    console.log(innerVar, outerVar, globalVar);
  }
  
  return inner;
}
```

### 2. 函数调用阶段：构建完整作用域链

当函数被调用时，JavaScript 引擎会：
1. 创建函数的执行上下文
2. 创建活动对象（包含参数、局部变量等）
3. 将活动对象添加到作用域链的前端
4. 将函数定义时保存的外部引用作为作用域链的后续部分

```javascript
var globalVar = 'global';

function outer() {
  var outerVar = 'outer';
  
  function inner() {
    var innerVar = 'inner';
    console.log(innerVar, outerVar, globalVar);
  }
  
  return inner;
}

var innerFunc = outer();
innerFunc(); // 输出: inner outer global

// inner函数执行时的作用域链：
// [inner的活动对象] -> [outer的活动对象] -> [全局变量对象]
```

## 四、内部实现原理：[[Environment]] 属性

从 ECMAScript 规范的角度，每个函数都有一个内部属性 `[[Environment]]`，它在函数创建时被设置，指向函数被定义时的词法环境。

### 1. 函数创建时

```javascript
var x = 10;

function foo() {
  var y = 20;
  
  // bar函数创建时，其[[Environment]]指向foo的执行环境
  function bar() {
    console.log(x, y);
  }
  
  return bar;
}
```

### 2. 函数调用时

当函数被调用时，会创建一个新的词法环境，其外部引用（outer reference）指向函数的 `[[Environment]]` 属性值。

```javascript
var barFunc = foo();
barFunc(); // 输出: 10 20

// bar函数调用时的环境关系：
// bar的词法环境 -> foo的词法环境 -> 全局词法环境
```

## 五、变量查找的具体过程

当访问一个变量时，JavaScript 引擎会沿着作用域链逐级查找：

```javascript
var globalVar = 'global';

function level1() {
  var level1Var = 'level1';
  
  function level2() {
    var level2Var = 'level2';
    
    function level3() {
      var level3Var = 'level3';
      
      // 变量查找过程：
      console.log(level3Var); // 1. 在当前作用域找到
      console.log(level2Var); // 2. 在level2作用域找到
      console.log(level1Var); // 3. 在level1作用域找到
      console.log(globalVar); // 4. 在全局作用域找到
      console.log(undefinedVar); // 5. 抛出ReferenceError
    }
    
    level3();
  }
  
  level2();
}

level1();
```

## 六、闭包与作用域链

闭包是函数和其词法环境的组合，作用域链机制使得闭包成为可能。

```javascript
function createCounter() {
  var count = 0; // 这个变量会被闭包"记住"
  
  return {
    increment: function() {
      count++;
      return count;
    },
    getCount: function() {
      return count;
    }
  };
}

var counter = createCounter();
console.log(counter.getCount()); // 0
console.log(counter.increment()); // 1
console.log(counter.getCount()); // 1

// increment和getCount函数的作用域链：
// [函数的活动对象] -> [createCounter的活动对象] -> [全局变量对象]
// 因此它们可以访问createCounter的count变量
```

## 七、ES6 块级作用域的影响

`let` 和 `const` 引入了块级作用域，这对作用域链产生了影响。

```javascript
function demonstrateBlockScope() {
  var varVariable = 'function scoped';
  let letVariable = 'block scoped';
  
  if (true) {
    var innerVar = 'still function scoped'; // 提升到函数作用域
    let innerLet = 'truly block scoped'; // 只在块内有效
    
    console.log(letVariable); // 可以访问外部块的let变量
    console.log(innerLet); // 可以访问当前块的变量
  }
  
  console.log(innerVar); // 可以访问，因为var提升
  // console.log(innerLet); // ReferenceError: innerLet is not defined
}

demonstrateBlockScope();
```

## 八、实际代码中的查找过程示例

```javascript
// 全局作用域
var globalVar = 'global';

function outer(outerParam) {
  // outer函数作用域
  var outerVar = 'outer';
  
  function inner(innerParam) {
    // inner函数作用域
    var innerVar = 'inner';
    
    // 变量查找过程演示
    console.log(innerVar);    // 1. 当前作用域 → "inner"
    console.log(innerParam);   // 2. 当前作用域 → inner参数值
    console.log(outerVar);     // 3. outer作用域 → "outer"
    console.log(outerParam);   // 4. outer作用域 → outer参数值
    console.log(globalVar);    // 5. 全局作用域 → "global"
    
    // 函数查找同样遵循作用域链
    someFunction();            // 6. 全局作用域 → 找到全局函数
  }
  
  inner('innerValue');
}

function someFunction() {
  console.log('Global function called');
}

outer('outerValue');
```

## 九、引擎内部的实现机制

虽然具体实现因 JavaScript 引擎而异，但基本原理类似：

### 1. 环境记录（Environment Record）
每个执行上下文都有一个环境记录，用于存储变量和函数声明。

### 2. 外部环境引用（Outer Environment Reference）
每个环境记录都有一个指向外部环境的引用，形成作用域链。

### 3. 变量解析算法
当需要解析一个标识符时：
1. 检查当前环境记录中是否存在该标识符
2. 如果存在，返回对应的值
3. 如果不存在，沿着外部环境引用链向上查找
4. 如果到达全局环境仍未找到，抛出 ReferenceError

```javascript
// 伪代码表示变量查找过程
function resolveIdentifier(identifier, currentEnvironment) {
  while (currentEnvironment !== null) {
    if (currentEnvironment.hasBinding(identifier)) {
      return currentEnvironment.getBindingValue(identifier);
    }
    currentEnvironment = currentEnvironment.outer;
  }
  throw new ReferenceError(identifier + " is not defined");
}
```

## 十、总结

作用域链的确定基于以下核心原则：

1. **词法性**：作用域在代码书写时（定义时）确定，而不是运行时
2. **层级性**：内层作用域可以访问外层作用域的变量，但外层不能访问内层
3. **链式结构**：作用域形成一条链，变量查找沿这条链进行
4. **闭包支持**：函数保持对其定义时作用域的引用，即使在其他位置执行

理解作用域链的机制对于掌握 JavaScript 的变量查找、闭包、模块模式等高级概念至关重要。这种基于词法作用域的链式查找机制是 JavaScript 语言设计的核心特性之一。