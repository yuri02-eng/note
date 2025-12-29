#include <algorithm>
#include <iostream>
#include <vector>
using namespace std;
int main() {
    vector<int> vec = {5, 3, 1, 4, 2};
    sort(vec.begin(), vec.end());
    for (int i : vec) {
        cout << i << " ";
    }
    return 0;
}