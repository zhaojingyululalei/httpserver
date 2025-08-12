#ifndef __ZHAO_HOOK_HPP__
#define __ZHAO_HOOK_HPP__ 
#include <stddef.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
namespace zhao
{
    /**
     * @brief 当前线程是否hook
     */
    bool is_hook_enable();
    /**
     * @brief 设置当前线程的hook状态
     */
    void set_hook_enable(bool flag);
}
extern "C" {
    typedef void* (*malloc_fun) (size_t size);
    typedef void* (*free_fun) (void* ptr);
    typedef void* (*realloc_fun) (void* ptr, size_t size);
    typedef unsigned int (*sleep_fun)(unsigned int seconds);
    typedef int (*usleep_fun)(useconds_t usec);
    // socket相关
    typedef int (*socket_fun)(int domain, int type, int protocol);
    typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    typedef int (*accept_fun)(int s, struct sockaddr *addr, socklen_t *addrlen);
    
    // read相关
    typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
    typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
    typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
    typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
    typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
    
    // write相关
    typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
    typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
    typedef ssize_t (*send_fun)(int s, const void *msg, size_t len, int flags);
    typedef ssize_t (*sendto_fun)(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
    typedef ssize_t (*sendmsg_fun)(int s, const struct msghdr *msg, int flags);
    
    // 其他系统调用
    typedef int (*close_fun)(int fd);
    typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */ );
    typedef int (*ioctl_fun)(int d, unsigned long int request, ...);
    typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
    typedef int (*setsockopt_fun)(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
    //extern malloc_fun __real_malloc;
    #define XX(name) extern name##_fun __real_##name;
    XX(malloc)
    XX(free)
    XX(realloc)
    XX(sleep)
    XX(usleep)
    // socket相关
    XX(socket)
    XX(connect)
    XX(accept)
    
    // read相关
    XX(read)
    XX(readv)
    XX(recv)
    XX(recvfrom)
    XX(recvmsg)
    
    // write相关
    XX(write)
    XX(writev)
    XX(send)
    XX(sendto)
    XX(sendmsg)
    
    // 其他系统调用
    XX(close)
    XX(fcntl)
    XX(ioctl)
    XX(getsockopt)
    XX(setsockopt)
    #undef XX
}
#endif