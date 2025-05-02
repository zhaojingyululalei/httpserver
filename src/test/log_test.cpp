
#include "log.h"
#include <iostream>
#include <ctime>
#include "utils.h"
static void test1(){
    //using namespace zhao;
    // Logger::Ptr logger = std::make_shared<Logger>("test");
    // //获取当前时间戳
    // //"%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
    // uint64_t timestamp = getTimeStamp();
    // LogEvent::Ptr event = std::make_shared<LogEvent>(logger,LogLevel::DEBUG,"file",1,0,0,0,timestamp,"thread_name","log_content");
    // logger->log(LogLevel::DEBUG,event);
}
static void test2()
{
     // 创建一个日志器
     zhao::Logger::Ptr logger = std::make_shared<zhao::Logger>("test_logger");

     // 使用 LogEventWrap 输出日志
     ZHAO_LOG(logger, zhao::LogLevel::INFO) << "This is an info message."; // T 推导为 const char*
     ZHAO_LOG(logger, zhao::LogLevel::DEBUG) << 42; // T 推导为 int
     ZHAO_LOG(logger, zhao::LogLevel::ERROR) <<  3.14; // T 推导为 double
     ZHAO_LOG(logger, zhao::LogLevel::WARN) << std::string("This is a string object."); // T 推导为 std::string
 
     ZHAO_LOG_FMT_DEBUG(logger, "This is a formatted debug message with %d arguments.", 1);
     ZHAO_LOG_FMT_INFO(logger, "This is a formatted info message with %s arguments.", "one");
}
void log_test(void){
    
    auto logger = zhao::LoggerMgr::getInstance()->getLogger("test");

    // 使用 LogEventWrap 输出日志
    ZHAO_LOG(logger, zhao::LogLevel::INFO) << "This is an info message."; // T 推导为 const char*
    ZHAO_LOG(logger, zhao::LogLevel::DEBUG) << 42; // T 推导为 int
    ZHAO_LOG(logger, zhao::LogLevel::ERROR) <<  3.14; // T 推导为 double
    ZHAO_LOG(logger, zhao::LogLevel::WARN) << std::string("This is a string object."); // T 推导为 std::string

    ZHAO_LOG_FMT_DEBUG(logger, "This is a formatted debug message with %d arguments.", 1);
    ZHAO_LOG_FMT_INFO(logger, "This is a formatted info message with %s arguments.", "one");
   
}