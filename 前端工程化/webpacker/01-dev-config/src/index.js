class A {
    constructor() {
        this.str = "hello webpack"
    }

    say() {
        console.log(this.str)
    }
}

const a = new A()
a.say()