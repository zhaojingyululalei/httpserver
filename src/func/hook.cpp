#include "hook.hpp"
#include "mem/tcmalloc.hpp"
#include "mem/pagecache.hpp"
#include "mem/common.hpp"
#include "scheduler/scheduler.hpp"
#include "fiber/fiber.hpp"
#include "utils/utils.hpp"
#include "log/logger.hpp"
#include "fdmanager.hpp"
#include <iostream>
#include <functional>
#include <dlfcn.h>
#include <stdarg.h>
#define HOOK_FUN(XX)   \
    XX(malloc)         \
    XX(free)           \
    XX(realloc)        \
    XX(sleep)          \
    XX(usleep)         \
    /* socket相关 */   \
    XX(socket)         \
    XX(connect)        \
    XX(accept)         \
    /* read相关 */     \
    XX(read)           \
    XX(readv)          \
    XX(recv)           \
    XX(recvfrom)       \
    XX(recvmsg)        \
    /* write相关 */    \
    XX(write)          \
    XX(writev)         \
    XX(send)           \
    XX(sendto)         \
    XX(sendmsg)        \
    /* 其他系统调用 */ \
    XX(close)          \
    XX(fcntl)          \
    XX(ioctl)          \
    XX(getsockopt)     \
    XX(setsockopt)
#define MODULE_NAME "hook"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))
namespace zhao
{
    static thread_local bool t_hook_enable = false;
    static auto connect_timeout = zhao::ConfigMgr::getInstance()->add<uint64_t>("tcp.connect.timeout", 5000, "连接超时时间");
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
struct timer_info
{
    int cancelled = 0;
    void reset(void)
    {
        cancelled = 0;
    }
};
template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                     uint32_t event, int timeout_so, Args &&...args)
{
    if (!zhao::t_hook_enable)
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    zhao::FileDescription::ptr ctx = zhao::FdMgr::getInstance()->get(fd);
    if (!ctx)
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    if (ctx->isClose())
    {
        errno = EBADF;
        return -1;
    }
    // 用户自己设置了非阻塞，那么就直接调用原始函数
    if (!ctx->isSocket() || ctx->getUsrNoneBlock())
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info); // 不会被销毁，因为后面yiled了，保存了栈帧

retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...); // 用原始函数去读
    while (n == -1 && errno == EINTR)
    { // 只要是intr打断的，就一直读
        n = fun(fd, std::forward<Args>(args)...);
    }
    if (n == -1 && errno == EAGAIN)
    { // 目前暂时确实是没有数据，就交给io调度器
        zhao::Scheduler *sche = zhao::Scheduler::getThis();
        zhao::DelayWork::ptr delay_work;
        std::weak_ptr<timer_info> winfo(tinfo);
        /// 该系统调用如果设置了超时时间
        if (to != (uint64_t)-1)
        {
            delay_work = sche->addConditionDelayWork(to, [winfo, fd, sche, event]()
                                                     {
                /**调度到这里，该函数执行，说明该系统调用已经超时了，就是没有数据，就标记为cancelled,并且从epoll树中取消该事件 */
                auto t = winfo.lock();
                if(!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                //清除事件，并且强行trigger唤醒fiber
                sche->cancelEvent(fd, (zhao::IOManager::Event)(event)); }, winfo); // 条件恒成立，因为winfo是>0的值，是一个结构体的地址.
        }
        // 上面的代码都是瞬时完成的，只不过注册了一个timer回调而已，注册完立即执行到这里
        
        int rt = sche->addEvent(fd, (zhao::IOManager::Event)(event)); // 不同的系统调用执行不同的事件，这里也只是个注册操作，只不过是注册到epoll树
        if (rt)
        { // 没有注册成功，把前面的定时器取消掉，因为事件都没注册成功，更不可能存在超时返回一说
            dbg << hook_fun_name << " addEvent("
            << fd << ", " << event << ")";
            if (delay_work)
            {
                delay_work->cancel(); // 取消前面的定时器
            }
            return -1;
        }
        else
        {
            zhao::Fiber::ptr cur_fiber = zhao::Fiber::getThis()->shared_from_this();
            cur_fiber->yieldToHold();// 注册成功后，让出调度
            /**
             * 1.可能从sche->addConditionTimer唤醒，表示该事件超时，始终没有数据
             * 2.可能从sche->addEvent唤醒，表示该事件被触发了，有数据可读
             * */
            if (delay_work)
            {
                delay_work->cancel(); // 无论从哪里唤醒，取消定时器,已经不会超时了
            }
            if (tinfo->cancelled)
            { // 这个肯定是从sche->addConditionTimer唤醒的，errno标记为超时，并返回-1表示超过一定时间也没等到数据
                errno = tinfo->cancelled;
                return -1;
            }
            // 刚才读的时候没有数据嘛，一个字没读出来，现在epoll唤醒了，重新去读，肯定有数据
            goto retry; // 唤醒后，重新尝试读取数据。只有从addEvent唤醒的，才会执行到这里；从addConditionTimer唤醒的，在前三行就返回-1了
        }
    }

    return n;
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
        if (!zhao::t_hook_enable)
        {
            return __real_sleep(seconds);
        }
        zhao::Scheduler *scheduler = zhao::Scheduler::getThis();
        zhao::Fiber::ptr fiber = zhao::Fiber::getThis()->shared_from_this();
        scheduler->addDelayWork(seconds * 1000, std::bind(
                                                    static_cast<void (zhao::Scheduler::*)(zhao::Fiber::ptr, zhao::tid_t)>(&zhao::Scheduler::schedule), scheduler, fiber, -1));
        fiber->yieldToHold();
    }

    int usleep(useconds_t usec)
    {
        if (!zhao::t_hook_enable)
        {
            return __real_usleep(usec);
        }
        zhao::Scheduler *scheduler = zhao::Scheduler::getThis();
        zhao::Fiber::ptr fiber = zhao::Fiber::getThis()->shared_from_this();
        scheduler->addDelayWork(usec / 1000, std::bind(
                                                 static_cast<void (zhao::Scheduler::*)(zhao::Fiber::ptr, zhao::tid_t)>(&zhao::Scheduler::schedule), scheduler, fiber, -1));
        fiber->yieldToHold();
    }
    // socket相关
    int socket(int domain, int type, int protocol)
    {
        if (!zhao::t_hook_enable)
        {
            return __real_socket(domain, type, protocol);
        }
        int fd = __real_socket(domain, type, protocol);
        if (fd == -1)
        {
            return fd;
        }
        zhao::FdMgr::getInstance()->get(fd);
        return fd;
    }
    /**
     * @brief connect函数，不像read write，可以通过setsockopt设置超时时间
     */
    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
    {
        if (!zhao::t_hook_enable)
        {
            return __real_connect(sockfd, addr, addrlen);
        }

        zhao::FileDescription::ptr ctx = zhao::FdMgr::getInstance()->get(sockfd);
        if (!ctx)
        {
            return __real_connect(sockfd, addr, addrlen);
        }

        if (ctx->isClose())
        {
            errno = EBADF;
            return -1;
        }
        // 用户自己设置了非阻塞，那么就直接调用原始函数
        if (!ctx->isSocket() || ctx->getUsrNoneBlock())
        {
            return __real_connect(sockfd, addr, addrlen);
        }

        int n = __real_connect(sockfd, addr, addrlen);
        if (n == 0) // 直接连接成功
        {
            return 0;
        }
        else if (n == -1 && errno == EINPROGRESS)
        {
            // 连接正在进行中
            uint64_t timeout_ms = zhao::connect_timeout->getValue();
            zhao::Scheduler *sche = zhao::Scheduler::getThis();
            zhao::DelayWork::ptr delay_work;
            std::shared_ptr<timer_info> tinfo(new timer_info);
            std::weak_ptr<timer_info> winfo(tinfo);

            if (timeout_ms != (uint64_t)-1)
            {
                delay_work = sche->addConditionDelayWork(timeout_ms, [winfo, sockfd, sche]()
                                                         {
                auto t = winfo.lock();
                if(!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                sche->cancelEvent(sockfd, (zhao::IOManager::WRITE)); }, winfo);
            }
            
            zhao::Fiber::ptr cur_fiber = zhao::Fiber::getThis()->shared_from_this();
            int rt = sche->addEvent(sockfd, zhao::IOManager::WRITE);
            if (rt == 0)
            {
                cur_fiber->yieldToHold();
                if (delay_work)
                {
                    delay_work->cancel();
                }
                if (tinfo->cancelled)
                {
                    errno = tinfo->cancelled;
                    return -1;
                }
            }
            else
            {
                if (delay_work)
                {
                    delay_work->cancel();
                }
                error << "connect addEvent(" << sockfd << ", WRITE) error";
            }
        }
        else
        {
            return n; // 有错误,是函数调用错误，参数之类的有毛病，不是连接方面的错误
        }
        /**
         * addr和len已经交给内核了，只要成功连接，内核会自动赋值，无需重新connect
         * epoll唤醒后，也不知到是连接成功还是失败了，只能通过getsockopt来获取结果 */
        int error_ = 0;
        socklen_t len = sizeof(int);
        if (-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error_, &len))
        {
            return -1;
        }
        if (!error_)
        {
            return 0;
        }
        else
        {
            errno = error_;
            return -1;
        }
       
    }

    int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
    {
        int fd = do_io(s, __real_accept, "accept", zhao::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
        if (fd > 0)
        {
            zhao::FdMgr::getInstance()->get(fd);
        }
        return fd;
    }

    // read相关
    ssize_t read(int fd, void *buf, size_t count)
    {
        return do_io(fd, __real_read, "read", zhao::IOManager::READ, SO_RCVTIMEO, buf, count);
    }

    ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
    {
        return do_io(fd, __real_readv, "readv", zhao::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
    }

    ssize_t recv(int sockfd, void *buf, size_t len, int flags)
    {
        return do_io(sockfd, __real_recv, "recv", zhao::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
    }

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
    {
        return do_io(sockfd, __real_recvfrom, "recvfrom", zhao::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
    }

    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
    {
        return do_io(sockfd, __real_recvmsg, "recvmsg", zhao::IOManager::READ, SO_RCVTIMEO, msg, flags);
    }

    // write相关
    ssize_t write(int fd, const void *buf, size_t count)
    {
        return do_io(fd, __real_write, "write", zhao::IOManager::WRITE, SO_SNDTIMEO, buf, count);
    }

    ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
    {
        return do_io(fd, __real_writev, "writev", zhao::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
    }

    ssize_t send(int s, const void *msg, size_t len, int flags)
    {
        return do_io(s, __real_send, "send", zhao::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
    }

    ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
    {
        return do_io(s, __real_sendto, "sendto", zhao::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
    }

    ssize_t sendmsg(int s, const struct msghdr *msg, int flags)
    {
        return do_io(s, __real_sendmsg, "sendmsg", zhao::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
    }

    // 其他系统调用
    int close(int fd)
    {
        if (!zhao::t_hook_enable)
        {
            return __real_close(fd);
        }

        zhao::FileDescription::ptr ctx = zhao::FdMgr::getInstance()->get(fd);
        if (ctx)
        {
            auto sche = zhao::Scheduler::getThis();
            if (sche)
            {
                //有的fd可能会阻塞在read或者accept等函数等待数据，调用cancleAll唤醒，不能用delAll
                //cancle会间接调用triggerEvent使得等待数据的fiber再次运行，而delAll会直接删除该fd的上下文，那个fiber就再也不会运行了
                sche->cancelAll(fd); 
            }
            zhao::FdMgr::getInstance()->del(fd);
        }
        return __real_close(fd);
    }

    /**
     * @brief 用户设置了非阻塞，才算非阻塞。系统自动设置的非阻塞不算
     */
    int fcntl(int fd, int cmd, ... /* arg */)
    {
        va_list va;
        va_start(va, cmd);
        switch (cmd)
        {
        case F_SETFL:
        {
            int arg = va_arg(va, int);
            va_end(va);
            zhao::FileDescription::ptr ctx = zhao::FdMgr::getInstance()->get(fd); // 这里系统已经自动设置了非阻塞
            if (!ctx || ctx->isClose() || !ctx->isSocket())
            { // 不是socket不用管
                return __real_fcntl(fd, cmd, arg);
            }
            if (arg & O_NONBLOCK) // 如果用户也要设置非阻塞，做个标记即可
            {
                ctx->setUsrNoneBlock(true); // 用户设置了nonblock
            }

            arg &= ~O_NONBLOCK; // 设置剩余配置
            return __real_fcntl(fd, cmd, arg);
        }
        break;
        case F_GETFL:
        {
            va_end(va);
            int arg = __real_fcntl(fd, cmd);
            zhao::FileDescription::ptr ctx = zhao::FdMgr::getInstance()->get(fd);
            if (!ctx || ctx->isClose() || !ctx->isSocket())
            {
                return arg;
            }

            if (ctx->getUsrNoneBlock())
            {
                return arg | O_NONBLOCK;
            }
            else
            {
                // 欺骗操作，实际上所有的socket_fd都在创建FdCtx时已经设置为非阻塞了
                return arg & ~O_NONBLOCK;
            }
        }
        break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
        {
            int arg = va_arg(va, int);
            va_end(va);
            return __real_fcntl(fd, cmd, arg);
        }
        break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
        {
            va_end(va);
            return __real_fcntl(fd, cmd);
        }
        break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
        {
            struct flock *arg = va_arg(va, struct flock *);
            va_end(va);
            return __real_fcntl(fd, cmd, arg);
        }
        break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
        {
            struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
            va_end(va);
            return __real_fcntl(fd, cmd, arg);
        }
        break;
        default:
            va_end(va);
            return __real_fcntl(fd, cmd);
        }
    }

    int ioctl(int d, unsigned long int request, ...)
    {
        va_list va;
        va_start(va, request);
        void *arg = va_arg(va, void *);
        va_end(va);

        if (FIONBIO == request)
        {
            bool user_nonblock = *(int *)arg == 0 ? false : true;
            zhao::FileDescription::ptr ctx = zhao::FdMgr::getInstance()->get(d);
            if (!ctx || ctx->isClose() || !ctx->isSocket())
            {
                return __real_ioctl(d, request, arg);
            }
            ctx->setUsrNoneBlock(user_nonblock);
        }
        return __real_ioctl(d, request, arg);
    }

    int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
    {
        return __real_getsockopt(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
    {
        if (!zhao::t_hook_enable)
        {
            return __real_setsockopt(sockfd, level, optname, optval, optlen);
        }
        if (level == SOL_SOCKET)
        {
            if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
            {
                zhao::FileDescription::ptr ctx = zhao::FdMgr::getInstance()->get(sockfd);
                if (ctx)
                {
                    const timeval *v = (const timeval *)optval;
                    ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
                }
            }
        }
        return __real_setsockopt(sockfd, level, optname, optval, optlen);
    }
}