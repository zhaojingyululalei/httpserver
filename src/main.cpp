#include "log.h"
#include <thread>
#include <chrono>

extern void log_test(void);
extern void config_test(void);
int main() {
    //log_test();
    config_test();
    return 0;
}