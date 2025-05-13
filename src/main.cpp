#include "log.h"
#include <thread>
#include <chrono>

extern void log_test(void);
extern void config_test(void);
extern void thread_test(void);
int main()
{
    // log_test();
    //config_test();
    thread_test();
    return 0;
}