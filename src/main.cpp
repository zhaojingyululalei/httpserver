#include <iostream>
#include <yaml-cpp/yaml.h>
#include "log/logger.hpp"
#include "config/configvar.hpp"
#include "scheduler/scheduler.hpp"
#include <signal.h>
#include <atomic>
extern void thread_test();
extern void mem_test();
extern void algorithm_test();
extern void spanlist_test();
extern void tcmalloc_test();
extern void hook_test();
extern void log_test();
extern void config_test();
extern void yaml_test();
extern void fiber_test();
extern void scheduler_test();
extern void msic_test();
extern void address_test();
extern void socket_test();
extern void stream_test();
extern void http_test();
extern void server_test();
static std::atomic<bool> g_running {true};
static zhao::Scheduler::ptr g_scheduler;
static void sigHandler(int sig) {
    std::cout << "\nReceived SIGINT, stopping scheduler..." << std::endl;
    g_running = false;
}
/**
 * @brief 设置各个模块的输出屏蔽等级，默认都是DEBUG
 * @details 各个模块调试通过后，都改为WARN;test测试项没必要改，就还是DEBUG
 */
static void log_level(void)
{
    MODULE_MODIFY_LEVEL("fiber", LOGGER_GET("system"), zhao::LogLevel::WARN);
    MODULE_MODIFY_LEVEL("scheduler", LOGGER_GET("system"), zhao::LogLevel::WARN);
    MODULE_MODIFY_LEVEL("http", LOGGER_GET("system"), zhao::LogLevel::WARN);
    MODULE_MODIFY_LEVEL("hook", LOGGER_GET("system"), zhao::LogLevel::WARN);
    MODULE_MODIFY_LEVEL("socket", LOGGER_GET("system"), zhao::LogLevel::WARN);
    MODULE_MODIFY_LEVEL("bytestream", LOGGER_GET("system"), zhao::LogLevel::WARN);
    //MODULE_MODIFY_LEVEL("tcpserver", LOGGER_GET("system"), zhao::LogLevel::WARN);
}
static void entry(void)
{
    //scheduler_test();
    //hook_test();
    //address_test();
    //socket_test();
    //stream_test();
    http_test();
    //server_test();
    while(g_running)
    {
        //std::cout <<"entry function"<<std::endl;
        sleep(1000);
    }
}
/**
 * @brief 主函数入口 
 * @details 注：全局只有一个调度器，entry已经是被调度的一个协程，后续测试在entry里面做
 * @details 后续不要再创建调度器了
 * @details 主线程和iomanager线程没有设置hook_enable，默认是false;调度器中的线程都设置了hook_enable，默认是true
 */
int main(int argc, char *argv[])
{
    YAML::Node root = YAML::LoadFile("config/log.yml");
    zhao::ConfigMgr::getInstance()->LoadFromYaml(root);
    log_level();
    //thread_test();
    //mem_test();
    //algorithm_test();
    //tcmalloc_test(); 
    //hook_test();
    //log_test();
    //yaml_test();
    //config_test();
    //fiber_test();
    //scheduler_test();
   // msic_test();
    // 注册信号处理函数
    signal(SIGINT, sigHandler);
    
    g_scheduler.reset(new zhao::Scheduler(2));
    g_scheduler->start();
    g_scheduler->schedule(entry);
    
    // 替换while(true)为条件循环
    while(g_running) {
        sleep(1);
    }
    g_scheduler->stop();
    std::cout << "\nReceived SIGINT, shutting down..." << std::endl;
    return 0;
}
