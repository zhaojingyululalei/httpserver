#include "log.h"
#include <thread>
#include <chrono>

extern void log_test(void);
int main() {
    log_test();
    // 获取当前时间点
    // auto now = std::chrono::system_clock::now();
    // // 转换为时间戳（自1970年1月1日以来的秒数）
    // auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    // std::cout << "Current timestamp: " << timestamp << std::endl;
    return 0;
}