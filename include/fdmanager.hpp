#ifndef __FD_MANAGER_H__
#define __FD_MANAGER_H__

#include <memory>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include "thread/ipc.hpp"
#include "utils/singleton.hpp"

namespace zhao {

    class FileDescription :public std::enable_shared_from_this<FileDescription>
    {
    public:
        typedef std::shared_ptr<FileDescription> ptr;
        FileDescription(int fd);
        ~FileDescription();
        int getFd() ;
        void setUsrNoneBlock(bool flag);
        bool getUsrNoneBlock()const {return m_usr_set_noneblock;}
        void setTimeout(int type,uint64_t timeout);
        uint64_t getTimeout(int type);
        bool isSocket() const {return m_is_socket;}
        bool isClose() const {return m_is_close;}   
    private:
        int m_fd = -1;
        struct stat m_fd_state = {};
        bool m_is_socket = false;
        bool m_usr_set_noneblock = false;
        bool m_is_close = false;
        bool m_is_init = false;
        uint64_t m_recvTimeout = -1;
        uint64_t m_sendTimeout = -1;
    };

    class FdManager
    {
    public:
        typedef std::shared_ptr<FdManager> ptr;
        FdManager();
        FileDescription::ptr get(int fd, bool auto_create = true);
        void del(int fd);
    private:
        RWMutex m_mutex;
        std::vector<FileDescription::ptr> m_fds;
    };
    typedef Singleton<FdManager> FdMgr;
} // namespace zhao

#endif // __FD_MANAGER_H__
