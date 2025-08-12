#ifndef __ZHAO_LOGGER_HPP_
#define __ZHAO_LOGGER_HPP_
#include "log/loglevel.hpp"
#include "log/logformatter.hpp"
#include "log/logevent.hpp"
#include "log/logconsole.hpp"
#include "utils/utils.hpp"
#include "utils/singleton.hpp"
#include "thread/ipc.hpp"
#include <memory>
#include <list>
#include <map>
#include <vector>
#include <set>
#include <time.h>

#define LOG_DEFAULT_PATTERN "time:%d||content:{%n%m%n}"

namespace zhao
{

    class Logger : public std::enable_shared_from_this<Logger>
    {
    public:
        typedef std::shared_ptr<Logger> ptr;
        Logger(const std::string &name = "root");

        void log(LogEvent::ptr event, LogLevel::Level level, const std::string &module_name = "root");
        void debug(LogEvent::ptr event, const std::string &module_name = "root");
        void info(LogEvent::ptr event, const std::string &module_name = "root");
        void warn(LogEvent::ptr event, const std::string &module_name = "root");
        void error(LogEvent::ptr event, const std::string &module_name = "root");
        void fatal(LogEvent::ptr event, const std::string &module_name = "root");

        void addConsole(LogConsole::ptr console);
        void delConsole(LogConsole::ptr console);
        void clearConsoles()
        {
            Mutex::MutexLockGuardType guard(m_mutex);
            m_appenders.clear();
        }

        void addModule(const std::string &name, LogLevel::Level level = LogLevel::DEBUG);
        void delModule(const std::string &name);
        void modifyModule(const std::string &name, LogLevel::Level new_level);
        void maskModule(const std::string &name);
        void maskExcludeModule(const std::vector<std::string> &names);

        std::string getName();
        LogFormatter::ptr getFormatter(){return m_formatter;}
        void setFormatter(const std::string& pattern){
            LogFormatter::ptr new_val(new LogFormatter(pattern));
            m_formatter = new_val;
        }
        std::set<std::string>& getExcludeModules(){return m_excludes;}
        std::set<std::string>& getMasks(){return m_masks;}
    private:
        std::string m_name; // 日志名字
        LogFormatter::ptr m_formatter;
        std::map<std::string, LogLevel::Level> m_modules; // 模块名称和屏蔽等级
        std::list<LogConsole::ptr> m_appenders;           // Console集合
        std::set<std::string> m_masks;
        std::set<std::string> m_excludes;
        Mutex m_mutex;
    };

    class LogManager
    {
    public:
        LogManager();
        Logger::ptr getLogger(const std::string &name="root");
        void addLogger(Logger::ptr logger){
            Mutex::MutexLockGuardType guard(m_mutex);
            m_loggers[logger->getName()] = logger;
        }
        void delLogger(const std::string &name){
            Mutex::MutexLockGuardType guard(m_mutex);
            m_loggers.erase(name);
        }
    private:
        std::map<std::string, Logger::ptr> m_loggers;
        
        Logger::ptr m_root;
        Mutex m_mutex;
    };
    typedef Singleton<LogManager> LoggerMgr;

} // namespace zhao

#define MODULE_LOG_LEVEL(module, logger, level) \
    if (true)                                   \
    zhao::LogEventWrapper(logger, zhao::LogEvent::ptr(new zhao::LogEvent(__FILE__, __LINE__, 0, zhao::Utils::getCurrentThreadId(), zhao::Utils::getFiberId(), time(0),zhao::Utils::getCurrentThreadName())), level, module).getSS()

#define MODULE_DEBUG(module, logger) MODULE_LOG_LEVEL(module, logger, zhao::LogLevel::DEBUG)
#define MODULE_INFO(module, logger) MODULE_LOG_LEVEL(module, logger, zhao::LogLevel::INFO)
#define MODULE_WARN(module, logger) MODULE_LOG_LEVEL(module, logger, zhao::LogLevel::WARN)
#define MODULE_ERROR(module, logger) MODULE_LOG_LEVEL(module, logger, zhao::LogLevel::ERROR)
#define MODULE_FATAL(module, logger) MODULE_LOG_LEVEL(module, logger, zhao::LogLevel::FATAL)
#define MODULE_MODIFY_LEVEL(module, logger, level) logger->modifyModule(module, level)

#define LOG_LEVEL(logger, level) \
    if (true)                    \
    zhao::LogEventWrapper(logger, zhao::LogEvent::ptr(new zhao::LogEvent(__FILE__, __LINE__, 0, zhao::Utils::getCurrentThreadId(), zhao::Utils::getFiberId(), time(0),zhao::Utils::getCurrentThreadName())), level).getSS()

#define LOG_DEBUG(logger) LOG_LEVEL(logger, zhao::LogLevel::DEBUG)
#define LOG_INFO(logger) LOG_LEVEL(logger, zhao::LogLevel::INFO)
#define LOG_WARN(logger) LOG_LEVEL(logger, zhao::LogLevel::WARN)
#define LOG_ERROR(logger) LOG_LEVEL(logger, zhao::LogLevel::ERROR)
#define LOG_FATAL(logger) LOG_LEVEL(logger, zhao::LogLevel::FATAL)

#define LOGGER_ROOT() zhao::LoggerMgr::getInstance()->getLogger()
#define LOGGER_GET(name) zhao::LoggerMgr::getInstance()->getLogger(name)
#define LOGGER_ADD(logger) zhao::LoggerMgr::getInstance()->addLogger(logger)
#define LOGGER_DEL(name) zhao::LoggerMgr::getInstance()->delLogger(name)
#endif