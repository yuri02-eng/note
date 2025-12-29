#include <iostream>
using namespace std;

int main() {
    int original = 100;
    int& ref = original;  // ref是original的引用（别名）

    cout << "原变量: " << original << endl;  // 输出: 100
    cout << "引用: " << ref << endl;         // 输出: 100

    // 通过引用修改变量值
    ref = 200;
    cout << "修改后原变量: " << original << endl;  // 输出: 200

    // 验证它们指向同一地址
    cout << "原变量地址: " << &original << endl;  // 相同地址
    cout << "引用地址: " << &ref << endl;         // 相同地址

    return 0;
}