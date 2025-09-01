## 要手写一个 `Promise`，我们需要模拟它的基本行为，确保它具备以下功能：

* 支持 `pending`、`fulfilled` 和 `rejected` 状态。
* 支持 `resolve` 和 `reject` 方法来更新状态。
* 支持 `.then()` 方法来处理成功和失败的回调。
* 支持 `.catch()` 方法来处理失败的回调。
* 支持链式调用。

以下是一个简单的手写 `Promise` 实现：

```javascript
class MyPromise {
    // 构造函数，接受一个执行器函数
    constructor(executor) {
        this.state = 'pending';  // Promise的初始状态
        this.value = undefined;  // 成功时的结果
        this.reason = undefined;  // 失败时的原因
        this.onFulfilledCallbacks = [];  // 存放成功时的回调函数
        this.onRejectedCallbacks = [];  // 存放失败时的回调函数

        // 执行器函数，传入 resolve 和 reject
        const resolve = (value) => {
            if (this.state === 'pending') {
                this.state = 'fulfilled';
                this.value = value;
                // 执行所有成功的回调
                this.onFulfilledCallbacks.forEach(callback => callback(value));
            }
        };

        const reject = (reason) => {
            if (this.state === 'pending') {
                this.state = 'rejected';
                this.reason = reason;
                // 执行所有失败的回调
                this.onRejectedCallbacks.forEach(callback => callback(reason));
            }
        };

        // 执行 executor 函数
        try {
            executor(resolve, reject);
        } catch (error) {
            reject(error);
        }
    }

    // then 方法，处理成功和失败的回调
    then(onFulfilled, onRejected) {
        // 如果 onFulfilled 不是函数，则使用默认的回调
        onFulfilled = typeof onFulfilled === 'function' ? onFulfilled : value => value;
        onRejected = typeof onRejected === 'function' ? onRejected : reason => { throw reason };

        // 返回一个新的 Promise，支持链式调用
        return new MyPromise((resolve, reject) => {
            // 如果 Promise 已经是 fulfilled 状态
            if (this.state === 'fulfilled') {
                setTimeout(() => {
                    try {
                        const x = onFulfilled(this.value);
                        resolvePromise(x, resolve, reject);
                    } catch (error) {
                        reject(error);
                    }
                });
            }

            // 如果 Promise 已经是 rejected 状态
            if (this.state === 'rejected') {
                setTimeout(() => {
                    try {
                        const x = onRejected(this.reason);
                        resolvePromise(x, resolve, reject);
                    } catch (error) {
                        reject(error);
                    }
                });
            }

            // 如果 Promise 仍然是 pending 状态
            if (this.state === 'pending') {
                this.onFulfilledCallbacks.push(() => {
                    setTimeout(() => {
                        try {
                            const x = onFulfilled(this.value);
                            resolvePromise(x, resolve, reject);
                        } catch (error) {
                            reject(error);
                        }
                    });
                });

                this.onRejectedCallbacks.push(() => {
                    setTimeout(() => {
                        try {
                            const x = onRejected(this.reason);
                            resolvePromise(x, resolve, reject);
                        } catch (error) {
                            reject(error);
                        }
                    });
                });
            }
        });
    }

    // catch 方法，简化错误处理
    catch(onRejected) {
        return this.then(null, onRejected);
    }
}

// 用于处理 Promise 链式调用中的返回值，保证返回值为 Promise
function resolvePromise(x, resolve, reject) {
    if (x === resolve) {
        reject(new TypeError('Chaining cycle detected.'));
    } else if (x instanceof MyPromise) {
        x.then(resolve, reject);
    } else {
        resolve(x);
    }
}
```

解释：

1. **状态管理**：

   * `state` 属性表示 Promise 当前的状态，初始为 `pending`，它可以是 `fulfilled`（成功）或者 `rejected`（失败）。
   * `value` 属性保存成功时的结果，`reason` 属性保存失败时的原因。

2. **构造函数**：

   * 接受一个执行器函数 `executor`，它在构造 `Promise` 时立即执行，传入两个参数：`resolve` 和 `reject`。
   * `resolve` 用来改变状态为 `fulfilled`，并存储成功结果。
   * `reject` 用来改变状态为 `rejected`，并存储失败原因。

3. **`then()` 方法**：

   * `.then()` 方法返回一个新的 Promise，支持链式调用。
   * 如果当前 Promise 的状态是 `fulfilled`，则立即执行成功的回调。
   * 如果当前 Promise 的状态是 `rejected`，则立即执行失败的回调。
   * 如果当前 Promise 仍然是 `pending`，则将回调函数推入对应的队列，待状态变更后执行。

4. **`catch()` 方法**：

   * `.catch()` 是 `.then(null, onRejected)` 的语法糖，专门用于处理失败的回调。

5. **`resolvePromise()`**：

   * 处理 Promise 链式调用中的返回值，确保返回值是一个 Promise 并且正确地处理它的状态，避免 Promise 之间产生环形引用。

示例：

```javascript
let promise = new MyPromise((resolve, reject) => {
    setTimeout(() => {
        resolve("成功的结果");
    }, 1000);
});

promise.then(result => {
    console.log(result);  // 输出: "成功的结果"
    return "继续处理";
}).then(result => {
    console.log(result);  // 输出: "继续处理"
}).catch(error => {
    console.log("错误:", error);
});
```

