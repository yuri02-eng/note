# Map 与 Set 数据结构详解

## 1. Map（映射）

Map 是一种键值对集合，与普通对象相比具有更灵活的特性。

### 1.1 基本用法与特性

```javascript
// 创建 Map
const map = new Map();

// 添加键值对 - Map 可以使用任意类型作为键
map.set('name', 'John');           // 字符串作为键
map.set(1, 'number one');          // 数字作为键
map.set({ id: 1 }, 'object key');  // 对象作为键
map.set([1, 2, 3], 'array key');   // 数组作为键
map.set(true, 'boolean key');      // 布尔值作为键
map.set(() => {}, 'function key'); // 函数作为键

// 获取值
console.log(map.get('name')); // "John"
console.log(map.get(1));      // "number one"

// 检查键是否存在
console.log(map.has(true));   // true

// 获取大小
console.log(map.size);        // 6

// 删除键值对
map.delete(1);

// 清空 Map
// map.clear();
```

**核心特性：**
- Map 的键可以是任意数据类型，包括对象、数组和函数
- Map 保持键值对的插入顺序
- Map 的大小可以通过 size 属性直接获取
- Map 提供了更直观的遍历方法

### 1.2 遍历 Map 的多种方式

```javascript
// 使用 forEach 方法
map.forEach((value, key) => {
  console.log(`${key}: ${value}`);
});

// 使用 for...of 循环遍历键值对
for (let [key, value] of map) {
  console.log(key, value);
}

// 分别遍历键和值
for (let key of map.keys()) {
  console.log('Key:', key);
}

for (let value of map.values()) {
  console.log('Value:', value);
}

// 使用迭代器
const iterator = map.entries();
let next = iterator.next();
while (!next.done) {
  console.log(next.value);
  next = iterator.next();
}
```

### 1.3 Map 与普通对象的区别

| 特性 | Map | 普通对象 |
|------|-----|----------|
| **键的类型** | 任意类型（对象、函数等） | 字符串或 Symbol |
| **键的顺序** | 保持插入顺序 | 无序（ES6后字符串键按插入顺序） |
| **大小获取** | `size` 属性直接获取 | 需要手动计算 `Object.keys(obj).length` |
| **迭代** | 直接可迭代，有 forEach 方法 | 需要 `Object.keys()`、`Object.values()` 等 |
| **性能** | 频繁增删操作时性能更优 | 少量数据时性能稍好 |
| **序列化** | 不能直接 JSON 序列化 | 可直接 JSON 序列化 |
| **原型污染** | 不会受到原型链上的属性影响 | 可能受到原型链属性影响 |

### 1.4 Map 的初始化与转换

```javascript
// 通过二维数组初始化 Map
const map2 = new Map([
  ['name', 'Alice'],
  [1, 'number one'],
  [true, 'boolean key']
]);

// 将 Map 转换为数组
const mapArray = Array.from(map);
console.log(mapArray);

// 将 Map 转换为对象（仅当键为字符串时有效）
const mapObj = Object.fromEntries(map);
console.log(mapObj);
```

## 2. Set（集合）

Set 是一种存储唯一值的集合，自动去重。

### 2.1 基本用法与特性

```javascript
// 创建 Set
const set = new Set();

// 添加值
set.add(1);
set.add('text');
set.add({ name: 'John' });
set.add([1, 2, 3]);
set.add(1); // 重复值，不会被添加

// 检查值是否存在
console.log(set.has('text')); // true

// 获取大小
console.log(set.size); // 4

// 删除值
set.delete(1);

// 清空 Set
// set.clear();
```

**核心特性：**
- Set 存储唯一值，自动去重
- Set 使用严格相等（===）判断值是否相等
- NaN 在 Set 中被视为相等（尽管 NaN !== NaN）
- Set 保持插入顺序

### 2.2 Set 的去重机制

```javascript
const uniqueSet = new Set();

// 不同类型不会去重
uniqueSet.add(1);
uniqueSet.add('1'); // 字符串 '1'，与数字 1 不同

// NaN 的特殊处理
uniqueSet.add(NaN);
uniqueSet.add(NaN); // NaN 被视为相等，只保留一个

// 对象引用不同，不会去重
uniqueSet.add({ id: 1 });
uniqueSet.add({ id: 1 }); // 两个不同的对象，都会保留

console.log(uniqueSet.size); // 5
```

### 2.3 Set 的遍历方法

