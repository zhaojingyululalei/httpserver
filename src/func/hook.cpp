#include <dlfcn.h>
#include "hook.hpp"
#include "mem/tcmalloc.hpp"
#include "mem/pagecache.hpp"
#include "mem/common.hpp"
#include "scheduler/scheduler.hpp"
#include "fiber/fiber.hpp"
#include "utils/utils.hpp"
#include <iostream>
#include <functional> 
#define HOOK_FUN(XX) \
    XX(malloc)       \
    XX(free)         \
    XX(realloc)      \
    XX(sleep)        \
    XX(usleep)       \
    /* socket相关 */ \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    /* read相关 */   \
    XX(read)         \
    XX(readv)        \
    XX(recv)         \
    XX(recvfrom)     \
    XX(recvmsg)      \
    /* write相关 */  \
    XX(write)        \
    XX(writev)       \
    XX(send)         \
    XX(sendto)       \
    XX(sendmsg)      \
    /* 其他系统调用 */ \
    XX(close)        \
    XX(fcntl)        \
    XX(ioctl)        \
    XX(getsockopt)   \
    XX(setsockopt)

namespace zhao
{
    static thread_local bool t_hook_enable = false;
    bool is_hook_enable()
    {
        return t_hook_enable;
    }

    void set_hook_enable(bool flag)
    {
        t_hook_enable = flag;
    }
    // 保存所有原函数地址到__real_##name
    void hook_init()
    {
        // dlsym:运行时，动态从动态库.so中获取函数或者变量地址 赋值给__real_##name保存
        //__real_malloc = (malloc_fun)dlsym(RTLD_NEXT, "malloc");
#define XX(name) __real_##name = (name##_fun)dlsym(RTLD_NEXT, #name); // 把例如malloc函数地址全部保存到__real_malloc
        HOOK_FUN(XX);
#undef XX
    }

    class HookIniter
    {
    public:
        HookIniter()
        {
            zhao::hook_init();
        }
    };
    static HookIniter __hook_initer; // main函数开始前执行
}
extern "C"
{
    // malloc_fun __real_malloc = nullptr;
#define XX(name) name##_fun __real_##name = nullptr;
    HOOK_FUN(XX)
#undef XX

    void *malloc(size_t size)
    {
        // std::cout << "malloc" << std::endl;
        // printf("malloc \n");
        return zhao::tcmalloc(size);
    }
    /**
     * @brief realloc，stl中动态扩容可能会用到
     */
    void *realloc(void *ptr, size_t new_size)
    {
        //  ptr为NULL，相当于malloc(new_size)
        if (ptr == nullptr)
        {
            return zhao::tcmalloc(new_size);
        }

        // new_size为0，相当于free(ptr)
        if (new_size == 0)
        {
            zhao::tcfree(ptr);
            return nullptr;
        }

        // 获取原内存块的大小信息
        zhao::Span *span = zhao::PageCache::getInstance()->mapObjToSpan(ptr);
        if (!span)
        {
            // 如果无法映射到Span，说明不是由我们的分配器分配的内存
            return nullptr;
        }

        size_t old_size = span->obj_size;

        //  新大小小于等于原大小，可以原地缩减
        if (new_size <= old_size)
        {
            return ptr;
        }

        // 需要分配更大的内存块
        void *new_ptr = zhao::tcmalloc(new_size);
        if (!new_ptr)
        {
            return nullptr; // 分配失败
        }

        // 拷贝原数据到新内存块
        size_t copy_size = old_size < new_size ? old_size : new_size;
        memcpy(new_ptr, ptr, copy_size);

        // 释放原内存块
        zhao::tcfree(ptr);

        return new_ptr;
    }
    void free(void *ptr)
    {
        // std::cout << "free" << std::endl;
        // printf("free %p\n",ptr);
        zhao::tcfree(ptr);
    }

    unsigned int sleep(unsigned int seconds)
    {
        if(!zhao::t_hook_enable)
        {
            return __real_sleep(seconds);
        }
        zhao::Scheduler* scheduler = zhao::Scheduler::getThis();
        zhao::Fiber::ptr fiber = zhao::Fiber::getThis()->shared_from_this();
        scheduler->addDelayWork(seconds * 1000, std::bind(
            static_cast<void(zhao::Scheduler::*)(zhao::Fiber::ptr, zhao::tid_t)>(&zhao::Scheduler::schedule)
            ,scheduler, fiber, -1));
        fiber->yieldToHold();
    }

    int usleep(useconds_t usec)
    {
        if(!zhao::t_hook_enable)
        {
            return __real_usleep(usec);
        }
        zhao::Scheduler* scheduler = zhao::Scheduler::getThis();
        zhao::Fiber::ptr fiber = zhao::Fiber::getThis()->shared_from_this();
        scheduler->addDelayWork(usec / 1000, std::bind(
            static_cast<void(zhao::Scheduler::*)(zhao::Fiber::ptr, zhao::tid_t)>(&zhao::Scheduler::schedule)
            ,scheduler, fiber, -1));
        fiber->yieldToHold();
    }
    // socket相关
    int socket(int domain, int type, int protocol) {
        return 0;
    }

    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
        return 0;
    }

    int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
        return 0;  
    }

    // read相关
    ssize_t read(int fd, void *buf, size_t count) {
        return 0;
    }

    ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
        return 0;
    }

    ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
        return 0;
    }

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
        return 0;
    }

    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
        return 0;
    }

    // write相关 
    ssize_t write(int fd, const void *buf, size_t count) {
        return 0;
    }

    ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
        return 0;
    }

    ssize_t send(int s, const void *msg, size_t len, int flags) {
        return 0;
    }

    ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
        return 0;
    }

    ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
        return 0;
    }

    // 其他系统调用
    int close(int fd) {
        return 0;
    }

    int fcntl(int fd, int cmd, ... /* arg */) {
        return 0;
    }

    int ioctl(int d, unsigned long int request, ...) {
        return 0;
    }

    int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
        return 0;
    }

    int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
        return 0;
    }

}