在这个手写的 Promise 实现中，我们模拟了 Promise 的核心功能，并支持链式调用、错误处理以及 `catch()` 方法。这只是一个简单的实现，实际中的 Promise 还会处理更多的边界情况，但这已经是一个基础且有效的版本。

## **要深入理解为什么 `.then()` 方法需要返回一个新的 Promise，且如何在新的 Promise 内部执行代码，我们得从 Promise 的设计理念、异步回调以及链式调用的特性来理解。**

### **Promise 链式调用和返回新的 Promise**

Promise 的设计初衷之一是允许链式调用。每个 `.then()` 都应该返回一个新的 Promise，以便下一个 `.then()` 可以依赖这个新的 Promise 结果继续执行。换句话说，**Promise 的每个 `.then()` 都应当能够“延续”异步操作的处理**。

所以，**每个 `.then()` 需要返回一个新的 Promise，以支持链式调用**，而这个新的 Promise 必须是从 `.then()` 处理的异步结果中创建的。让我们逐步分析它的原因。

### **1. 链式调用的设计需求**

Promise 允许我们通过 `.then()` 来进行链式调用，链式调用的核心是：

- 每一个 `.then()` 调用都能接收到前一个 Promise 的结果或错误。
- 每一个 `.then()` 都有机会返回一个新的结果，这个结果会作为下一个 `.then()` 的输入。

为了让 `.then()` 支持链式调用，每次调用 `.then()` 都必须返回一个新的 Promise。否则，你将无法将多个 `.then()` 链接在一起，导致异步操作的结果无法正确传递。

### **2. 新的 Promise 必须是根据前一个结果创建的**

你提到的代码在 `.then()` 内部 `new MyPromise()` 的部分，实际上是为了确保每次 `.then()` 调用都能根据前一个 Promise 的状态（`fulfilled`、`rejected` 或 `pending`）决定如何处理结果。

假设我们已经有一个已经完成的 Promise，它的状态是 `fulfilled` 或 `rejected`，那么我们就可以立即执行回调并返回一个新的 Promise。如果当前 Promise 的状态是 `pending`（还没有完成），我们就推迟回调的执行，直到前一个 Promise 的状态被确定。

### **3. 回调（onFulfilled 和 onRejected）的处理逻辑**

在 `.then()` 方法中，我们还做了如下几个处理：

1. **回调默认值**：如果没有传入 `onFulfilled` 或 `onRejected`，我们给它们设置默认值，以保证即使用户没有传递回调，也能继续执行链式调用。
   - `onFulfilled = value => value`：如果用户没有提供成功的回调，那么默认的回调就是返回传入的 `value`，表示没有对成功结果进行处理，直接将结果传递下去。
   - `onRejected = reason => { throw reason }`：如果用户没有提供失败的回调，那么默认的回调就是抛出错误，将错误传递下去。
2. **setTimeout**：我们在回调函数中使用 `setTimeout()`，是为了确保回调函数在当前执行栈中的任务完成之后再执行。这是为了避免在当前调用栈中同步执行 `.then()` 时，可能导致栈溢出等问题，同时模拟异步行为，符合 Promise 的异步执行规范。

### **4. 返回新 Promise 的关键点**

在返回一个新的 Promise 时，我们需要确保这个新的 Promise 根据前一个操作的结果进行处理。这就是为什么每次执行 `onFulfilled` 或 `onRejected` 时都需要用 `resolvePromise()` 来确保：

- 如果回调函数返回的是一个值，那么我们将其传递到下一个 Promise。
- 如果回调函数返回的是一个 Promise，那么我们需要“等待”这个 Promise 被解决（通过递归调用 `.then()` 或 `.catch()`）并确保最终的结果传递下去。

`resolvePromise` 的目的是处理 Promise 链中的嵌套情况。如果 `onFulfilled` 或 `onRejected` 返回一个 Promise，新的 Promise 就会“等待”它完成，这就是 Promise 解决异步操作顺序的方式。

### **为什么必须在新 Promise 内部执行回调代码？**

1. **防止同步代码的执行顺序问题**：
   - 如果我们不在一个新的 Promise 内部执行回调，所有的 `.then()` 回调就会在当前执行栈中同步执行。这将导致链式调用的行为被破坏，异步执行的效果也会丧失。
   - 使用新的 Promise 并在其中执行回调代码，确保每个 `.then()` 的回调在事件循环的下一个“tick”中异步执行，从而实现 Promise 的异步效果。
2. **保证链式调用能够正确返回 Promise**：
   - 如果每个 `.then()` 返回的不是一个新的 Promise，或者没有对回调结果进行处理，那么就无法支持链式调用。
   - 每个 `.then()` 返回的新的 Promise 必须能够将上一个 `.then()` 的结果（无论是值还是错误）传递下去，以便下一个 `.then()` 可以正确地获取到前一个 `.then()` 的结果。

### 总结：

- **返回一个新的 Promise**：为了支持链式调用，每个 `.then()` 必须返回一个新的 Promise。否则，后续的 `.then()` 无法获取到前一个操作的结果。
- **异步执行**：通过 `setTimeout()` 和 `new MyPromise()` 的包装，确保回调在下一个事件循环中异步执行，符合 Promise 的异步特性。
- **回调的默认值和错误处理**：如果用户没有传入回调函数，我们提供默认行为（例如传递值或抛出错误），以保证 Promise 的稳定性和一致性。

这种设计保证了 Promise 的链式调用功能，同时解决了异步编程中的一些常见问题，如回调地狱和错误处理不当等。