```javascript
// 使用 forEach
set.forEach(value => {
  console.log(value);
});

// 使用 for...of
for (let value of set) {
  console.log(value);
}

// 转换为数组后遍历
const setArray = Array.from(set);
setArray.forEach(value => {
  console.log(value);
});
```

### 2.4 Set 的初始化与转换

```javascript
// 通过数组初始化 Set
const set2 = new Set([1, 2, 3, 4, 4, 5]);
console.log(set2.size); // 5，去重后

// 将 Set 转换为数组
const uniqueNumbers = [...set2];
console.log(uniqueNumbers); // [1, 2, 3, 4, 5]
```

### 2.5 Set 的数学运算应用

```javascript
// 求两个数组的并集、交集、差集
const setA = new Set([1, 2, 3, 4]);
const setB = new Set([3, 4, 5, 6]);

// 并集 - 所有不重复的元素
const union = new Set([...setA, ...setB]);
console.log([...union]); // [1, 2, 3, 4, 5, 6]

// 交集 - 两个集合共有的元素
const intersection = new Set([...setA].filter(x => setB.has(x)));
console.log([...intersection]); // [3, 4]

// 差集 (A - B) - 属于A但不属于B的元素
const difference = new Set([...setA].filter(x => !setB.has(x)));
console.log([...difference]); // [1, 2]

// 对称差集 - 属于A或B但不同时属于A和B的元素
const symmetricDifference = new Set([
  ...[...setA].filter(x => !setB.has(x)),
  ...[...setB].filter(x => !setA.has(x))
]);
console.log([...symmetricDifference]); // [1, 2, 5, 6]
```

## 3. WeakMap 与 WeakSet

### 3.1 WeakMap 详解

WeakMap 与 Map 类似，但有重要区别：

```javascript
const weakMap = new WeakMap();

let obj1 = { id: 1 };
let obj2 = { id: 2 };

// 添加键值对（键必须是对象）
weakMap.set(obj1, 'private data');
weakMap.set(obj2, 'sensitive info');

// 获取值
console.log(weakMap.get(obj1)); // "private data"

// 检查键是否存在
console.log(weakMap.has(obj1)); // true

// 删除键值对
weakMap.delete(obj1);

// 注意：WeakMap 没有 size 属性，也不能遍历
// console.log(weakMap.size); // undefined
// weakMap.forEach(...) // 错误
```

**WeakMap 特点：**
- 键必须是对象类型
- 键是弱引用，不会阻止垃圾回收
- 不可枚举，没有 size 属性
- 不支持遍历操作

### 3.2 WeakSet 详解

WeakSet 与 Set 类似，但只能存储对象引用：

```javascript
const weakSet = new WeakSet();

let user1 = { name: 'Alice' };
let user2 = { name: 'Bob' };

weakSet.add(user1);
weakSet.add(user2);

console.log(weakSet.has(user1)); // true

weakSet.delete(user1);

// 同样没有 size 属性和遍历方法
```

**WeakSet 特点：**
- 值必须是对象类型
- 值是弱引用，不会阻止垃圾回收
- 不可枚举，没有 size 属性
- 不支持遍历操作

### 3.3 WeakMap/WeakSet 与 Map/Set 的区别

| 特性 | WeakMap/WeakSet | Map/Set |
|------|-----------------|---------|
| **键/值的类型** | 只能是对象 | 任意类型 |
| **可迭代性** | 不可迭代 | 可迭代 |
| **大小获取** | 无 size 属性 | 有 size 属性 |
| **垃圾回收** | 弱引用，不阻止垃圾回收 | 强引用，阻止垃圾回收 |
| **方法** | 只有 set/get/has/delete | 完整的方法集 |
| **内存管理** | 自动清理不再使用的键/值 | 需要手动删除不再使用的键/值 |
| **使用场景** | 存储私有数据、缓存等 | 通用数据存储 |

## 4. 实际应用场景

### 4.1 Map 的应用场景

```javascript
// 1. 需要非字符串键的情况
const domElement = document.getElementById('myElement');
const metadata = new Map();
metadata.set(domElement, { clicked: 0, lastClick: null });

// 2. 需要保持插入顺序的键值对
const orderedMap = new Map();
orderedMap.set('z', 1);
orderedMap.set('a', 2);
orderedMap.set('m', 3);
console.log([...orderedMap.keys()]); // ['z', 'a', 'm'] 保持插入顺序

// 3. 存储元数据而不修改原始对象
const user = { name: 'John' };
const userMetadata = new Map();
userMetadata.set(user, { permissions: ['read', 'write'], loginCount: 5 });

// 4. 频率计数器
function countFrequency(arr) {
  const freqMap = new Map();
  for (const item of arr) {
    freqMap.set(item, (freqMap.get(item) || 0) + 1);
  }
  return freqMap;
}
```

