#ifndef __UTILS_H__
#define __UTILS_H__ 
#include <stdint.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <pthread.h>

namespace zhao
{
    typedef pid_t tid_t; 
    typedef uint64_t fid_t;
    int64_t getTimeStamp();
    tid_t getThreadId();
    fid_t getFiberId();
    int64_t getElapse();
    void setThreadName(const pthread_t thread, const std::string &name);
    std::string getThreadName();
    void backtrace(std::vector  <std::string> &bt, int size = 64, int skip = 1);
    std::string backtraceToString(int size = 64, int skip = 1);
} // namespace zhao

#endif