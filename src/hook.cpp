/**
 * 使用原生函数，阻塞时，整个线程退出调度，导致该线程无法调度其他协程。
 * 该cpp文件主要做socket原生函数的hook工作
 * 调用函数阻塞时，使得当前协程退出调度，进而调度其他协程。当数据到来或者超时时，让对应协程唤醒，加入调度器继续执行。
 */

#include <dlfcn.h>
#include "hook.h"
#include "log.h"
#include "ioscheduler.h"
#include "fdmanager.h"
#include "config.h"
static zhao::Logger::Ptr g_logger = GET_LOGGER("system");
#define ASSERT(x) ZHAO_LOG_ASSERT(x)
#define dbg ZHAO_LOG_DEBUG(g_logger)

namespace zhao
{
    ConfigItem<int>::Ptr g_tcp_connect_timeout = Config::lookup("tcp.connect.timeout", 5000, "tcp connect timeout");
    static uint64_t s_connect_timeout = -1;
    static thread_local bool st_ishook = false;
#define HOOK_FUNC(XXX) \
    XXX(sleep)         \
    XXX(usleep)        \
    XXX(nanosleep)     \
    XXX(socket)        \
    XXX(connect)       \
    XXX(accept)        \
    XXX(read)          \
    XXX(readv)         \
    XXX(recv)          \
    XXX(recvfrom)      \
    XXX(recvmsg)       \
    XXX(write)         \
    XXX(writev)        \
    XXX(send)          \
    XXX(sendto)        \
    XXX(sendmsg)       \
    XXX(close)         \
    XXX(fcntl)         \
    XXX(ioctl)         \
    XXX(getsockopt)    \
    XXX(setsockopt)

    void hookInit()
    {
        static bool inited = false;
        if (inited)
            return;
        // 将原始的sleep等函数地址存放到sleep_f等符号中
        /*
            使用 dlsym(RTLD_NEXT, "sleep") 会从当前模块之后的共享库中查找符号，
            这意味着它会跳过当前模块中的 sleep 符号（即自己定义的钩子函数），
            找到下一个共享库中的真实实现（通常是 libc 提供的 sleep 函数）。
            此时sleep_f存放着libc中的sleep函数的地址
        */
#define XXX(name) name##_f = (name##_func)dlsym(RTLD_NEXT, #name);
        HOOK_FUNC(XXX)
#undef XXX
    }

    struct HookIniter
    {
        HookIniter()
        {
            hookInit();
            s_connect_timeout = g_tcp_connect_timeout->getvalue();

            g_tcp_connect_timeout->addOnChangeCallback([](const int &old_value, const int &new_value)
                                               {
                dbg << "tcp connect timeout changed from "
                                         << old_value << " to " << new_value;
                s_connect_timeout = new_value; });
        }
    };
    static HookIniter s_hook_initer;
    bool isHook()
    {
        return st_ishook;
    }
    void setHookEnable(bool enable)
    {
        st_ishook = enable;
    }
} // namespace zhao

struct timer_info
{
    int cancelled = 0;
};

/**
 * 如果系统调用被用户设置成了非阻塞，那么就直接调用原始函数
 * 如果用户就是想用阻塞的，那么执行该函数的协程会退出调度器，其他协程会继续执行。当该函数收到数据或者等待超时，该协程会被重新调度。 */
