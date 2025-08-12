#ifndef __ZHAO_LOGFROMATTER_HPP__
#define __ZHAO_LOGFROMATTER_HPP__
#include <memory>
#include <string>
#include <vector>

#include "log/logevent.hpp"
namespace zhao
{

    
    class LogFormatter
    {
        /**
         * @brief pattern 格式
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
         *  %M 模块名称
         *  默认格式 "[%M:]%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
         */
    public:
        enum FormatItemType
        {
            // 消息
            MESSAGE,
            // 模块名称
            MODULE,
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

        class FormatItem
        {
        public:
            typedef std::shared_ptr<FormatItem> ptr;
            FormatItem(const std::string &fmt = "") {}
            virtual ~FormatItem(){}
            // 把事件对应的item输入到os中
            virtual void format(std::ostream &os, LogEvent::ptr event) = 0;
            virtual std::string toString(LogEvent::ptr event) = 0;
        };
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        LogFormatter(const std::string &pattern);
        ~LogFormatter(){}
        std::string format(LogEvent::ptr event);
        //解析m_pattern，并把各个Item存入m_items
        void init();
    private:

        std::string m_pattern;
        std::vector<FormatItem::ptr> m_items;
        bool m_error = false;
    };
} // namespace zhao

#endif