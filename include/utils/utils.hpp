#ifndef __ZHAO_UTILS_H__
#define __ZHAO_UTILS_H__
#include "noncopyable.hpp"
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdint.h>
#include <stddef.h>
#include <string>
namespace zhao
{
    typedef pid_t tid_t;
    class Utils : public Noncopyable
    {
    public:
        static tid_t getCurrentThreadId();
        static uint32_t getFiberId();
        static std::string getCurrentThreadName();
        static size_t getCpuCoreCount();
        class Memory
        {
        public:
            static size_t getPageSize();
        };

        class Math
        {
        public:
            static size_t alignUp(size_t size, size_t alignNum)
            { // alignNum是size对应分区的对齐数
                return ((size + alignNum - 1) & ~(alignNum - 1));
            }
            static int log2_pow2(unsigned int n)
            {
                if (n == 0)
                    return -1; // 0 不是 2 的幂
                int power = 0;
                while (n >>= 1)
                { // 右移直到 n = 1
                    power++;
                }
                return power;
            }
        };

        class Time
        {
        public:
            static uint64_t getCurrentMS()
            {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
            }
            static uint64_t getCurrentUS()
            {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
            }
            static uint64_t getCurrentSec()
            {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                return tv.tv_sec;
            }
        };
        class File
        {
        public:
        };
        class Str
        {
        public:
            static std::string Trim(const std::string &str, const std::string &delimit=" \t\r\n")
            {
                auto begin = str.find_first_not_of(delimit);
                if (begin == std::string::npos)
                {
                    return "";
                }
                auto end = str.find_last_not_of(delimit);
                return str.substr(begin, end - begin + 1);
            }
        };
    };
} // namespace zhao

#endif