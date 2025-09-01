const add1 = function (...args) {
    return args.reduce((a, b) => a + b, 0) + 1
}
const multiply2 = x => x * 2;
const multiply3 = x => x * 3;

function compose(...functions) {
    return functions.reduce((f1, f2) => (...args) => f1(f2(...args)))
}
console.log(add1(1,2,3))
const add1Multiply2 = compose(add1, multiply2, multiply3);
console.log(add1Multiply2(1,2,3));