#include "loglevel.hpp"

namespace zhao
{
    const char *LogLevel::toString(LogLevel::Level level)
    {
        switch (level)
        {
        case LogLevel::UNKNOWN:
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
        case LogLevel::OFF:
            return "OFF";
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
        if(str == "OFF")
            return LogLevel::OFF;
        return LogLevel::UNKNOWN;
    }
}