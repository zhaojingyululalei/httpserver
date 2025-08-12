#ifndef __ZHAO_LOGLEVEL_HPP__
#define __ZHAO_LOGLEVEL_HPP__

#include <string>
namespace zhao
{
    /**
 * @brief 日志级别
 */
class LogLevel {
public:
    /**
     * @brief 日志级别枚举
     */
    enum Level {
        /// 未知级别
        UNKNOWN = 0,
        /// DEBUG 级别
        DEBUG = 1,
        /// INFO 级别
        INFO = 2,
        /// WARN 级别
        WARN = 3,
        /// ERROR 级别
        ERROR = 4,
        /// FATAL 级别
        FATAL = 5,
        //关闭
        OFF = 6
    };

    static const char* toString(LogLevel::Level level);
    
  
    static LogLevel::Level fromString(const std::string& str);
};
} // namespace zhao

#endif