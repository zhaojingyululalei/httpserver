#ifndef __LOG_H
#define __LOG_H
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <stdint.h>
#include <list>
#include <sstream>
#include <vector>
namespace zhao
{
    class LogLevel;
    class LogFormatter;
    class LogEvent;
    class Logger;

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
            FATAL = 5
        };

        /**
         * @brief 将日志级别转成文本输出
         * @param[in] level 日志级别
         */
        static const char *ToString(LogLevel::Level level);

        /**
         * @brief 将文本转换成日志级别
         * @param[in] str 日志级别文本
         */
        static LogLevel::Level FromString(const std::string &str);
    };



    class LogEvent
    {
    public:
        const char *m_file = nullptr; /// 文件名
        uint32_t m_line = 0;           /// 行号
        uint32_t m_elapse = 0;        /// 程序启动开始到现在的毫秒数
        uint32_t m_threadId = 0;      /// 线程ID
        uint32_t m_fiberId = 0;       /// 协程ID
        uint64_t m_time = 0;          /// 时间戳
        std::string m_threadName;     /// 线程名称
        std::shared_ptr<Logger> m_logger; /// 日志器
        LogLevel::Level m_level;          /// 日志等级
        std::string m_content;            

    public:
        typedef std::shared_ptr<LogEvent> Ptr;
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
            ,const char* file, int32_t line, uint32_t elapse
            ,uint32_t thread_id, uint32_t fiber_id, uint64_t time
            ,const std::string& thread_name,const std::string &content);
        
    };
    
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
        typedef std::shared_ptr<LogFormatter> Ptr;
        LogFormatter(const std::string &pattern);
        class FormatItem{
        public:
            virtual ~FormatItem() {}
            typedef std::shared_ptr<FormatItem> Ptr;
            virtual std::string toString(LogEvent::Ptr event) = 0;
        };
        //解析m_pattern，把各个项放入vecs中
        void init();
        //把vecs中的各个项组合起来，形成日志字符串返回
        std::string format(LogEvent::Ptr event);
        //日志解析过程中是否有错
        bool isError() const { return m_error;}
    private:
        //传入的模式字符串
        std::string m_pattern;
        //针对每种格式的解析
        std::vector<FormatItem::Ptr> vecs;
        // 是否有错误
        bool m_error = false;
    };









    class LogAppender
    {
    private:
    protected:
        LogLevel::Level m_level; //输出登记
        LogFormatter::Ptr m_formatter;//格式刷
    public:
        typedef std::shared_ptr<LogAppender> Ptr;
        virtual ~LogAppender(){};

        virtual void log(LogLevel::Level level,LogEvent::Ptr event) = 0;
        void setFormatter(LogFormatter::Ptr formatter){m_formatter = formatter;}
        LogFormatter::Ptr getFormatter(void){return m_formatter;}
    };




    class ConsoleAppender:public LogAppender{
    private:
    public:
        typedef std::shared_ptr<ConsoleAppender> Ptr;
        void log(LogLevel::Level level,LogEvent::Ptr event) override;
    };




    class FileAppender:public LogAppender{
    private:
        std::string m_filename;
        std::ofstream m_filestream;
    public:
        typedef std::shared_ptr<FileAppender> Ptr;
        void log(LogLevel::Level level,LogEvent::Ptr event) override;
        FileAppender(const std::string filename);

        bool reopen(void);

        
    };




    class Logger
    {
    private:
        std::string m_name; //日志名称
        LogLevel::Level m_level; //日志等级
        std::list<LogAppender::Ptr> m_destions;//输出目的地集合


    public:
        typedef std::shared_ptr<Logger> Ptr;
        Logger(const std::string& name = "default"); 

        void log(LogLevel::Level level,LogEvent::Ptr event);

        //强制输出，不管配置的什么level都必须输出
        void log_force(LogLevel::Level level,LogEvent::Ptr event,LogAppender::Ptr dest);
        void info(LogEvent::Ptr event,LogAppender::Ptr dest);
        void debug(LogEvent::Ptr event,LogAppender::Ptr dest);
        void warn(LogEvent::Ptr event,LogAppender::Ptr dest);
        void error(LogEvent::Ptr event,LogAppender::Ptr dest);
        void fatal(LogEvent::Ptr event,LogAppender::Ptr dest);

        void addDestion(LogAppender::Ptr dest);
        void removeDestion(LogAppender::Ptr dest);
        
        std::string getName(){return m_name;};
        void setName(std::string& name){m_name = name;};

        LogLevel::Level getLevel(){return m_level;};
        void setLevel(LogLevel::Level level){m_level = level;};


    };


} // namespace zhao

#endif