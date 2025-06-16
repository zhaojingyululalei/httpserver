#include "utils.h"
#include <sys/time.h>
#include <iostream>
#include <sys/syscall.h>
#include <unistd.h>
#include <pthread.h>
#include <cstring>
#include <stdarg.h>
#include <execinfo.h>
#include <sstream>
#include "log.h"
#include "fiber.h"
namespace zhao
{
    static Logger::Ptr system_log = GET_LOGGER("system");
    int64_t getTimeStamp()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec;
    }

    tid_t getThreadId()
    {
        return syscall(SYS_gettid);
    }
    uint64_t getCurrentMS()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
    }
    fid_t getFiberId()
    {
        Fiber::Ptr cur = Fiber::getThis();
        if (cur)
        {
            return cur->getId();
        }
        return 0;
    }
    int64_t getElapse()
    {
        return 0;
    }
    // 设置线程名
    void setThreadName(const pthread_t thread, const std::string &name)
    {
        if (name.size() > 15)
        {
            std::cerr << "Thread name is too long (max 15 characters)!" << std::endl;
            return;
        }

        int ret = pthread_setname_np(thread, name.c_str());
        if (ret != 0)
        {
            std::cerr << "Failed to set thread name: " << strerror(ret) << std::endl;
        }
    }
    std::string getThreadName()
    {
        char name[16] = {0};
        pthread_t thread = pthread_self();
        int ret = pthread_getname_np(thread, name, sizeof(name));
        if (ret != 0)
        {
            std::cerr << "Failed to get thread name: " << strerror(ret) << std::endl;
            return "unknown";
        }
        return std::string(name);
    }
    void backtrace(std::vector<std::string> &bt, int size, int skip)
    {
        void **callstack = new void *[size];
        int nMaxFrames = ::backtrace(callstack, size);
        char **symbols = backtrace_symbols(callstack, nMaxFrames);

        if (symbols != nullptr)
        {
            for (int i = skip; i < nMaxFrames && i < size; ++i)
            {
                bt.push_back(symbols[i]);
            }
            free(symbols);
        }
        else
        {
            ZHAO_LOG_FATAL(system_log) << "backtrace_symbols error";
        }

        delete[] callstack;
    }
    std::string backtraceToString(int size, int skip)
    {
        std::stringstream ss;
        std::vector<std::string> bt;
        backtrace(bt, size, skip); // 调用上面的 backtrace 函数填充 vector

        ss << "Backtrace:" << std::endl;
        for (const auto &frame : bt)
        {
            ss << frame << std::endl;
        }

        return ss.str();
    }
    std::string MessageFormat(const char *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        char buffer[1024]; // 假设最大长度为 1024
        vsnprintf(buffer, sizeof(buffer), fmt, ap);
        va_end(ap);
        return std::string(buffer);
    }
} // namespace zhao
