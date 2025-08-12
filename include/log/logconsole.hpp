#ifndef __ZHAO_LOGCONSOLE_HPP__
#define __ZHAO_LOGCONSOLE_HPP__

#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include "log/logevent.hpp"
#include "log/logformatter.hpp"
namespace zhao
{
    
    class LogLevel;
    class LogFormatter;
    class LogEvent;

    class LogConsole
    {
    public:
        typedef std::shared_ptr<LogConsole> ptr;

        virtual void log(LogEvent::ptr event) = 0;
        virtual ~LogConsole(){}
        LogFormatter::ptr getFormatter(){return m_formatter;}
        void setFormatter(LogFormatter::ptr formatter){m_formatter = formatter;}
    protected:
        LogFormatter::ptr m_formatter;
    };

    class StdoutConsole:public LogConsole
    {
    public:
        typedef std::shared_ptr<StdoutConsole> ptr;
        void log(LogEvent::ptr event) override;
    };

    class FileConsole:public LogConsole
    {
    public:
        typedef std::shared_ptr<FileConsole> ptr;
        FileConsole(const std::string& filename);
        ~FileConsole(){
            m_filestream.close();
        }
        /**
         * @brief 重定向文件
         */
        void reloacation(const std::string& filename);
        void log(LogEvent::ptr event) override;
    private:
        std::string m_filename;
        std::fstream m_filestream;
    };
} // namespace zhao


#endif