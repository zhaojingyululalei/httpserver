#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <chrono>
#include "log.h"
#include <functional>
#include <map>


namespace zhao
{
    

    const char *LogLevel::toString(LogLevel::Level level)
    {
        switch (level)
        {
        case LogLevel::UNKNOW:
            return "UNKNOWN";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::WARN:
            return "WARN";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
        }
    }

    LogLevel::Level LogLevel::fromString(const std::string &str)
    {
        if (str == "INFO")
            return LogLevel::INFO;
        if (str == "DEBUG")
            return LogLevel::DEBUG;
        if (str == "WARN")
            return LogLevel::WARN;
        if (str == "ERROR")
            return LogLevel::ERROR;
        if (str == "FATAL")
            return LogLevel::FATAL;
        return LogLevel::UNKNOW;
    }

    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        MessageFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::Ptr event) override
        {
            os << event->m_message;
        }
        std::string toString(LogEvent::Ptr event) override
        {
            return event->m_message;
        }
    };
    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        LevelFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::Ptr event) override
        {
            os << LogLevel::toString(event->m_level);
        }
        std::string toString(LogEvent::Ptr event) override
        {
            return LogLevel::toString(event->m_level);
        }
    };
    class ElapseFormatItem : public LogFormatter::FormatItem
    {
    public:
        ElapseFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::Ptr event) override
        {
            os << event->m_elapse;
        }
        std::string toString(LogEvent::Ptr event) override
        {
            return std::to_string(event->m_elapse);
        }
    };
    class NameFormatItem : public LogFormatter::FormatItem
    {
    public:
        NameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::Ptr event) override
        {
            os << event->m_logger->getName();
        }
        std::string toString(LogEvent::Ptr event) override
        {
            return event->m_logger->getName();
        }
    };
    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::Ptr event) override
        {
            os << event->m_threadId;
        }
        std::string toString(LogEvent::Ptr event) override
        {
            return std::to_string(event->m_threadId);
        }
    };
    // New FormatItem subclasses
    class TimeFormatItem : public LogFormatter::FormatItem
    {
    public:
        TimeFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::Ptr event) override
        {
            std::time_t t = static_cast<std::time_t>(event->m_time );
            std::tm tm = *std::localtime(&t);
            os << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        }
        std::string toString(LogEvent::Ptr event) override
        {
            std::ostringstream oss;
            format(oss, event);
            return oss.str();
        }
    };

    class FileNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        FileNameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::Ptr event) override
        {
            os << event->m_file;
        }
        std::string toString(LogEvent::Ptr event) override
        {
            return event->m_file ? event->m_file : "";
        }
    };

    class LineNumberFormatItem : public LogFormatter::FormatItem
    {
    public:
        LineNumberFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::Ptr event) override
        {
            os << event->m_line;
        }
        std::string toString(LogEvent::Ptr event) override
        {
            return std::to_string(event->m_line);
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem
    {
    public:
        NewLineFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::Ptr event) override
        {
            os << std::endl;
        }
        std::string toString(LogEvent::Ptr event) override
        {
            return "\n";
        }
    };

    class TabFormatItem : public LogFormatter::FormatItem
    {
    public:
        TabFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::Ptr event) override
        {
            os << "\t";
        }
        std::string toString(LogEvent::Ptr event) override
        {
            return "\t";
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        FiberIdFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::Ptr event) override
        {
            os << event->m_fiberId;
        }
        std::string toString(LogEvent::Ptr event) override
        {
            return std::to_string(event->m_fiberId);
        }
    };

    class ThreadNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadNameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::Ptr event) override
        {
            os << event->m_threadName;
        }
        std::string toString(LogEvent::Ptr event) override
        {
            return event->m_threadName;
        }
    };

    class UnknownFormatItem : public LogFormatter::FormatItem
    {
    private:
        std::string m_format;

    public:
        UnknownFormatItem(const std::string &str = "") : m_format(str) {}
        void format(std::ostream &os, LogEvent::Ptr event) override
        {
            os << m_format;
        }
        std::string toString(LogEvent::Ptr event) override
        {
            return m_format;
        }
    };
    void LogFormatter::init()
    {
        // %x type
        std::vector<std::pair<std::string, LogFormatter::FormatItemType>> vec;
        size_t pos = 0;
        size_t start = 0;
        std::string token;
        std::string pattern = m_pattern;

        while ((pos = pattern.find('%', start)) != std::string::npos)
        {
            if (pos > start)
            {
                // 处理 % 之前的普通文本
                token = pattern.substr(start, pos - start);
                vec.push_back(std::make_pair(token, UNKNOWN));
            }

            // 处理 % 开头的格式项
            if (pos + 1 >= pattern.length())
            {
                // 如果 % 是最后一个字符，直接结束
                m_error = true;
                return;
            }

            char formatChar = pattern[pos + 1];
            std::string formatToken = "%" + std::string(1, formatChar);
            LogFormatter::FormatItemType type = UNKNOWN;

            // 根据格式字符确定类型
            switch (formatChar)
            {
            case 'm':
                type = MESSAGE;
                break;
            case 'p':
                type = LEVEL;
                break;
            case 'r':
                type = ELAPSE;
                break;
            case 'c':
                type = NAME;
                break;
            case 't':
                type = THREAD_ID;
                break;
            case 'n':
                type = NEWLINE;
                break;
            case 'd':
                type = TIME;
                break;
            case 'f':
                type = FILE;
                break;
            case 'l':
                type = LINE;
                break;
            case 'T':
                type = TAB;
                break;
            case 'F':
                type = FIBER_ID;
                break;
            case 'N':
                type = THREAD_NAME;
                break;
            default:
                // 未知格式字符
                m_error = true;
                return;
            }

            vec.push_back(std::make_pair(formatToken, type));
            start = pos + 2; // 跳过 % 和格式字符
        }

        // 处理剩余的普通文本
        if (start < pattern.length())
        {
            token = pattern.substr(start);
            vec.push_back(std::make_pair(token, UNKNOWN));
        }

        // 将解析结果转换为 FormatItem
        static std::map<LogFormatter::FormatItemType, std::function<FormatItem::Ptr(const std::string &str)>> s_type2format = {
#define XX(type, C)                                                                            \
    {                                                                                          \
        type, [](const std::string &str) { return FormatItem::Ptr(std::make_shared<C>(str)); } \
    }
            XX(MESSAGE, MessageFormatItem),
            XX(LEVEL, LevelFormatItem),
            XX(ELAPSE, ElapseFormatItem),
            XX(NAME, NameFormatItem),
            XX(THREAD_ID, ThreadIdFormatItem),
            XX(TIME, TimeFormatItem),
            XX(FILE, FileNameFormatItem),
            XX(LINE, LineNumberFormatItem),
            XX(NEWLINE, NewLineFormatItem),
            XX(TAB, TabFormatItem),
            XX(FIBER_ID, FiberIdFormatItem),
            XX(THREAD_NAME, ThreadNameFormatItem),
            XX(UNKNOWN, UnknownFormatItem)
#undef XX
        };

        // 将解析结果转换为 FormatItem
        for (auto &i : vec)
        {
            if (i.second != UNKNOWN)
            {
                m_items.push_back(s_type2format[i.second](i.first));
            }
            else
            {
                m_items.push_back(s_type2format[i.second](i.first));
            }
        }
    }

    void LoggerManager::init()
    {
        return;
    }
    Logger::Ptr LoggerManager::getLogger(const std::string &name)
    {
        Logger::Ptr ret_logger = m_loggers.find(name) == m_loggers.end() ? m_root : m_loggers[name];
        return ret_logger;
    }

    // LogFormatter::format() implementation
    std::string LogFormatter::format(LogEvent::Ptr event)
    {
        std::stringstream ss;
        for (auto &i : m_items)
        {
            i->format(ss, event);
        }
        return ss.str();
    }

    // FileAppender::log() implementation
    void FileAppender::log(LogLevel::Level level, LogEvent::Ptr event)
    {
        if (level >= m_level)
        {
            m_filestream << m_formatter->format(event);
        }
    }

    // ConsoleAppender::log() implementation
    void ConsoleAppender::log(LogLevel::Level level, LogEvent::Ptr event)
    {
        if (level >= m_level)
        {
            std::cout << m_formatter->format(event) << std::endl;
        }
    }

    Logger::Logger(const std::string &name) : m_name(name)
    {
        m_level = static_cast<LogLevel::Level>(LOG_DEFAULT_LEVEL);
        // 创建默认格式器
        if (!m_formatter)
        {
            m_formatter = std::make_shared<LogFormatter>(LOG_DEFAULT_PATTERN);
        }
        // 创建默认输出流
        addAppender(std::make_shared<ConsoleAppender>());
    }

    // Logger::log() implementation
    void Logger::log(LogLevel::Level level, LogEvent::Ptr event)
    {
        if (m_appenders.empty())
        {
            std::cout << "appenders is empty" << std::endl;
        }
        if (level >= m_level)
        {
            for (auto &appender : m_appenders)
            {
                appender->log(level, event);
            }
        }
    }

    // Logger::log_force() implementation
    void Logger::log_force(LogLevel::Level level, LogEvent::Ptr event, LogAppender::Ptr dest)
    {
        dest->log(level, event);
    }

    // Logger::info() implementation
    void Logger::info(LogEvent::Ptr event, LogAppender::Ptr dest)
    {
        log_force(LogLevel::INFO, event, dest);
    }

    // Logger::debug() implementation
    void Logger::debug(LogEvent::Ptr event, LogAppender::Ptr dest)
    {
        log_force(LogLevel::DEBUG, event, dest);
    }

    // Logger::warn() implementation
    void Logger::warn(LogEvent::Ptr event, LogAppender::Ptr dest)
    {
        log_force(LogLevel::WARN, event, dest);
    }

    // Logger::error() implementation
    void Logger::error(LogEvent::Ptr event, LogAppender::Ptr dest)
    {
        log_force(LogLevel::ERROR, event, dest);
    }

    // Logger::fatal() implementation
    void Logger::fatal(LogEvent::Ptr event, LogAppender::Ptr dest)
    {
        log_force(LogLevel::FATAL, event, dest);
    }

    // Logger::addAppender() implementation
    void Logger::addAppender(LogAppender::Ptr appender)
    {
        if (!appender->getFormatter())
        {
            appender->setFormatter(m_formatter);
        }
        m_appenders.push_back(appender);
    }

    // Logger::delAppender() implementation
    void Logger::delAppender(LogAppender::Ptr appender)
    {
        m_appenders.remove(appender);
    }

    // FileAppender::reopen() implementation
    bool FileAppender::reopen(void)
    {
        if (m_filestream)
        {
            m_filestream.close();
        }
        m_filestream.open(m_filename.c_str(), std::ios::app);
        return m_filestream.is_open();
    }

    LoggerManager::LoggerManager()
    {
        m_root = std::make_shared<Logger>("root");

    }

} // namespace zhao