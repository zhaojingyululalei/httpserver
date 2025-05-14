#ifndef __LOG_H
#define __LOG_H
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <list>
#include <sstream>
#include <vector>
#include <map>

#include <stdint.h>
#include <stdarg.h>
#include "utils.h"
#include "singleton.h"
#include "ipc.h"

#define ZHAO_LOG(logger, level)      \
    if (logger->getLevel() <= level) \
    zhao::LogEventWrap(zhao::LogEvent::Ptr(new zhao::LogEvent(logger, level, __FILE__, __LINE__, zhao::getElapse(), zhao::getThreadId(), zhao::getFiberId(), zhao::getTimeStamp(), zhao::getThreadName(), "")))

#define ZHAO_LOG_INFO(logger) ZHAO_LOG(logger, zhao::LogLevel::INFO)
#define ZHAO_LOG_DEBUG(logger) ZHAO_LOG(logger, zhao::LogLevel::DEBUG)
#define ZHAO_LOG_WARN(logger) ZHAO_LOG(logger, zhao::LogLevel::WARN)
#define ZHAO_LOG_ERROR(logger) ZHAO_LOG(logger, zhao::LogLevel::ERROR)
#define ZHAO_LOG_FATAL(logger) ZHAO_LOG(logger, zhao::LogLevel::FATAL)

#define ZHAO_LOG_FMT(logger, level, fmt, ...) \
    if (logger->getLevel() <= level)          \
    zhao::LogEventWrap(zhao::LogEvent::Ptr(new zhao::LogEvent(logger, level, __FILE__, __LINE__, zhao::getElapse(), zhao::getThreadId(), zhao::getFiberId(), zhao::getTimeStamp(), zhao::getThreadName(), zhao::MessageFormat(fmt, __VA_ARGS__))))

#define ZHAO_LOG_FMT_INFO(logger, fmt, ...) \
    ZHAO_LOG_FMT(logger, zhao::LogLevel::INFO, fmt, __VA_ARGS__)
