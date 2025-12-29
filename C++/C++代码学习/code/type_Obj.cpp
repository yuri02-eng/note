#include <iostream>
using namespace std;

enum Number { ZERO,
              ONE,
              TWO,
              THREE };

int main() {
    Number num = TWO;

    // 枚举转整数（隐式）
    int intValue = num;
    cout << "枚举值: " << intValue << endl;  // 输出2

    // 整数转枚举（需要显式转换）
    Number newNum = static_cast<Number>(3);
    cout << "新枚举值: " << newNum << endl;  // 输出3

    // 遍历枚举值
    for (int i = ZERO; i <= THREE; i++) {
        Number current = static_cast<Number>(i);
        cout << "当前值: " << current << endl;
    }

    return 0;
}