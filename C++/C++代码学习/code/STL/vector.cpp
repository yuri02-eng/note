#include <iostream>
#include <vector>
using namespace std;
int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    vec.push_back(6);
    for (int i : vec) {
        std::cout << i << " ";
    }
    return 0;
}