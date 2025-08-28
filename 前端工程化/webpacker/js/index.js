//once
function once(fn) {
    let flag = false
    return function (...args) {
        if (!flag) {
            flag = true
            fn.apply(this, args)
            console.log("函数执行一次")
        }
    }
}

const init = once(function () {
    console.log("Hello worl!")
})
init()
init()
init()

//debounce 防抖，输入框停止输入固定时间后，执行对应函数
function debounce(fn, delay) {
    let timer = null
    return function (...args) {
        if (timer) {
            clearTimeout(timer)
        }
        timer = setTimeout(() => {
            fn.apply(this, args)//this指向调用者,箭头函数this与外部一致
        }, delay)
    }
}

function search() {
    console.log("搜索")
}

const searchDebounce = debounce(search, 1000)
searchDebounce()
searchDebounce()
searchDebounce()

//throttle 节流，固定时间执行一次
function throttle(fn, delay) {
    let flag = true
    return function (...args) {
        if (!flag) {
            return
        }
        flag = false
        setTimeout(() => {
            fn.apply(this, args)
            flag = true
        }, delay)
    }
}

//curry
/**
 * @description: 将函数柯里化的工具函数
 * @param {Function} fn 待柯里化的函数
 * @param {array} args 已经接收的参数列表
 * @return {Function}
 */
const currying = function(fn, ...args) {
    // fn需要的参数个数
    const len = fn.length
    // 返回一个函数接收剩余参数
    return function (...params) {
        // 拼接已经接收和新接收的参数列表
        let _args = [...args, ...params]
        // 如果已经接收的参数个数还不够，继续返回一个新函数接收剩余参数
        if (_args.length < len) {
            return currying.call(this, fn, ..._args)
        }
        // 参数全部接收完调用原函数
        return fn.apply(this, _args)
    }
}
/**
 * 柯里化函数实现
 * @param {Function} fn - 需要被柯里化的函数
 * @param {...any} args - 初始参数
 * @returns {Function} 返回一个新函数，可以继续接收参数
 */
function curry(fn, ...args) {
    return function () {
        // 合并已传入的参数和新参数
        let _args=[...args,...arguments]
        // 获取目标函数需要的参数个数
        let len= fn.length
        // 如果参数个数不够，继续柯里化
        if(_args.length<len){
            return curry(fn,..._args)
        }else{
            // 参数足够时，执行原函数
            return fn.apply(this,_args)
        }
    }
}
function add(a, b, c) {
    return a + b + c
}
let addCurry = curry(add)
console.log(addCurry(1)(2)(3))
console.log(addCurry(1, 2)(3))
console.log(addCurry(1, 2, 3))