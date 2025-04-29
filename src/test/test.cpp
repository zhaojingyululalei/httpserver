
#include "log.h"
#include <iostream>
#include <ctime>
using namespace zhao;
void log_test(void){
    Logger::Ptr logger = std::make_shared<Logger>("test");
    //获取当前时间戳
    //"%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
    uint64_t timestamp = time(0);
    LogEvent::Ptr event = std::make_shared<LogEvent>(logger,LogLevel::DEBUG,"file",1,timestamp,0,0,0,"thread_name","log_content");
    logger->log(LogLevel::DEBUG,event);
}