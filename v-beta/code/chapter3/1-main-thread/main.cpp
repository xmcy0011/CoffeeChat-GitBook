#include <iostream>
#include <thread>

int main() {
    std::thread::id tid = std::this_thread::get_id();
    std::cout << "main thread id=" << tid << std::endl;

    std::thread t([]() {
       
    });
    return 0;
}