#include "scheduler/iomanager.hpp"
#include "log/logger.hpp"
#include "scheduler/scheduler.hpp"
#include <assert.h>
#define MODULE_NAME "scheduler"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))
namespace zhao
{
    //static Scheduler *s_scheduler;
    IOManager::IOManager(Scheduler* scheduler)
    {
        assert(scheduler);
        m_scheduler = scheduler;
        m_epfd = epoll_create(5000); // 创建epoll树
        assert(m_epfd > 0);
        int rt = pipe(m_tickleFds); // 创建pipe fd
        assert(!rt);
        // 设置事件
        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = m_tickleFds[0];
        // 设置为非阻塞
        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        assert(!rt);
        // 挂到树上监听
        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        assert(!rt);

        contextResize(32); // 提前创建fd，优化效率(用的时候直接可以取到fdContext)
        m_thread = Thread::ptr(new Thread(std::bind(&IOManager::run, this), "iomanager"));
    }

    IOManager::~IOManager()
    {
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);

        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (m_fdContexts[i])
            {
                delete m_fdContexts[i];
            }
        }
        m_thread->join();
    }

    void IOManager::run()
    {
        t_scheduler = m_scheduler;
        const uint64_t MAX_EVNETS = 256;
        epoll_event *events = new epoll_event[MAX_EVNETS](); // epoll_wait接收事件的缓冲区
        std::shared_ptr<epoll_event> shared_events(events, [](epoll_event *ptr)
                                                   { delete[] ptr; }); // 把events封装成share_ptr
        int ret = 0;
        uint64_t next_timeout = 0;
        while (true)
        {
            while (true)
            {
                if (t_scheduler->m_auto_stop)
                { // 如果手动停止了，退出
                    goto out;
                }
                next_timeout = t_scheduler->getEarliestNext();
                ret = epoll_wait(m_epfd, events, MAX_EVNETS, (int)next_timeout); // 等
                if (ret < 0 && errno == EINTR)
                {
                }
                else
                {
                    break;
                }
            }
            std::vector<std::function<void()>> cbs;
            t_scheduler->listExpiredWork(cbs); // 获取当前时间，然后遍历timer队列，找到所有超时timer，并把超时timer的回调加入到cbs中
            if (!cbs.empty())
            { // 如果有，全部加入到调度器进行调度
                // SYLAR_LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
                t_scheduler->schedule(cbs.begin(), cbs.end());
                cbs.clear();
            }
            for (int i = 0; i < ret; ++i)
            { // 遍历所有epoll_wait接收到的事件
                epoll_event &event = events[i];
                if (event.data.fd == m_tickleFds[0])
                { // 如果是管道事件，那就是有个worker被删除了
                    t_scheduler->forEachWorker(m_scheduler->getWorkerPool(), [](Worker *w)
                                               {
                if(w->getState() == Worker::Term)
                {
                    dbg << "delete term worker:" << w->toString();
                    Scheduler::getThis()->deleteWorker(w);
                } });
                    uint8_t dummy[256]; // 没啥有价值的数据可读的，就是个标记，表示向调度器加入了cb或者fiber
                    while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0)
                        ;
                    continue;
                }
                // 如果不是管道事件，是其他事件，那就是通过addEvent向epoll树添加的事件
                FdContext *fd_ctx = (FdContext *)event.data.ptr;
                FdContext::MutexType::MutexLockGuardType lock(fd_ctx->mutex);
                if (event.events & (EPOLLERR | EPOLLHUP))
                { // 有错误，重新添加
                    event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
                }
                int real_events = NONE;
                if (event.events & EPOLLIN)
                {
                    real_events |= READ;
                }
                if (event.events & EPOLLOUT)
                {
                    real_events |= WRITE;
                }

                if ((fd_ctx->events & real_events) == NONE)
                {
                    continue;
                }
                // 获取剩余尚未触发的事件，并修改epoll树
                int left_events = (fd_ctx->events & ~real_events);
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_events;

                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
                if (rt2)
                {
                    error << "epoll_ctl(" << m_epfd << ", "
                          << op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                          << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                    continue;
                }

                // 触发该事件
                if (real_events & READ)
                {
                    fd_ctx->triggerEvent(READ);
                    --m_pendingEventCount;
                }
                if (real_events & WRITE)
                {
                    fd_ctx->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }
        }
    out:

        return;
    }
    void IOManager::contextResize(size_t size)
    {
        m_fdContexts.resize(size);

        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (!m_fdContexts[i])
            {
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }
    }

    IOManager::FdContext::EventContext &IOManager::FdContext::getContext(IOManager::Event event)
    {
        switch (event)
        {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            assert(true);
        }
        throw std::invalid_argument("getContext invalid event");
    }

    void IOManager::FdContext::resetContext(EventContext &ctx)
    {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }

    void IOManager::FdContext::triggerEvent(IOManager::Event event)
    {

        assert(events & event);

        events = (Event)(events & ~event); // 清除该事件
        EventContext &ctx = getContext(event);
        // 把回调或者协程加入调度器进行调度
        if (ctx.cb)
        {
            ctx.scheduler->schedule(&ctx.cb);
        }
        else
        {
            ctx.scheduler->schedule(&ctx.fiber);
        }
        ctx.scheduler = nullptr;
        return;
    }
    int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        FdContext *fd_ctx = nullptr;
        // 有fd直接获取fdContext 没有就先contextResize，然后获取
        RWMutex::ReadLockGuardType lock(m_mutex);
        if ((int)m_fdContexts.size() > fd)
        {
            fd_ctx = m_fdContexts[fd];
            lock.unlock();
        }
        else
        {
            lock.unlock();
            RWMutex::WriteLockGuardType lock(m_mutex);
            contextResize(fd * 1.5);
            fd_ctx = m_fdContexts[fd];
        }

        FdContext::MutexType::MutexLockGuardType lock2(fd_ctx->mutex);
        if (fd_ctx->events & event)
        {
            error << "addEvent assert fd=" << fd
                  << " event=" << (EPOLL_EVENTS)event
                  << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
            assert(!(fd_ctx->events & event));
        }
        // 这个fd之前有没有注册过事件？如果注册了，那么修改事件，如果没注册过，那就向epoll树添加一个fd
        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event epevent;
        epevent.events = EPOLLET | fd_ctx->events | event; // 把当前添加的事件和之前注册过的事件都算上
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent); // 添加或者修改epoll树
        if (rt)
        {
            error << "epoll_ctl(" << m_epfd << ", "
                  << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                  << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                  << (EPOLL_EVENTS)fd_ctx->events;
            return -1;
        }

        ++m_pendingEventCount;
        // 修改相应的fdContext
        fd_ctx->events = (Event)(fd_ctx->events | event);
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
        assert(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

        event_ctx.scheduler = Scheduler::getThis(); // 获取调度器
        if (cb)
        {
            event_ctx.cb.swap(cb); // 如果参数中写了回调，那么事件触发时就执行回调
        }
        else
        {
            event_ctx.fiber=Fiber::getThis()->shared_from_this(); // 如果添加的事件没有回调，那么当事件触发时执行当前协程
            assert(event_ctx.fiber->getState() == Fiber::RUN);
        }
        return 0;
    }
    // 只删除，不触发
    bool IOManager::delEvent(int fd, Event event)
    {
        RWMutex::ReadLockGuardType lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::MutexLockGuardType lock2(fd_ctx->mutex);
        if (!(fd_ctx->events & event))
        {
            return false;
        }

        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            error << "epoll_ctl(" << m_epfd << ", "
                  << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        --m_pendingEventCount;
        fd_ctx->events = new_events;
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);
        return true;
    }

    bool IOManager::cancelEvent(int fd, Event event)
    {
        RWMutex::ReadLockGuardType lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd]; // 获取fdContext
        lock.unlock();

        FdContext::MutexType::MutexLockGuardType lock2(fd_ctx->mutex);
        if (!(fd_ctx->events & event))
        {
            return false;
        }

        // 修改或者清除epoll树
        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            error << "epoll_ctl(" << m_epfd << ", "
                  << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }
        // 触发该事件，这算是手动触发
        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;
        return true;
    }
    // 取消该fd上注册的所有事件
    bool IOManager::cancelAll(int fd)
    {
        RWMutex::ReadLockGuardType lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::MutexLockGuardType lock2(fd_ctx->mutex);
        if (!fd_ctx->events)
        {
            return false;
        }

        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            error << "epoll_ctl(" << m_epfd << ", "
                  << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        if (fd_ctx->events & READ)
        {
            fd_ctx->triggerEvent(READ);
            --m_pendingEventCount;
            
        }
        if (fd_ctx->events & WRITE)
        {
            fd_ctx->triggerEvent(WRITE);
            --m_pendingEventCount;
        }

        assert(fd_ctx->events == 0);
        return true;
    }
    bool IOManager::delAll(int fd)
    {
        RWMutex::ReadLockGuardType lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::MutexLockGuardType lock2(fd_ctx->mutex);
        if (!fd_ctx->events)
        {
            return false;
        }

        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            error << "epoll_ctl(" << m_epfd << ", "
                  << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }
        if (fd_ctx->events & READ)
        {
            --m_pendingEventCount;
            FdContext::EventContext &event_ctx = fd_ctx->getContext(READ);
            fd_ctx->resetContext(event_ctx);
        }
        if (fd_ctx->events & WRITE)
        {
            --m_pendingEventCount;
            FdContext::EventContext &event_ctx = fd_ctx->getContext(WRITE);
            fd_ctx->resetContext(event_ctx);
        }
        return true;
    }
    void IOManager::tickle()
    {

        int rt = write(m_tickleFds[1], "T", 1);
        assert(rt == 1);
    }
} // namespace zhao