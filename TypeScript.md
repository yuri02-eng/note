以下是一份**系统化、进阶向**的 TypeScript 深度笔记，覆盖语言的全部核心概念、常见陷阱与工程实践。你可以按需跳读，也可作为“从入门到源码级”的常备手册。

---

## 目录
1. 语言定位与编译流程  
2. 类型系统全景  
3. 函数深入  
4. 接口 vs 类型别名  
5. 类与继承全解  
6. 泛型：从入门到“类型体操”  
7. 高级类型（条件、映射、模板字面量）  
8. 模块解析策略  
9. 命名空间与声明合并  
10. 装饰器元编程  
11. 工程配置（tsconfig 全字段）  
12. 类型声明文件（*.d.ts）实战  
13. 常见错误与调试技巧  
14. 性能与构建优化  
15. 附录：术语表 & 速查表

---

## 1. 语言定位与编译流程
- **设计目标**  
  静态类型检查 + 保留 JS 运行时行为；类型信息**擦除**（type erasure）。  
- **三阶段编译**  
  1. Scanner（词法分析） → 2. Parser（AST） → 3. Checker（类型检查） → 4. Emitter（JS 代码生成 + sourcemap）。  
- **tsc vs babel vs swc/esbuild**  
  - tsc：官方，类型检查最完整，速度中等。  
  - babel/swc：只做**语法降级**，不检查类型；适合大型项目并行编译。  
  - 组合：tsc --noEmit 仅检查类型，swc/esbuild 负责转译。

---

## 2. 类型系统全景
| 类别                  | 示例                                                         | 备注                                                         |
| --------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 原始类型              | `number` `string` `boolean` `bigint` `symbol` `undefined` `null` | `undefined` 和 `null` 是所有类型的子类型（--strictNullChecks 关闭时）。 |
| 字面量类型            | `type Dir = 'north' \| 'south'`                              | 与联合类型结合可模拟枚举。                                   |
| 数组 & 元组           | `Array<T>` `[T, U]`                                          | 可变长元组 `[...T[], U]`（TS 4.0+）。                        |
| 对象类型              | `{ x: number; readonly y?: string }`                         | `readonly` 只作用于属性本身，不深度冻结。                    |
| 函数类型              | `(a: number) => string`                                      | 支持重载列表 + 实现签名。                                    |
| any / unknown / never |                                                              | `never` 是底层类型，可用于穷尽检查。                         |
| unique symbol         | `const sym: unique symbol = Symbol('tag')`                   | 用于品牌类型（brand）。                                      |

---

## 3. 函数深入
### 3.1 参数特性
```ts
// 可选与默认值
function f(a: number, b?: number, c = 0) { }

// 剩余参数（元组类型）
function call<T extends unknown[]>(
  fn: (...args: T) => void,
  ...args: T
) { }

// 解构 + 类型
function g({ a, b }: { a: number; b: string }) { }
```

### 3.2 函数重载
```ts
// 重载签名（仅静态检查）
function create(x: string): string;
function create(x: number): number;
// 实现签名（不对外可见）
function create(x: string | number) { return x; }
```

### 3.3 this 参数
```ts
interface DB {
  filterUsers(this: DB, predicate: (u: User) => boolean): User[];
}
```
在回调中显式约束 `this` 类型，防止运行时错误。

---

## 4. 接口 vs 类型别名
| 对比维度 | interface                | type alias     |
| -------- | ------------------------ | -------------- |
| 同名合并 | ✅（声明合并）            | ❌              |
| 映射类型 | 不支持                   | ✅              |
| 递归     | 需借助 `interface` 自身  | ✅（直接）      |
| 性能     | 类型检查更快（缓存友好） | 复杂场景可能慢 |

### 4.1 声明合并示例
```ts
interface Box { width: number; }
interface Box { height: number; } // 合并为 { width, height }
```

---

## 5. 类与继承全解
### 5.1 成员修饰符
- `public`（默认）  
- `private`（仅在同类可见，编译时检查，运行时被擦除）  
- `protected`（同类 + 派生类）  
- `#field`（原生私有字段，运行时也私有）  

### 5.2 抽象类 vs 接口
- 抽象类可有实现代码，接口不可。  
- 单继承多实现：`class A extends B implements C, D {}`

### 5.3 参数属性
```ts
class Point {
  constructor(public x: number, public y: number) {}
}
```
编译后等价于先声明再赋值。

### 5.4 访问器 & 静态块
```ts
class C {
  static #count = 0;
  static get count() { return this.#count; }
  static { console.log('static block'); } // TS 4.4+
}
```

---

## 6. 泛型：从入门到“类型体操”
### 6.1 泛型约束
```ts
function longest<T extends { length: number }>(a: T, b: T) {
  return a.length > b.length ? a : b;
}
```

