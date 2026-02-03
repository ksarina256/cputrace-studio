#include <iostream>
#include <vector>
#include <cmath>
#include <unistd.h>

void expensive_math_task() {
    double result = 0;
    for (int i = 0; i < 1000000; ++i) {
        result += std::sin(i) * std::cos(i);
    }
}

void memory_heavy_task() {
    std::vector<int> data(100000, 42);
    for (auto& x : data) x *= 2;
}

int main() {
    std::cout << "Test app running. PID: " << getpid() << std::endl;
    while (true) {
        expensive_math_task();
        memory_heavy_task();
    }
    return 0;
}