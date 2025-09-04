#include "fdmanager.hpp"
#include "hook.hpp"
namespace zhao
{
    FileDescription::FileDescription(int fd) : m_fd(fd)
    {
        if (fstat(m_fd, &m_fd_state) == -1)
        {
            m_is_init = false;
            m_is_socket = false;
        }
        else
        {
            m_is_init = true;
            m_is_socket = S_ISSOCK(m_fd_state.st_mode);
        }

        if(m_is_socket){
            //没有设置成非阻塞，就给他设置成非阻塞
            int flags = __real_fcntl(m_fd, F_GETFL, 0);
            if (!(flags & O_NONBLOCK)) {
                __real_fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);
            }
        }
    }
    FileDescription::~FileDescription()
    {

    }
    int FileDescription::getFd()
    {
        return m_fd;
    }
    void FileDescription::setUsrNoneBlock(bool flag)
    {
        m_usr_set_noneblock = flag;
    }
    void FileDescription::setTimeout(int type, uint64_t timeout)
    {
        if(type == SO_RCVTIMEO){
            m_recvTimeout = timeout;
        }else if(type == SO_SNDTIMEO){
            m_sendTimeout = timeout;
        }
    }
    uint64_t FileDescription::getTimeout(int type)
    {
        if(type == SO_RCVTIMEO){
            return m_recvTimeout;
        }else if(type == SO_SNDTIMEO){
            return m_sendTimeout;
        }
        return 0;
    }
    FdManager::FdManager()
    {
        m_fds.resize(512); // 初始化一个足够大的vector
    }
    FileDescription::ptr FdManager::get(int fd, bool auto_create)
    {
        RWMutex::ReadLockGuardType lock(m_mutex);
        if(m_fds.size() <= fd)
        {
            if(auto_create)
            {
                m_fds.resize(fd + 32);
            }
            else
            {
                return nullptr;
            }
        }
        if(m_fds[fd] == nullptr && auto_create)
        {
            lock.unlock(); // 解锁读锁，避免死锁
            m_mutex.wrlock();
            m_fds[fd] = std::make_shared<FileDescription>(fd);
            m_mutex.unlock(); // 解锁写锁
            lock.lock(); // 重新获取读锁
        }
        return m_fds[fd];
    }
    void FdManager::del(int fd)
    {
        RWMutex::WriteLockGuardType lock(m_mutex);
        if(fd < 0 || fd >= static_cast<int>(m_fds.size()))
        {
            return; // 无效的fd
        }
        m_fds[fd].reset(); // 释放FileDescription对象
        
    }
}