### 6.2 默认类型
```ts
interface Container<T = string> { value: T; }
```

### 6.3 条件类型
```ts
type IsString<T> = T extends string ? true : false;
type A = IsString<"hi">; // true
```

### 6.4 映射类型
```ts
type Mutable<T> = { -readonly [P in keyof T]: T[P] };
type MyPartial<T> = { [P in keyof T]?: T[P] };
```

### 6.5 模板字面量类型
```ts
type EventName<T extends string> = `${T}Changed`;
type Foo = EventName<'foo'>; // "fooChanged"
```

---

## 7. 高级类型
- **infer 关键字**（提取类型）
  ```ts
  type ReturnType<T> = T extends (...args: any[]) => infer R ? R : never;
  ```
- **分布式条件类型**
  ```ts
  type ToArray<T> = T extends any ? T[] : never;
  type R = ToArray<string | number>; // string[] | number[]
  ```
- **递归类型**
  ```ts
  type Json = string | number | boolean | null | Json[] | { [k: string]: Json };
  ```

---

## 8. 模块解析策略
| 模块系统          | 文件扩展名   | 路径映射              |
| ----------------- | ------------ | --------------------- |
| CommonJS          | .js / .ts    | baseUrl + paths       |
| ESModule          | .mjs / .ts   | 支持 import maps      |
| Node16 / NodeNext | 区分 ESM/CJS | package.json `"type"` |

### 8.1 路径映射（paths）
```json
{
  "compilerOptions": {
    "baseUrl": ".",
    "paths": {
      "@/*": ["src/*"]
    }
  }
}
```
需配合构建工具（webpack/vite）的 alias。

---

## 9. 命名空间与声明合并
- **namespace** 用于全局库（如 `React`）。  
- **模块** 优于 `namespace`（ESM 时代）。  
- **声明合并** 还包括：枚举 + 命名空间、类 + 命名空间。

---

## 10. 装饰器元编程
- **Stage 3 装饰器提案**（TS 5.0+ `experimentalDecorators: false`）
  ```ts
  type Context = { kind: 'class'; name: string };
  function logged(value: any, context: Context) {
    console.log(`${context.name} defined`);
  }
  @logged
  class C {}
  ```
- **旧版装饰器**（`experimentalDecorators: true`）  
  参数：target / propertyKey / descriptor。

---

## 11. 工程配置（tsconfig 全字段）
| 关键字段                       | 说明                                 |
| ------------------------------ | ------------------------------------ |
| `strict`                       | 打开所有严格检查（推荐始终开启）。   |
| `exactOptionalPropertyTypes`   | 区分 `undefined` 与 属性缺失。       |
| `noUncheckedIndexedAccess`     | 索引签名结果包含 `undefined`。       |
| `moduleResolution`             | `node16` 支持 package.json exports。 |
| `allowSyntheticDefaultImports` | 允许 `import React from 'react'`。   |
| `declarationMap` + `sourceMap` | 调试 .d.ts 跳转源码。                |

---

## 12. 类型声明文件（*.d.ts）
### 12.1 三斜线指令
```ts
/// <reference types="node" />
```

### 12.2 全局声明
```ts
declare global {
  interface Window {
    __DEV__: boolean;
  }
}
```

### 12.3 模块扩充
```ts
declare module "*.svg" {
  const content: React.FC<React.SVGProps<SVGSVGElement>>;
  export default content;
}
```

---

## 13. 常见错误与调试技巧
- **“类型 X 上不存在属性 Y”**  
  1. 检查拼写；2. 使用 `as` 断言或类型守卫。  
- **“无法分配到 never”**  
  通常是穷尽检查失败，补充剩余分支。  
- **开启 `noImplicitAny`** 可捕获大量潜在错误。  
- **VS Code 快速修复**  
  `Ctrl+.` 自动生成类型、导入、缺失字段。

---

## 14. 性能与构建优化
| 手段                           | 说明                                    |
| ------------------------------ | --------------------------------------- |
| `tsc --incremental`            | 增量编译，生成 `.tsbuildinfo`。         |
| `project references`           | 多项目共享编译缓存。                    |
| `transpileOnly` + `swc-loader` | webpack 中仅转译，tsc 做类型检查。      |
| `skipLibCheck`                 | 跳过 node_modules 类型检查，提速 30%+。 |

---

## 15. 附录
### 15.1 术语表
- Discriminated Union（可辨识联合）  
- Exhaustiveness Check（穷尽检查）  
- Branded Type（品牌类型）  
- Higher-Kinded Type（高阶类型，TS 本身不支持，需模拟）

### 15.2 速查表（打印版）
```
# 类型断言
value as T
# 非空断言
element!
# 可选链
obj?.prop
# 空值合并
x ?? defaultVal
# 模板字面量
`Hello ${name}`
```