template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                     uint32_t event, int timeout_so, Args &&...args)
{
    if (!zhao::st_ishook)
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    zhao::FdCtx::ptr ctx = zhao::FdMgr::getInstance()->get(fd);
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
    if (!ctx->isSocket() || ctx->getUserNonblock())
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...); // 用原始函数去读
    while (n == -1 && errno == EINTR)
    { // 只要是intr打断的，就一直读
        n = fun(fd, std::forward<Args>(args)...);
    }
    if (n == -1 && errno == EAGAIN)
    { // 目前暂时确实是没有数据，就交给io调度器
        zhao::IOScheduler *iom = zhao::IOScheduler::getThis();
        zhao::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);
        /// 该系统调用如果设置了超时时间
        if (to != (uint64_t)-1)
        {
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event]()
                                           {
                /**调度到这里，该函数执行，说明该系统调用已经超时了，就是没有数据，就标记为cancelled,并且从epoll树中取消该事件 */
                auto t = winfo.lock();
                if(!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                //清除事件，并且强行trigger唤醒fiber
                iom->cancelEvent(fd, (zhao::IOScheduler::EventType)(event)); }, winfo); // 条件恒成立，因为winfo是>0的值，是一个结构体的地址.
        }
        // 上面的代码都是瞬时完成的，只不过注册了一个timer回调而已，注册完立即执行到这里
        int rt = iom->addEvent(fd, (zhao::IOScheduler::EventType)(event)); // 不同的系统调用执行不同的事件，这里也只是个注册操作，只不过是注册到epoll树
        if (rt)
        { // 没有注册成功，把前面的定时器取消掉，因为事件都没注册成功，更不可能存在超时返回一说
            dbg << hook_fun_name << " addEvent("
                << fd << ", " << event << ")";
            if (timer)
            {
                timer->cancel(); // 取消前面的定时器
            }
            return -1;
        }
        else
        {
            zhao::Fiber::yieldToHold(); // 注册成功后，让出调度
            /**
             * 1.可能从iom->addConditionTimer唤醒，表示该事件超时，始终没有数据
             * 2.可能从iom->addEvent唤醒，表示该事件被触发了，有数据可读
             * */
            if (timer)
            {
                timer->cancel(); // 无论从哪里唤醒，取消定时器
            }
            if (tinfo->cancelled)
            { // 这个肯定是从iom->addConditionTimer唤醒的，errno标记为超时，并返回-1表示超过一定时间也没等到数据
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry; // 唤醒后，重新尝试读取数据。只有从addEvent唤醒的，才会执行到这里；从addConditionTimer唤醒的，在前三行就返回-1了
        }
    }

    return n;
}
extern "C"
{
#define XXX(name) name##_func name##_f = nullptr;
    HOOK_FUNC(XXX)
#undef XXX
    /**
     * 在 Linux 下，允许用户重写标准库函数（如 malloc, sleep, read 等）。
     * 当在程序中定义了一个全局函数 sleep，动态链接器在解析符号时，会优先使用你定义的版本，而不是 glibc 提供的。
     */
    unsigned int sleep(unsigned int seconds)
    {
        // 如果没有hook，就调用原始函数
        if (!zhao::isHook())
        {
            return sleep_f(seconds);
        }
        zhao::Fiber::Ptr cur_fiber = zhao::Fiber::getThis();
        zhao::IOScheduler *iosche = zhao::IOScheduler::getThis();
        // 否则，使用定时器调度，在seconds秒后，重新将该协程加入调度器
        iosche->addTimer(
            seconds * 1000,
            std::bind((void (zhao::FiberScheduler::*)(zhao::Fiber::Ptr, int))&zhao::FiberScheduler::join, iosche, cur_fiber, -1));
        zhao::Fiber::yieldToHold(); // 暂时退出调度器，不再执行当前协程，后续定时器时间到，放入调度器调度执行
    }
    int usleep(useconds_t usec)
    {
        // 如果没有启用 hook，直接调用原始的 usleep
        if (!zhao::isHook())
        {
            return usleep_f(usec);
        }

        // 获取当前协程和调度器
        zhao::Fiber::Ptr cur_fiber = zhao::Fiber::getThis();
        zhao::IOScheduler *iosche = zhao::IOScheduler::getThis();

        // 使用定时器，在 usec 微秒后将协程重新加入调度器
        iosche->addTimer(
            usec / 1000, // 转换为毫秒
            std::bind((void (zhao::FiberScheduler::*)(zhao::Fiber::Ptr, int))&zhao::FiberScheduler::join,
                      iosche, cur_fiber, -1));

        // 当前协程让出执行权
        zhao::Fiber::yieldToHold();

        return 0; // usleep 返回 0 表示成功
    }
    int nanosleep(const struct timespec *req, struct timespec *rem)
    {
        if (!zhao::isHook())
        {
            return nanosleep_f(req, rem);
        }
        int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
        // 获取当前协程和调度器
        zhao::Fiber::Ptr cur_fiber = zhao::Fiber::getThis();
        zhao::IOScheduler *iosche = zhao::IOScheduler::getThis();

        // 使用定时器，在 usec 微秒后将协程重新加入调度器
        iosche->addTimer(
            timeout_ms, // 转换为毫秒
            std::bind((void (zhao::FiberScheduler::*)(zhao::Fiber::Ptr, int))&zhao::FiberScheduler::join,
                      iosche, cur_fiber, -1));

        // 当前协程让出执行权
        zhao::Fiber::yieldToHold();

        return 0; // usleep 返回 0 表示成功
    }
    // ========== socket 函数 ==========
    int socket(int domain, int type, int protocol)
    {
        if (!zhao::isHook())
        {
            return socket_f(domain, type, protocol);
        }
        int fd = socket_f(domain, type, protocol);
        if (fd == -1)
        {
            return fd;
        }
        zhao::FdMgr::getInstance()->get(fd, true); // 创建FdCtx,给fd设置成非阻塞，设置超时时间等。
        return fd;
    }
    int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms)
    {
        if (!zhao::st_ishook)
        {
            return connect_f(fd, addr, addrlen);
        }
        zhao::FdCtx::ptr ctx = zhao::FdMgr::getInstance()->get(fd);
        if (!ctx || ctx->isClose())
        {
            errno = EBADF;
            return -1;
        }

        if (!ctx->isSocket())
        {
            return connect_f(fd, addr, addrlen);
        }

        if (ctx->getUserNonblock()) // 用户设置了非阻塞，直接调用原函数
        {
            return connect_f(fd, addr, addrlen);
        }

        int n = connect_f(fd, addr, addrlen);
        if (n == 0)
        {
            return 0;
        }
        else if (n != -1 || errno != EINPROGRESS) // 立马能够得到返回值，直接返回
        {
            return n;
        }
        // 暂时得不到返回值，让出调度，并且交给定时器处理超时
        zhao::IOScheduler *iom = zhao::IOScheduler::getThis();
        zhao::Timer::ptr timer;
        std::shared_ptr<timer_info> tinfo(new timer_info);
        std::weak_ptr<timer_info> winfo(tinfo);

        if (timeout_ms != (uint64_t)-1)
        {
            timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]()
                                           {
                auto t = winfo.lock();
                if(!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, zhao::IOScheduler::WRITE); }, winfo); // 清除事件，并且强行trigger唤醒fiber
        }

        int rt = iom->addEvent(fd, zhao::IOScheduler::WRITE);
        if (rt == 0) // 添加成功
        {
            zhao::Fiber::yieldToHold();
            // 从两个地方唤醒，iom->addConditionTimer   iom->addEvent
            if (timer)
            {
                timer->cancel();
            }
            if (tinfo->cancelled) // 从iom->addConditionTimer唤醒的，会执行这个if
            {
                errno = tinfo->cancelled;
                return -1;
            }
        }
        else // 添加失败
        {
            if (timer)
            {
                timer->cancel();
            }
            dbg << "connect addEvent(" << fd << ", WRITE) error";
        }

        int error = 0;
        socklen_t len = sizeof(int);
        if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len))
        {
            return -1;
        }
        if (!error)
        {
            return 0; // 链接成功
        }
        else
        {
            errno = error;
            return -1;
        }
    }
    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
    {
        return connect_with_timeout(sockfd, addr, addrlen, zhao::s_connect_timeout);
    }

    int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
    {
        int fd = do_io(s, accept_f, "accept", zhao::IOScheduler::READ, SO_RCVTIMEO, addr, addrlen);
        if (fd >= 0)
        {
            zhao::FdMgr::getInstance()->get(fd, true);
        }
        return fd;
    }
    // ========== read write 函数 ==========
    ssize_t read(int fd, void *buf, size_t count)
    {
        return do_io(fd, read_f, "read", zhao::IOScheduler::READ, SO_RCVTIMEO, buf, count);
    }

    ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
    {
        return do_io(fd, readv_f, "readv", zhao::IOScheduler::READ, SO_RCVTIMEO, iov, iovcnt);
    }

    ssize_t recv(int sockfd, void *buf, size_t len, int flags)
    {
        return do_io(sockfd, recv_f, "recv", zhao::IOScheduler::READ, SO_RCVTIMEO, buf, len, flags);
    }

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
    {
        return do_io(sockfd, recvfrom_f, "recvfrom", zhao::IOScheduler::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
    }

    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
    {
        return do_io(sockfd, recvmsg_f, "recvmsg", zhao::IOScheduler::READ, SO_RCVTIMEO, msg, flags);
    }

    ssize_t write(int fd, const void *buf, size_t count)
    {
        return do_io(fd, write_f, "write", zhao::IOScheduler::WRITE, SO_SNDTIMEO, buf, count);
    }

    ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
    {
        return do_io(fd, writev_f, "writev", zhao::IOScheduler::WRITE, SO_SNDTIMEO, iov, iovcnt);
    }

    ssize_t send(int s, const void *msg, size_t len, int flags)
    {
        return do_io(s, send_f, "send", zhao::IOScheduler::WRITE, SO_SNDTIMEO, msg, len, flags);
    }

    ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
    {
        return do_io(s, sendto_f, "sendto", zhao::IOScheduler::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
    }

    ssize_t sendmsg(int s, const struct msghdr *msg, int flags)
    {
        return do_io(s, sendmsg_f, "sendmsg", zhao::IOScheduler::WRITE, SO_SNDTIMEO, msg, flags);
    }

    // ========== 其他 IO 函数 ==========
    int close(int fd)
    {
        if (!zhao::isHook())
        {
            return close_f(fd);
        }

        zhao::FdCtx::ptr ctx = zhao::FdMgr::getInstance()->get(fd);
        if (ctx)
        {
            auto iosche = zhao::IOScheduler::getThis();
            if (iosche)
            {
                iosche->cancelAll(fd);
            }
            zhao::FdMgr::getInstance()->del(fd);
        }

        return close_f(fd);
    }

    int fcntl(int fd, int cmd, ...)
    {
        va_list va;
        va_start(va, cmd);
        switch (cmd)
        {
        case F_SETFL:
        {
            int arg = va_arg(va, int);
            va_end(va);
            zhao::FdCtx::ptr ctx = zhao::FdMgr::getInstance()->get(fd);
            if (!ctx || ctx->isClose() || !ctx->isSocket())
            { // 不是socket不用管
                return fcntl_f(fd, cmd, arg);
            }
            ctx->setUserNonblock(arg & O_NONBLOCK); // 用户是否设置了nonblock,如果设置了把m_userNonblock=true;
            if (ctx->getSysNonblock())
            {
                arg |= O_NONBLOCK;
            }
            else
            {
                arg &= ~O_NONBLOCK;
            }
            return fcntl_f(fd, cmd, arg);
        }
        break;
        case F_GETFL:
        {
            va_end(va);
            int arg = fcntl_f(fd, cmd);
            zhao::FdCtx::ptr ctx = zhao::FdMgr::getInstance()->get(fd);
            if (!ctx || ctx->isClose() || !ctx->isSocket())
            {
                return arg;
            }
            // 欺骗操作，实际上所有的socket_fd都在创建FdCtx时已经设置为非阻塞了
            if (ctx->getUserNonblock())
            {
                return arg | O_NONBLOCK;
            }
            else
            {
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
            return fcntl_f(fd, cmd, arg);
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
            return fcntl_f(fd, cmd);
        }
        break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
        {
            struct flock *arg = va_arg(va, struct flock *);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
        {
            struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
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
            bool user_nonblock = !!*(int *)arg;
            zhao::FdCtx::ptr ctx = zhao::FdMgr::getInstance()->get(d);
            if (!ctx || ctx->isClose() || !ctx->isSocket())
            {
                return ioctl_f(d, request, arg);
            }
            ctx->setUserNonblock(user_nonblock); // 默认是阻塞，用户只能选择设置为非阻塞，要么就不设置。
        }
        return ioctl_f(d, request, arg);
    }

    int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
    {
        return getsockopt_f(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
    {
        if (!zhao::st_ishook)
        {
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        }
        if (level == SOL_SOCKET)
        {
            if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
            {
                zhao::FdCtx::ptr ctx = zhao::FdMgr::getInstance()->get(sockfd);
                if (ctx)
                {
                    const timeval *v = (const timeval *)optval;
                    ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
                }
            }
        }
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
}
