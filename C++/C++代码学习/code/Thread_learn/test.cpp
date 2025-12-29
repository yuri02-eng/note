#include <iostream>
#include <mutex>
#include <thread>

std::mutex mtx;

void foo(int n) {
    for (int i = 0; i < n; ++i) {
        std::lock_guard<std::mutex> lk(mtx);
        std::cout << "函数指针线程\n";
    }
}

class ThreadObj {
   public:
    void operator()(int n) const {
        for (int i = 0; i < n; ++i) {
            std::lock_guard<std::mutex> lk(mtx);
            std::cout << "函数对象线程\n";
        }
    }
};

int main() {
    std::thread t1(foo, 3);
    std::thread t2(ThreadObj(), 3);
    std::thread t3([](int n) {
        for (int i = 0; i < n; ++i) {
            std::lock_guard<std::mutex> lk(mtx);
            std::cout << "lambda 线程\n";
        }
    },
                   3);

    t1.join();
    t2.join();
    t3.join();
}