### 4.2 Set 的应用场景

```javascript
// 1. 数组去重
const numbers = [1, 2, 2, 3, 4, 4, 5];
const uniqueNumbers = [...new Set(numbers)];
console.log(uniqueNumbers); // [1, 2, 3, 4, 5]

// 2. 成员资格检查（比数组高效）
const validUsers = new Set(['user1', 'user2', 'user3']);
function isValidUser(username) {
  return validUsers.has(username); // O(1) 时间复杂度
}

// 3. 跟踪已处理的项目
const processedItems = new Set();
function processItem(item) {
  if (processedItems.has(item)) return;
  // 处理项目...
  processedItems.add(item);
}

// 4. 获取两个数据集的差异
const currentUsers = new Set(['user1', 'user2', 'user4']);
const newUsers = new Set(['user2', 'user3', 'user5']);

// 新增的用户
const addedUsers = new Set([...newUsers].filter(user => !currentUsers.has(user)));
// 删除的用户
const removedUsers = new Set([...currentUsers].filter(user => !newUsers.has(user)));
```

### 4.3 WeakMap/WeakSet 的应用场景

```javascript
// 1. 存储对象的私有数据（避免内存泄漏）
const privateData = new WeakMap();

class User {
  constructor(name) {
    privateData.set(this, { name, secret: Math.random() });
  }
  
  getName() {
    return privateData.get(this).name;
  }
}

// 2. DOM元素与数据的关联（元素删除时自动清理）
const domData = new WeakMap();
const button = document.createElement('button');
domData.set(button, { clickCount: 0, lastClicked: null });

// 3. 缓存计算结果（当对象不再需要时自动清除）
const cache = new WeakMap();
function computeExpensiveValue(obj) {
  if (cache.has(obj)) {
    return cache.get(obj);
  }
  const result = /* 昂贵计算 */;
  cache.set(obj, result);
  return result;
}

// 4. 监听器管理（当DOM元素被移除时自动清理）
const listenerRegistry = new WeakMap();
function addListener(element, event, handler) {
  const wrappedHandler = e => {
    // 处理事件...
    handler(e);
  };
  element.addEventListener(event, wrappedHandler);
  listenerRegistry.set(element, { event, handler: wrappedHandler });
}

// 当元素被移除时，相关的监听器会自动被垃圾回收
```

## 5. 性能考虑

### 5.1 Map vs 对象

- **查找性能**：Map 和对象都有 O(1) 的时间复杂度
- **插入性能**：Map 通常比对象稍快，尤其是在频繁添加删除键值对时
- **内存使用**：Map 通常比存储相同数据的对象使用更多内存
- **迭代性能**：Map 的迭代性能优于对象

### 5.2 Set vs 数组

- **查找性能**：Set 的 has() 方法是 O(1)，数组的 includes() 是 O(n)
- **插入性能**：Set 的 add() 方法是 O(1)，数组的 push() 是 O(1) 但需要去重时更复杂
- **去重性能**：Set 去重比数组方法更高效

### 5.3 何时选择哪种数据结构

- 使用 **Map** 当：
  - 需要任意类型的键
  - 需要保持键的插入顺序
  - 频繁添加和删除键值对

- 使用 **Set** 当：
  - 需要存储唯一值
  - 需要快速检查值是否存在
  - 需要执行集合操作（并集、交集等）

- 使用 **WeakMap/WeakSet** 当：
  - 需要将数据与对象关联而不影响垃圾回收
  - 处理DOM元素和其他可能被销毁的对象
  - 需要临时缓存与对象相关的数据

## 6. 总结

- **Map**：灵活的键值对集合，支持任意类型键，保持插入顺序，适合键值对存储和频繁更新场景
- **Set**：唯一值集合，自动去重，支持高效成员检查，适合去重和集合运算场景
- **WeakMap**：键为弱引用的Map，适合存储对象关联数据而不阻止垃圾回收
- **WeakSet**：值为弱引用的Set，适合存储对象集合而不阻止垃圾回收

在现代JavaScript开发中，合理使用这些数据结构可以显著提高代码的可读性、性能和内存管理效率。根据具体需求选择合适的数据结构是编写高效JavaScript代码的关键技能之一。