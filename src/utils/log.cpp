#include "log.h"

namespace zhao
{
    // LogLevel 的实现
    const char *LogLevel::ToString(LogLevel::Level level)
    {
        switch (level)
        {
        case LogLevel::UNKNOW:
            return "UNKNOW";
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARN:
            return "WARN";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::FATAL:
            return "FATAL";
        default:
            return "UNKNOW";
        }
    }

    LogLevel::Level LogLevel::FromString(const std::string &str)
    {
        if (str == "DEBUG")
            return LogLevel::DEBUG;
        if (str == "INFO")
            return LogLevel::INFO;
        if (str == "WARN")
            return LogLevel::WARN;
        if (str == "ERROR")
            return LogLevel::ERROR;
        if (str == "FATAL")
            return LogLevel::FATAL;
        return LogLevel::UNKNOW;
    }

    // LogEvent 的实现
    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                       const char *file, int32_t line, uint32_t elapse,
                       uint32_t thread_id, uint32_t fiber_id, uint64_t time,
                       const std::string &thread_name,const std::string &content)
        : m_logger(logger), m_level(level), m_file(file), m_line(line),
          m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id),
          m_time(time), m_threadName(thread_name),m_content(content) {}

    // Logger 的实现
    Logger::Logger(const std::string &name) : m_name(name) {}

    void Logger::log(LogLevel::Level level, LogEvent::Ptr event)
    {
        if (level >= m_level)
        {
            for (auto &i : m_destions)
            {
                i->log(level, event);
            }
        }
    }

    void Logger::log_force(LogLevel::Level level, LogEvent::Ptr event, LogAppender::Ptr dest)
    {
        dest->log(level, event);
    }

    void Logger::info(LogEvent::Ptr event, LogAppender::Ptr dest)
    {
        log_force(LogLevel::INFO, event, dest);
    }

    void Logger::debug(LogEvent::Ptr event, LogAppender::Ptr dest)
    {
        log_force(LogLevel::DEBUG, event, dest);
    }

    void Logger::warn(LogEvent::Ptr event, LogAppender::Ptr dest)
    {
        log_force(LogLevel::WARN, event, dest);
    }

    void Logger::error(LogEvent::Ptr event, LogAppender::Ptr dest)
    {
        log_force(LogLevel::ERROR, event, dest);
    }

    void Logger::fatal(LogEvent::Ptr event, LogAppender::Ptr dest)
    {
        log_force(LogLevel::FATAL, event, dest);
    }

    void Logger::addDestion(LogAppender::Ptr dest)
    {
        m_destions.push_back(dest);
    }

    void Logger::removeDestion(LogAppender::Ptr dest)
    {
        m_destions.remove(dest);
    }

    // ConsoleAppender 的实现
    void ConsoleAppender::log(LogLevel::Level level, LogEvent::Ptr event)
    {
        if (level > m_level)
        {
            std::cout << m_formatter->format(event);
        }
    }

    // FileAppender 的实现
    FileAppender::FileAppender(const std::string filename) : m_filename(filename)
    {
        m_filestream.open(m_filename, std::ios::out | std::ios::app);
        if (!m_filestream.is_open())
        {
            std::cerr << "Failed to open file: " << m_filename << std::endl;
        }
    }

    bool FileAppender::reopen(void)
    {
        if (m_filestream)
        {
            m_filestream.close();
        }
        m_filestream.open(m_filename,std::ios::out | std::ios::app);
        return !m_filestream.fail();
    }

    void FileAppender::log(LogLevel::Level level, LogEvent::Ptr event)
    {
        if (level > m_level)
        {
            m_filestream << m_formatter->format(event);
        }
    }

    // LogFormatter 的实现
    LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern)
    {
        init();
    }

    std::string LogFormatter::format(LogEvent::Ptr event)
    {
        std::stringstream ss;
        for (auto &i : vecs)
        {
            ss << i->toString(event);
        }
        return ss.str();
    }

    // FormatItem 子类的实现
    class StringFormatItem : public LogFormatter::FormatItem
    {
    public:
        StringFormatItem(const std::string &str) : m_str(str) {}
        std::string toString(LogEvent::Ptr event) override
        {
            return m_str;
        }

    private:
        std::string m_str;
    };

    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        std::string toString(LogEvent::Ptr event) override
        {
            return event->m_content;
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        std::string toString(LogEvent::Ptr event) override
        {
            return LogLevel::ToString(event->m_level);
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem
    {
    public:
        std::string toString(LogEvent::Ptr event) override
        {
            return std::to_string(event->m_elapse);
        }
    };

    class NameFormatItem : public LogFormatter::FormatItem
    {
    public:
        std::string toString(LogEvent::Ptr event) override
        {
            return event->m_logger->getName();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        std::string toString(LogEvent::Ptr event) override
        {
            return std::to_string(event->m_threadId);
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        std::string toString(LogEvent::Ptr event) override
        {
            return std::to_string(event->m_fiberId);
        }
    };

    class ThreadNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        std::string toString(LogEvent::Ptr event) override
        {
            return event->m_threadName;
        }
    };

    class DateTimeFormatItem : public LogFormatter::FormatItem
    {
    public:
        DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
            : m_format(format)
        {
            if (m_format.empty())
            {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }

        std::string toString(LogEvent::Ptr event) override
        {
            time_t t = event->m_time / 1000;
            struct tm *tm = localtime(&t);
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), tm);
            return buf;
        }

    private:
        std::string m_format;
    };

    class FilenameFormatItem : public LogFormatter::FormatItem
    {
    public:
        std::string toString(LogEvent::Ptr event) override
        {
            return event->m_file ? event->m_file : "";
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem
    {
    public:
        std::string toString(LogEvent::Ptr event) override
        {
            return std::to_string(event->m_line);
        }
    };

    class TabFormatItem : public LogFormatter::FormatItem
    {
    public:
        std::string toString(LogEvent::Ptr event) override
        {
            return "\t";
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem
    {
    public:
        std::string toString(LogEvent::Ptr event) override
        {
            return "\n";
        }
    };
    void LogFormatter::init()
    {
        std::string str;
        for (size_t i = 0; i < m_pattern.size(); ++i)
        {
            if (m_pattern[i] != '%')
            {
                str += m_pattern[i];
                continue;
            }

            if (i + 1 >= m_pattern.size())
            {
                m_error = true;
                return;
            }

            if (!str.empty())
            {
                vecs.push_back(std::make_shared<StringFormatItem>(str));
                str.clear();
            }

            char fmt = m_pattern[++i];
            switch (fmt)
            {
            case 'm': // 消息
                vecs.push_back(std::make_shared<MessageFormatItem>());
                break;
            case 'p': // 日志级别
                vecs.push_back(std::make_shared<LevelFormatItem>());
                break;
            case 'r': // 累计毫秒数
                vecs.push_back(std::make_shared<ElapseFormatItem>());
                break;
            case 'c': // 日志名称
                vecs.push_back(std::make_shared<NameFormatItem>());
                break;
            case 't': // 线程ID
                vecs.push_back(std::make_shared<ThreadIdFormatItem>());
                break;
            case 'n': // 换行
                vecs.push_back(std::make_shared<NewLineFormatItem>());
                break;
            case 'd': // 时间
            {
                std::string time_fmt;
                if (i + 1 < m_pattern.size() && m_pattern[i + 1] == '{')
                {
                    ++i;
                    size_t j = m_pattern.find('}', i);
                    if (j == std::string::npos)
                    {
                        m_error = true;
                        return;
                    }
                    time_fmt = m_pattern.substr(i + 1, j - i - 1);
                    i = j;
                }
                vecs.push_back(std::make_shared<DateTimeFormatItem>(time_fmt));
            }
            break;
            case 'f': // 文件名
                vecs.push_back(std::make_shared<FilenameFormatItem>());
                break;
            case 'l': // 行号
                vecs.push_back(std::make_shared<LineFormatItem>());
                break;
            case 'T': // 制表符
                vecs.push_back(std::make_shared<TabFormatItem>());
                break;
            case 'F': // 协程ID
                vecs.push_back(std::make_shared<FiberIdFormatItem>());
                break;
            case 'N': // 线程名称
                vecs.push_back(std::make_shared<ThreadNameFormatItem>());
                break;
            default:
                m_error = true;
                return;
            }
        }

        if (!str.empty())
        {
            vecs.push_back(std::make_shared<StringFormatItem>(str));
        }
    }
}