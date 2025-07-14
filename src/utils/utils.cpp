#include <sys/time.h>
#include <iostream>
#include <sys/syscall.h>
#include <unistd.h>
#include <pthread.h>
#include <cstring>
#include <stdarg.h>
#include <execinfo.h>
#include <sstream>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>

#include "utils.h"
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
    uint64_t getCurrentUS()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
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

    // fs
    // 给目录，递归输出所有文件
    void FSUtil::ListAllFile(std::vector<std::string> &files, const std::string &path, const std::string &subfix)
    {
        if (access(path.c_str(), 0) != 0)
        {
            return;
        }
        DIR *dir = opendir(path.c_str());
        if (dir == nullptr)
        {
            return;
        }
        struct dirent *dp = nullptr;
        while ((dp = readdir(dir)) != nullptr)
        {
            if (dp->d_type == DT_DIR)
            {
                if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
                {
                    continue;
                }
                ListAllFile(files, path + "/" + dp->d_name, subfix);
            }
            else if (dp->d_type == DT_REG)
            {
                std::string filename(dp->d_name);
                if (subfix.empty())
                {
                    files.push_back(path + "/" + filename);
                }
                else
                {
                    if (filename.size() < subfix.size())
                    {
                        continue;
                    }
                    if (filename.substr(filename.length() - subfix.size()) == subfix)
                    {
                        files.push_back(path + "/" + filename);
                    }
                }
            }
        }
        closedir(dir);
    }
    // 获取文件状态保存到st
    static int __lstat(const char *file, struct stat *st = nullptr)
    {
        struct stat lst;
        int ret = lstat(file, &lst);
        if (st)
        {
            *st = lst;
        }
        return ret;
    }
    // 创建目录
    static int __mkdir(const char *dirname)
    {
        if (access(dirname, F_OK) == 0)
        {
            return 0;
        }
        return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    // 递归创建目录
    bool FSUtil::Mkdir(const std::string &dirname)
    {
        if (__lstat(dirname.c_str()) == 0)
        {
            return true;
        }
        char *path = strdup(dirname.c_str());
        char *ptr = strchr(path + 1, '/');
        do
        {
            for (; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/'))
            {
                *ptr = '\0';
                if (__mkdir(path) != 0)
                {
                    break;
                }
            }
            if (ptr != nullptr)
            {
                break;
            }
            else if (__mkdir(path) != 0)
            {
                break;
            }
            free(path);
            return true;
        } while (0);
        free(path);
        return false;
    }
    // 检查指定的 PID 文件是否存在，并且该 PID 对应的进程正在运行。
    bool FSUtil::IsRunningPidfile(const std::string &pidfile)
    {
        if (__lstat(pidfile.c_str()) != 0)
        {
            return false;
        }
        std::ifstream ifs(pidfile);
        std::string line;
        if (!ifs || !std::getline(ifs, line))
        {
            return false;
        }
        if (line.empty())
        {
            return false;
        }
        pid_t pid = atoi(line.c_str());
        if (pid <= 1)
        {
            return false;
        }
        if (kill(pid, 0) != 0)
        {
            return false;
        }
        return true;
    }
    // 删除文件
    bool FSUtil::Unlink(const std::string &filename, bool exist)
    {
        if (!exist && __lstat(filename.c_str()))
        {
            return true;
        }
        return ::unlink(filename.c_str()) == 0;
    }
    // 递归删除路径，可以是文件或目录
    bool FSUtil::Rm(const std::string &path)
    {
        struct stat st;
        if (lstat(path.c_str(), &st))
        {
            return true;
        }
        if (!(st.st_mode & S_IFDIR))
        {
            return Unlink(path);
        }

        DIR *dir = opendir(path.c_str());
        if (!dir)
        {
            return false;
        }

        bool ret = true;
        struct dirent *dp = nullptr;
        while ((dp = readdir(dir)))
        {
            if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
            {
                continue;
            }
            std::string dirname = path + "/" + dp->d_name;
            ret = Rm(dirname);
        }
        closedir(dir);
        if (::rmdir(path.c_str()))
        {
            ret = false;
        }
        return ret;
    }
    // 移动或重命名文件/目录。
    bool FSUtil::Mv(const std::string &from, const std::string &to)
    {
        if (!Rm(to))
        {
            return false;
        }
        return rename(from.c_str(), to.c_str()) == 0;
    }
    // 获取文件的绝对路径（解析符号链接）。
    bool FSUtil::Realpath(const std::string &path, std::string &rpath)
    {
        if (__lstat(path.c_str()))
        {
            return false;
        }
        char *ptr = ::realpath(path.c_str(), nullptr);
        if (nullptr == ptr)
        {
            return false;
        }
        std::string(ptr).swap(rpath);
        free(ptr);
        return true;
    }
    // 创建符号链接（软链接）。
    bool FSUtil::Symlink(const std::string &from, const std::string &to)
    {
        if (!Rm(to))
        {
            return false;
        }
        return ::symlink(from.c_str(), to.c_str()) == 0;
    }
    // 获取文件路径的目录部分。
    std::string FSUtil::Dirname(const std::string &filename)
    {
        if (filename.empty())
        {
            return ".";
        }
        auto pos = filename.rfind('/');
        if (pos == 0)
        {
            return "/";
        }
        else if (pos == std::string::npos)
        {
            return ".";
        }
        else
        {
            return filename.substr(0, pos);
        }
    }
    // 获取文件名部分（不包括路径）。
    std::string FSUtil::Basename(const std::string &filename)
    {
        if (filename.empty())
        {
            return filename;
        }
        auto pos = filename.rfind('/');
        if (pos == std::string::npos)
        {
            return filename;
        }
        else
        {
            return filename.substr(pos + 1);
        }
    }
    // 以只读方式打开文件。
    bool FSUtil::OpenForRead(std::ifstream &ifs, const std::string &filename, std::ios_base::openmode mode)
    {
        ifs.open(filename.c_str(), mode);
        return ifs.is_open();
    }
    // 以写入方式打开文件。如果目录不存在，自动创建目录。
    bool FSUtil::OpenForWrite(std::ofstream &ofs, const std::string &filename, std::ios_base::openmode mode)
    {
        ofs.open(filename.c_str(), mode);
        if (!ofs.is_open())
        {
            std::string dir = Dirname(filename);
            Mkdir(dir);
            ofs.open(filename.c_str(), mode);
        }
        return ofs.is_open();
    }
} // namespace zhao
