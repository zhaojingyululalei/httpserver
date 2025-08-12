#include <iostream>
#include "log/logger.hpp"
static void test01(void)
{
    zhao::Logger::ptr logger(new zhao::Logger);
    logger->addConsole(zhao::StdoutConsole::ptr(new zhao::StdoutConsole));
    logger->addConsole(zhao::FileConsole::ptr(new zhao::FileConsole("log/log.txt")));
    zhao::LogEvent::ptr event(new zhao::LogEvent(__FILE__, __LINE__, 0, 1, 2, time(0), "testThread"));
    event->m_content << "hello world";
    logger->debug(event);
}

static void test02(void)
{
#define MODULE_NAME "test02"
    zhao::Logger::ptr logger(new zhao::Logger);
    logger->addConsole(zhao::StdoutConsole::ptr(new zhao::StdoutConsole));
    logger->addModule(MODULE_NAME);
    zhao::LogEvent::ptr event(new zhao::LogEvent(__FILE__, __LINE__, 0, 1, 2, time(0), "testThread"));
    event->m_content << "hello world";
    logger->debug(event,MODULE_NAME);
}
static void test03(void)
{
    zhao::Logger::ptr logger(new zhao::Logger);
    logger->addConsole(zhao::StdoutConsole::ptr(new zhao::StdoutConsole));
    logger->addModule(MODULE_NAME);
    zhao::LogEvent::ptr event(new zhao::LogEvent(__FILE__, __LINE__, 0, 1, 2, time(0), "testThread"));
    event->m_content << "hello world";
    logger->debug(event); //root模块
    logger->debug(event,MODULE_NAME);//test02模块
    logger->maskModule("root");
    std::cout << "after mask.........." << std::endl;
    logger->debug(event); //root模块
    logger->debug(event,MODULE_NAME);//test02模块
}
static void test04(void)
{
    zhao::Logger::ptr logger = zhao::LoggerMgr::getInstance()->getLogger();
    const char* module = "test04";
    #define MODULE_NAME_4 "test04"
    MODULE_DEBUG(MODULE_NAME_4,logger)<<"nihao"; 
    LOG_DEBUG(logger)<<"woaini";
}
static void test05(void)
{
    #define MODULE_NAME_5 "test05" 
    MODULE_DEBUG(MODULE_NAME_5,LOGGER_ROOT())<<"nihao";

    LOG_DEBUG(LOGGER_ROOT())<<"woaini";
}
#define MODULE_NAME "test06"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_ROOT())
#define warn MODULE_WARN(MODULE_NAME, LOGGER_ROOT())
#define error MODULE_ERROR(MODULE_NAME, LOGGER_ROOT())
static void test06(void)
{ 
    dbg<<"nihao";
    warn<<"woaini";
    error<<"hello";
}
#undef MODULE_NAME
#undef dbg
#undef warn
#undef error
#define MODULE_NAME "test07"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_ROOT())
#define warn MODULE_WARN(MODULE_NAME, LOGGER_ROOT())
#define error MODULE_ERROR(MODULE_NAME, LOGGER_ROOT())
static void test07()
{
    dbg<<"nihao";
    warn<<"woaini";
    error<<"hello";
}
#undef MODULE_NAME
#undef dbg
#undef warn
#undef error

static void test08()
{ 
    MODULE_DEBUG("A", LOGGER_GET("system"))<<"nihao";
    MODULE_DEBUG("B", LOGGER_GET("system"))<<"nihao";
    MODULE_DEBUG("C", LOGGER_GET("system"))<<"nihao";
    MODULE_DEBUG("zhao", LOGGER_GET("system"))<<"nihao";
    MODULE_DEBUG("world", LOGGER_GET("system"))<<"nihao";
}
static void test09(){
    MODULE_DEBUG("A", LOGGER_GET("system"))<<"nihao";
    MODULE_DEBUG("A", LOGGER_ROOT())<<"root";
    MODULE_DEBUG("B", LOGGER_GET("system"))<<"nihao";
    MODULE_DEBUG("B", LOGGER_ROOT())<<"root";
    MODULE_DEBUG("C", LOGGER_GET("system"))<<"nihao";
    MODULE_DEBUG("C", LOGGER_GET())<<"root";
    MODULE_DEBUG("zhao", LOGGER_GET("system"))<<"nihao";
    //修改模块屏蔽等级
    MODULE_MODIFY_LEVEL("world", LOGGER_GET("system"), zhao::LogLevel::ERROR);
    MODULE_DEBUG("world", LOGGER_GET("system"))<<"nihao";
    MODULE_DEBUG("world", LOGGER_GET("system"))<<"hello world";

    MODULE_DEBUG("zhao", LOGGER_ROOT())<<"root";
    MODULE_DEBUG("world", LOGGER_ROOT())<<"root";
}
void log_test(void)
{
    //test06();
    //test07();
    test09();
}
