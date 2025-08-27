const name={name:"张三"}
console.log(name)
if(true){
    const name={name:"李四"}
    console.log(name)
}
console.log(name)
function Hello(){
    console.log("Hello",this.name)
}
Hello()
const person={name:"张三"}
const Hello2=Hello.bind(person)
Hello2()
person.name="李四"
Hello2()
person.name="王五"
Hello2()