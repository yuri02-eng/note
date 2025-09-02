function myNew(Constructor, ...args) {
    let obj = {}
    obj.__proto__ = Constructor.prototype
    const result = Constructor.apply(obj, args);
    const isObject = typeof result === 'object' && result !== null
    const isFunction = typeof result === 'function'
    if (isObject || isFunction) {
        return result;
    }
    return obj;
}