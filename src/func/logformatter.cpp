#include "log/logformatter.hpp"

#include <functional>
#include <iostream>
#include <ctime>
#include <chrono>
#include <iomanip>
#include "logformatter.hpp"
#include "log/logevent.hpp"
#include "log/logger.hpp"
namespace zhao
{

    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        MessageFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->m_content.str();
        }
        std::string toString(LogEvent::ptr event) override
        {
            return event->m_content.str();
        }
    };
    class ModuleFormatItem : public LogFormatter::FormatItem
    {
    public:
        ModuleFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->m_module_name;
        }
        std::string toString(LogEvent::ptr event) override
        {
            return event->m_module_name;
        }
    };
    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        LevelFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << LogLevel::toString(event->m_level);
        }
        std::string toString(LogEvent::ptr event) override
        {
            return LogLevel::toString(event->m_level);
        }
    };
    class ElapseFormatItem : public LogFormatter::FormatItem
    {
    public:
        ElapseFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->m_elapse;
        }
        std::string toString(LogEvent::ptr event) override
        {
            return std::to_string(event->m_elapse);
        }
    };
    class NameFormatItem : public LogFormatter::FormatItem
    {
    public:
        NameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->m_logger->getName();
        }
        std::string toString(LogEvent::ptr event) override
        {
            return event->m_logger->getName();
        }
    };
    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->m_threadId;
        }
        std::string toString(LogEvent::ptr event) override
        {
            return std::to_string(event->m_threadId);
        }
    };
    // New FormatItem subclasses
    class TimeFormatItem : public LogFormatter::FormatItem
    {
    public:
        TimeFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            std::time_t t = static_cast<std::time_t>(event->m_time);
            std::tm tm = *std::localtime(&t);
            os << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        }
        std::string toString(LogEvent::ptr event) override
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
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->m_file;
        }
        std::string toString(LogEvent::ptr event) override
        {
            return event->m_file ? event->m_file : "";
        }
    };

    class LineNumberFormatItem : public LogFormatter::FormatItem
    {
    public:
        LineNumberFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->m_line;
        }
        std::string toString(LogEvent::ptr event) override
        {
            return std::to_string(event->m_line);
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem
    {
    public:
        NewLineFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << std::endl;
        }
        std::string toString(LogEvent::ptr event) override
        {
            return "\n";
        }
    };

    class TabFormatItem : public LogFormatter::FormatItem
    {
    public:
        TabFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << "\t";
        }
        std::string toString(LogEvent::ptr event) override
        {
            return "\t";
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        FiberIdFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->m_fiberId;
        }
        std::string toString(LogEvent::ptr event) override
        {
            return std::to_string(event->m_fiberId);
        }
    };

    class ThreadNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadNameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->m_threadName;
        }
        std::string toString(LogEvent::ptr event) override
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
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << m_format;
        }
        std::string toString(LogEvent::ptr event) override
        {
            return m_format;
        }
    };
    LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern)
    {
        if(m_pattern.empty()){
            std::cout<< "warning: pattern is empty"<<std::endl;
        }
        init();
    }

    std::string LogFormatter::format(LogEvent::ptr event)
    {
        //std::cout << "m_items.size()==>" << m_items.size() << std::endl;
        std::stringstream ss;
        for (auto &i : m_items)
        {
            if(i)
                i->format(ss, event);
        }
        return ss.str();
    }
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
            case 'M':
                type = MODULE;
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
        static std::map<LogFormatter::FormatItemType, std::function<FormatItem::ptr(const std::string &str)>> s_type2format = {
#define XX(type, C)                                                              \
    {                                                                            \
        type, [](const std::string &str) { return FormatItem::ptr(new C(str)); } \
    }
            XX(MESSAGE, MessageFormatItem),
            XX(MODULE, ModuleFormatItem),
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
        //std::cout << "m_items.size()==>" << m_items.size() << std::endl;
    }
}