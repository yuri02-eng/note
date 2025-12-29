#include <iostream>
#include <string>
using namespace std;

class Example {
   private:
    int normal_member;
    mutable int mutable_member;  // 可变的
    string text;

   public:
    Example() : normal_member(0), mutable_member(0), text("Hello") {}

    // const成员函数 - 编译器会严格检查！
    const string& getData() const {
        // normal_member = 100;  // ❌ 编译错误：不能修改普通成员
        // text = "Changed";     // ❌ 编译错误：不能修改普通成员

        mutable_member++;  // ✅ 正确：可以修改mutable成员

        return text;  // ✅ 正确：可以返回const引用
    }

    // 非const成员函数 - 没有限制
    void setData(const string& new_text) {
        normal_member = 100;  // ✅ 正确：非const函数可以修改
        text = new_text;      // ✅ 正确
    }

    void demonstrate() const {
        cout << "Const函数演示:" << endl;
        cout << "普通成员: " << normal_member << endl;   // ✅ 读取OK
        cout << "可变成员: " << mutable_member << endl;  // ✅ 读取OK

        // 调用其他函数也有限制
        anotherConstFunction();  // ✅ 正确：可以调用其他const函数
        // nonConstFunction();      // ❌ 错误：不能调用非const函数
    }

    void anotherConstFunction() const {  // 另一个const函数
        cout << "另一个const函数" << endl;
    }

    void nonConstFunction() {  // 非const函数
        cout << "非const函数" << endl;
    }
};
int main() {
    Example example1 = Example();
}
