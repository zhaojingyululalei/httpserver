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



    
std::string StringUtil::Format(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    auto v = Formatv(fmt, ap);
    va_end(ap);
    return v;
}

std::string StringUtil::Formatv(const char* fmt, va_list ap) {
    char* buf = nullptr;
    auto len = vasprintf(&buf, fmt, ap);
    if(len == -1) {
        return "";
    }
    std::string ret(buf, len);
    free(buf);
    return ret;
}

static const char uri_chars[256] = {
    /* 0 */
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 1, 1, 0,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 0, 0, 0, 1, 0, 0,
    /* 64 */
    0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 0, 1,
    0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 1, 0,
    /* 128 */
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    /* 192 */
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
};

static const char xdigit_chars[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,
    0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

#define CHAR_IS_UNRESERVED(c)           \
    (uri_chars[(unsigned char)(c)])

//-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~
std::string StringUtil::UrlEncode(const std::string& str, bool space_as_plus) {
    static const char *hexdigits = "0123456789ABCDEF";
    std::string* ss = nullptr;
    const char* end = str.c_str() + str.length();
    for(const char* c = str.c_str() ; c < end; ++c) {
        if(!CHAR_IS_UNRESERVED(*c)) {
            if(!ss) {
                ss = new std::string;
                ss->reserve(str.size() * 1.2);
                ss->append(str.c_str(), c - str.c_str());
            }
            if(*c == ' ' && space_as_plus) {
                ss->append(1, '+');
            } else {
                ss->append(1, '%');
                ss->append(1, hexdigits[(uint8_t)*c >> 4]);
                ss->append(1, hexdigits[*c & 0xf]);
            }
        } else if(ss) {
            ss->append(1, *c);
        }
    }
    if(!ss) {
        return str;
    } else {
        std::string rt = *ss;
        delete ss;
        return rt;
    }
}

std::string StringUtil::UrlDecode(const std::string& str, bool space_as_plus) {
    std::string* ss = nullptr;
    const char* end = str.c_str() + str.length();
    for(const char* c = str.c_str(); c < end; ++c) {
        if(*c == '+' && space_as_plus) {
            if(!ss) {
                ss = new std::string;
                ss->append(str.c_str(), c - str.c_str());
            }
            ss->append(1, ' ');
        } else if(*c == '%' && (c + 2) < end
                    && isxdigit(*(c + 1)) && isxdigit(*(c + 2))){
            if(!ss) {
                ss = new std::string;
                ss->append(str.c_str(), c - str.c_str());
            }
            ss->append(1, (char)(xdigit_chars[(int)*(c + 1)] << 4 | xdigit_chars[(int)*(c + 2)]));
            c += 2;
        } else if(ss) {
            ss->append(1, *c);
        }
    }
    if(!ss) {
        return str;
    } else {
        std::string rt = *ss;
        delete ss;
        return rt;
    }
}

std::string StringUtil::Trim(const std::string& str, const std::string& delimit) {
    auto begin = str.find_first_not_of(delimit);
    if(begin == std::string::npos) {
        return "";
    }
    auto end = str.find_last_not_of(delimit);
    return str.substr(begin, end - begin + 1);
}

std::string StringUtil::TrimLeft(const std::string& str, const std::string& delimit) {
    auto begin = str.find_first_not_of(delimit);
    if(begin == std::string::npos) {
        return "";
    }
    return str.substr(begin);
}

std::string StringUtil::TrimRight(const std::string& str, const std::string& delimit) {
    auto end = str.find_last_not_of(delimit);
    if(end == std::string::npos) {
        return "";
    }
    return str.substr(0, end);
}

std::string StringUtil::WStringToString(const std::wstring& ws) {
    std::string str_locale = setlocale(LC_ALL, "");
    const wchar_t* wch_src = ws.c_str();
    size_t n_dest_size = wcstombs(NULL, wch_src, 0) + 1;
    char *ch_dest = new char[n_dest_size];
    memset(ch_dest,0,n_dest_size);
    wcstombs(ch_dest,wch_src,n_dest_size);
    std::string str_result = ch_dest;
    delete []ch_dest;
    setlocale(LC_ALL, str_locale.c_str());
    return str_result;
}

std::wstring StringUtil::StringToWString(const std::string& s) {
    std::string str_locale = setlocale(LC_ALL, "");
    const char* chSrc = s.c_str();
    size_t n_dest_size = mbstowcs(NULL, chSrc, 0) + 1;
    wchar_t* wch_dest = new wchar_t[n_dest_size];
    wmemset(wch_dest, 0, n_dest_size);
    mbstowcs(wch_dest,chSrc,n_dest_size);
    std::wstring wstr_result = wch_dest;
    delete []wch_dest;
    setlocale(LC_ALL, str_locale.c_str());
    return wstr_result;
}

std::string Time2Str(time_t ts, const std::string& format) {
    struct tm tm;
    localtime_r(&ts, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), format.c_str(), &tm);
    return buf;
}
} // namespace zhao