#define ZHAO_LOG_FMT_DEBUG(logger, fmt, ...) \
    ZHAO_LOG_FMT(logger, zhao::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define ZHAO_LOG_FMT_WARN(logger, fmt, ...) \
    ZHAO_LOG_FMT(logger, zhao::LogLevel::WARN, fmt, __VA_ARGS__)
#define ZHAO_LOG_FMT_ERROR(logger, fmt, ...) \
    ZHAO_LOG_FMT(logger, zhao::LogLevel::ERROR, fmt, __VA_ARGS__)
#define ZHAO_LOG_FMT_FATAL(logger, fmt, ...) \
    ZHAO_LOG_FMT(logger, zhao::LogLevel::FATAL, fmt, __VA_ARGS__)

#define GET_ROOT_LOGGER() zhao::LoggerMgr::getInstance()->getRoot()
#define GET_LOGGER(name) zhao::LoggerMgr::getInstance()->getLogger(name)
#define LOG_DEFAULT_LEVEL 0

#define LOG_DEFAULT_PATTERN "time:%d  |  tid:%t  |  threadname:%N  |  fiberid:%F  |  dbg:[%p]  |  logger:[%c]  |  file:%f  |  line:%l  |  content:%m%n"

namespace zhao
{
    class LogLevel;
    class LogFormatter;
    class LogEvent;
    class Logger;

    std::string MessageFormat(const char *fmt, ...);
    /**
     * @brief 日志级别
     */
    class LogLevel
    {
    public:
        enum Level
        {
            UNKNOW = 0,
            INFO = 1,
            DEBUG = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5,
            MAX = 6,

        };

        /**
         * @brief 将日志级别转成文本输出
         * @param[in] level 日志级别
         */
        static const char *toString(LogLevel::Level level);

        /**
         * @brief 将文本转换成日志级别
         * @param[in] str 日志级别文本
         */
        static LogLevel::Level fromString(const std::string &str);
    };

    /**
     * @brief 日志事件
     */
    class LogEvent
    {
    public:
        const char *m_file = nullptr;     /// 文件名
        uint32_t m_line = 0;              /// 行号
        uint32_t m_elapse = 0;            /// 程序启动开始到现在的毫秒数
        uint32_t m_threadId = 0;          /// 线程ID
        uint32_t m_fiberId = 0;           /// 协程ID
        uint64_t m_time = 0;              /// 时间戳
        std::string m_threadName;         /// 线程名称
        std::shared_ptr<Logger> m_logger; /// 日志器
        LogLevel::Level m_level;          /// 日志等级
        std::string m_message;            /// 日志内容

    public:
        typedef std::shared_ptr<LogEvent> Ptr;
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time, const std::string &thread_name, const std::string &message)
            : m_logger(logger), m_level(level), m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id), m_time(time), m_threadName(thread_name), m_message(message) {}
    };

    /**
     * @brief 日志格式器
     */
    class LogFormatter
    {
        /**
         * @brief 构造函数
         * @param[in] pattern 格式模板
         * @details
         *  %m 消息
         *  %p 日志级别
         *  %r 累计毫秒数
         *  %c 日志名称
         *  %t 线程id
         *  %n 换行
         *  %d 时间
         *  %f 文件名
         *  %l 行号
         *  %T 制表符
         *  %F 协程id
         *  %N 线程名称
         *
         *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
         */
    public:
        enum FormatItemType
        {
            // 消息
            MESSAGE,
            // 日志级别
            LEVEL,
            // 累计毫秒数
            ELAPSE,
            // 日志名称
            NAME,
            // 线程id
            THREAD_ID,
            // 时间
            TIME,
            // 文件名
            FILE,
            // 行号
            LINE,
            // 换行
            NEWLINE,
            // 制表符
            TAB,
            // 协程id
            FIBER_ID,
            // 线程名称
            THREAD_NAME,
            // 未知
            UNKNOWN
        };
        typedef std::shared_ptr<LogFormatter> Ptr;
        LogFormatter(const std::string &pattern) : m_pattern(pattern) { init(); }
        class FormatItem
        {
        public:
            typedef std::shared_ptr<FormatItem> Ptr;
            virtual ~FormatItem() {}
            virtual void format(std::ostream &os, LogEvent::Ptr event) = 0;
            virtual std::string toString(LogEvent::Ptr event) = 0;
        };
        // 解析m_pattern，把各个项放入vecs中
        void init();
        // 把m_items中的各个项组合起来，形成日志字符串返回
        std::string format(LogEvent::Ptr event);
        // 日志解析过程中是否有错
        bool isError() const { return m_error; }
        std::string getPattern() const { return m_pattern; }
    private:
        // 传入的模式字符串
        std::string m_pattern;
        // 针对每种格式的解析
        std::vector<FormatItem::Ptr> m_items;
        // 是否有错误
        bool m_error = false;
    };

    class LogAppender
    {
    private:
    protected:
        LogLevel::Level m_level;       // 输出登记
        LogFormatter::Ptr m_formatter; // 格式刷
    public:
        typedef std::shared_ptr<LogAppender> Ptr;
        virtual ~LogAppender() {};

        virtual void log(LogLevel::Level level, LogEvent::Ptr event) = 0;
        virtual std::string toYamlString(void) = 0;
        void setFormatter(LogFormatter::Ptr formatter) { m_formatter = formatter; }
        void setLevel(LogLevel::Level level) { m_level = level; }
        LogFormatter::Ptr getFormatter(void) { return m_formatter; }
    };

    class ConsoleAppender : public LogAppender
    {
    private:
    public:
        typedef std::shared_ptr<ConsoleAppender> Ptr;
        void log(LogLevel::Level level, LogEvent::Ptr event) override;
        std::string toYamlString(void) override;
    };

    class FileAppender : public LogAppender
    {
    private:
        std::string m_filename;
        std::ofstream m_filestream;

    public:
        typedef std::shared_ptr<FileAppender> Ptr;
        void log(LogLevel::Level level, LogEvent::Ptr event) override;
        // 重新打开文件，成功返回true
        bool reopen(void);
        FileAppender(const std::string filename) : m_filename(filename) {reopen();}
        std::string toYamlString(void) override;
    };

    class Logger
    {
    private:
        std::string m_name;                      // 日志名称
        LogLevel::Level m_level;                 // 日志等级
        std::list<LogAppender::Ptr> m_appenders; // 输出目的地集合
        LogFormatter::Ptr m_formatter;           // 日志格默认格式器
    public:
        typedef std::shared_ptr<Logger> Ptr;
        Logger(const std::string &name = "default");

        void log(LogLevel::Level level, LogEvent::Ptr event);

        // 强制输出，不管配置的什么level都必须输出
        void log_force(LogLevel::Level level, LogEvent::Ptr event, LogAppender::Ptr dest);
        void info(LogEvent::Ptr event, LogAppender::Ptr dest);
        void debug(LogEvent::Ptr event, LogAppender::Ptr dest);
        void warn(LogEvent::Ptr event, LogAppender::Ptr dest);
        void error(LogEvent::Ptr event, LogAppender::Ptr dest);
        void fatal(LogEvent::Ptr event, LogAppender::Ptr dest);

        void addAppender(LogAppender::Ptr appender);
        void delAppender(LogAppender::Ptr appender);
        void clearAppenders();
        std::string getName() { return m_name; }
        void setName(std::string &name) { m_name = name; }

        LogLevel::Level getLevel() { return m_level; }
        void setLevel(LogLevel::Level level) { m_level = level; }
        void setFormatter(LogFormatter::Ptr formatter) { m_formatter = formatter; }
        std::string toYamlString( );
    };

    class LogEventWrap
    {
    private:
        LogEvent::Ptr m_event; // 日志事件
    public:
        /**
         * @brief 构造函数
         * @param event 日志事件
         */
        LogEventWrap(LogEvent::Ptr event) : m_event(event) {}

        /**
         * @brief 析构函数
         * @details 在析构时输出日志
         */
        ~LogEventWrap()
        {
            if (m_event->m_logger)
            {
                m_event->m_logger->log(m_event->m_level, m_event);
            }
        }

        template <typename T>
        LogEventWrap &operator<<(T &&value)
        {
            std::stringstream ss;
            ss << value;
            m_event->m_message += ss.str();
            return *this;
        }
    };

    class LoggerManager
    {
    private:
        std::map<std::string, Logger::Ptr> m_loggers;
        Logger::Ptr m_root;

    public:
        LoggerManager();
        // 根据配置系统获取日志
        void init();
        Logger::Ptr getLogger(const std::string &name = "");
        Logger::Ptr getRoot() { return m_root; }
        std::string toYamlString(void);
    };
    //单例
    typedef zhao::SingletonPtr<zhao::LoggerManager> LoggerMgr;
} // namespace zhao

#endif