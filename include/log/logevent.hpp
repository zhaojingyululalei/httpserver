#ifndef __ZHAO_LOGEVENT_HPP__
#define __ZHAO_LOGEVENT_HPP__



#include <memory>
#include <string>
#include <sstream>
#include <stdint.h>
#include "log/loglevel.hpp"
#include "utils/utils.hpp"
namespace zhao
{
    class Logger;
    
    class LogEvent
    {
    public:
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent(const char* file, int32_t line, uint32_t elapse
            ,tid_t thread_id, uint32_t fiber_id, uint64_t time
            ,const std::string& thread_name);
        ~LogEvent(){}
        void setModuleName(const std::string &name)
        {
            m_module_name = name;
        }
        void setLevel(LogLevel::Level level)
        {
            m_level = level;
        }

    public:
        /// 文件名
        const char *m_file = nullptr;
        // 模块名
        std::string m_module_name;
        /// 行号
        int32_t m_line = 0;
        /// 程序启动开始到现在的毫秒数
        uint32_t m_elapse = 0;
        /// 线程ID
        tid_t m_threadId = 0;
        /// 协程ID
        uint32_t m_fiberId = 0;
        /// 时间戳
        uint64_t m_time = 0;
        /// 线程名称
        std::string m_threadName;
        /// 日志内容流
        std::stringstream m_content;
        /// 日志器
        std::shared_ptr<Logger> m_logger;
        /// 日志等级
        LogLevel::Level m_level;
    };

    class LogEventWrapper
    {
    public:
        LogEventWrapper(std::shared_ptr<Logger> logger,LogEvent::ptr event,LogLevel::Level level,const std::string& module_name="root");
        
        ~LogEventWrapper();
        std::ostream& getSS()
        {
            return m_event->m_content;
        }
    private:
        LogEvent::ptr m_event;
        std::shared_ptr<Logger> m_logger;
        LogLevel::Level m_level;
        std::string m_module_name;
    };
}
#endif