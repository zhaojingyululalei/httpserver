#ifndef __UTILS_H__
#define __UTILS_H__ 
#include <stdint.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <pthread.h>
#include <iostream>

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
    uint64_t getCurrentMS();
    uint64_t getCurrentUS();
    class FSUtil {
public:
    static void ListAllFile(std::vector<std::string>& files
                            ,const std::string& path
                            ,const std::string& subfix);
    static bool Mkdir(const std::string& dirname);
    static bool IsRunningPidfile(const std::string& pidfile);
    static bool Rm(const std::string& path);
    static bool Mv(const std::string& from, const std::string& to);
    static bool Realpath(const std::string& path, std::string& rpath);
    static bool Symlink(const std::string& frm, const std::string& to);
    static bool Unlink(const std::string& filename, bool exist = false);
    static std::string Dirname(const std::string& filename);
    static std::string Basename(const std::string& filename);
    static bool OpenForRead(std::ifstream& ifs, const std::string& filename
                    ,std::ios_base::openmode mode);
    static bool OpenForWrite(std::ofstream& ofs, const std::string& filename
                    ,std::ios_base::openmode mode);
};
} // namespace zhao

#endif