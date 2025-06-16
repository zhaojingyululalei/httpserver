#include "ioscheduler.h"
#include "log.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
namespace zhao
{
    static Logger::Ptr g_logger = GET_LOGGER("system");
#define dbg_error ZHAO_LOG_ERROR(g_logger)
#define dbg_info ZHAO_LOG_INFO(g_logger)
#define dbg_warn ZHAO_LOG_WARN(g_logger)
#define ASSERT(x) ZHAO_LOG_ASSERT(x)
    IOScheduler::FdEvent::EventMethod &IOScheduler::FdEvent::get(EventType type)
    {
        switch (type)
        {
        case EventType::READ:
            return read;
        case EventType::WRITE:
            return write;
        default:
            ASSERT(false);
            break;
        }
    }

    void IOScheduler::FdEvent::reset(EventMethod &em)
    {
        em.scheduler = nullptr;
        em.cb = nullptr;
        em.fiber.reset();
    }

    void IOScheduler::FdEvent::trigger(EventType type)
    {
        // 当前事件必须已经有
        ASSERT(type & events);
        // 清除当前事件标志，并进行处理
        events = (EventType)(events & ~type);
        // 处理该事件注册的回调，加入IO调度器
        EventMethod &emthod = get(type);
        ASSERT(emthod.scheduler != nullptr);
        if (emthod.cb)
        {
            emthod.scheduler->join(&emthod.cb);
        }
        else if (emthod.fiber)
        {
            emthod.scheduler->join(&emthod.fiber);
        }
        emthod.scheduler = nullptr;
    }
    IOScheduler::IOScheduler(size_t threads, bool use_caller, const std::string &name)
        : FiberScheduler(threads, use_caller, name)
    {
        // 创建epoll树
        m_epfd = epoll_create(5000);
        ASSERT(m_epfd > 0);
        // 创建管道，有新任务，通过写管道唤醒epoll_wait，继而进行调度
        int ret = pipe(m_pipefd);
        ASSERT(ret == 0);

        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = m_pipefd[0];
        // ET模式，需要循环读，一直把数据读干净。如果设置成阻塞，读干净后再读会卡死而不是返回0，会导致线程卡死，无法调度协程
        ret = fcntl(m_pipefd[0], F_SETFL, O_NONBLOCK);
        ASSERT(ret == 0);
        // 注册读管道任务，用于添加新任务时，唤醒epoll_wait，让epoll_wait协程让出调度，进而调度到新任务执行
        ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_pipefd[0], &event);
        ASSERT(ret == 0);
        fdeventResize(32);
        start();
    }

    IOScheduler::~IOScheduler()
    {
        stop();
        close(m_epfd);
        close(m_pipefd[0]);
        close(m_pipefd[1]);
        for (size_t i = 0; i < m_fdEvents.size(); i++)
        {
            delete m_fdEvents[i];
        }
    }

    int IOScheduler::addEvent(int fd, EventType type, std::function<void()> cb)
    {
        int ret;
        FdEvent *fd_evt = nullptr;
        // 获取该fd对应的事件
        RWMutex::ReadLockGuardType rdlock(m_mutex);
        if (m_fdEvents.size() > fd)
        {
            fd_evt = m_fdEvents[fd];
            rdlock.unlock();
        }
        else
        {
            rdlock.unlock();
            RWMutex::WriteLockGuardType wrlock(m_mutex);
            fdeventResize(fd * 1.5);
            ASSERT(m_fdEvents.size() > fd);
            fd_evt = m_fdEvents[fd];
        }

        Mutex::MutexLockGuardType lock(fd_evt->mutex);
        if (fd_evt->events & type)
        {
            dbg_error << "addEvent fd=" << fd << " events=" << type << " fail, already exists";
            ASSERT(false);
        }
        // 修改or添加epoll节点，修改epoll树
        int e_opt = fd_evt->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event ep_event;
        memset(&ep_event, 0, sizeof(ep_event));
        ep_event.events = EPOLLET | fd_evt->events | type;
        ep_event.data.ptr = fd_evt;
        ret = epoll_ctl(m_epfd, e_opt, fd, &ep_event);
        ASSERT(ret == 0);
        // 未决事件++
        ++m_pendingEventCount;

        // 维护std::vector<FdEvent *> m_fdEvents;列表，记录所有FdEvent
        fd_evt->events = (EventType)(fd_evt->events | type);
        FdEvent::EventMethod &emthod = fd_evt->get(type);
        ASSERT(!emthod.scheduler && !emthod.cb && !emthod.fiber);
        emthod.scheduler = FiberScheduler::getThis();
        if (cb)
        {
            emthod.cb.swap(cb);
        }
        else
        {
            emthod.fiber = Fiber::getThis();
            ASSERT(emthod.fiber->getState() == Fiber::State::EXEC);
        }

        return 0;
    }
    // 删除相应事件
    bool IOScheduler::delEvent(int fd, EventType type)
    {
        int ret;
        RWMutex::ReadLockGuardType rdlock(m_mutex);
        if (m_fdEvents.size() <= fd)
        {
            return false;
        }
        FdEvent *fd_evt = m_fdEvents[fd];
        rdlock.unlock();
        Mutex::MutexLockGuardType lock(fd_evt->mutex);
        EventType new_events = (EventType)(fd_evt->events & ~type);
        int e_opt = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event ep_event;
        memset(&ep_event, 0, sizeof(ep_event));
        ep_event.events = EPOLLET | new_events;
        ep_event.data.ptr = fd_evt;
        ret = epoll_ctl(m_epfd, e_opt, fd, &ep_event);
        ASSERT(ret == 0);
        --m_pendingEventCount;
        fd_evt->events = new_events;
        FdEvent::EventMethod &emthod = fd_evt->get(type);
        fd_evt->reset(emthod);
        return true;
    }
    // 强行删除并触发这个事件(加入调度)
    bool IOScheduler::cancelEvent(int fd, EventType type)
    {
        int ret;
        RWMutex::ReadLockGuardType rdlock(m_mutex);
        if (m_fdEvents.size() <= fd)
        {
            return false;
        }
        FdEvent *fd_evt = m_fdEvents[fd];
        rdlock.unlock();
        Mutex::MutexLockGuardType lock(fd_evt->mutex);
        // 根本没有这个事件，怎么删
        if (!(fd_evt->events & type))
        {
            return false;
        }
        EventType new_events = (EventType)(fd_evt->events & ~type);
        int e_opt = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event ep_event;
        memset(&ep_event, 0, sizeof(ep_event));
        ep_event.events = EPOLLET | new_events;
        ep_event.data.ptr = fd_evt;
        ret = epoll_ctl(m_epfd, e_opt, fd, &ep_event);
        ASSERT(ret == 0);
        fd_evt->trigger(type);
        --m_pendingEventCount;
        return true;
    }

    bool IOScheduler::cancelAll(int fd)
    {
        int ret;
        RWMutex::ReadLockGuardType rdlock(m_mutex);
        if (m_fdEvents.size() <= fd)
        {
            return false;
        }
        FdEvent *fd_evt = m_fdEvents[fd];
        rdlock.unlock();
        Mutex::MutexLockGuardType lock(fd_evt->mutex);
        if (!fd_evt->events)
        {
            return false;
        }
        int e_opt = EPOLL_CTL_DEL;
        epoll_event ep_event;
        memset(&ep_event, 0, sizeof(ep_event));
        ep_event.events = 0;
        ep_event.data.ptr = fd_evt;
        ret = epoll_ctl(m_epfd, e_opt, fd, &ep_event);
        ASSERT(ret == 0);
        if (fd_evt->events & EventType::READ)
        {
            fd_evt->trigger(EventType::READ);
            --m_pendingEventCount;
        }
        if (fd_evt->events & EventType::WRITE)
        {
            fd_evt->trigger(EventType::WRITE);
            --m_pendingEventCount;
        }
        ASSERT(fd_evt->events == 0);
        return true;
    }

    void IOScheduler::fdeventResize(size_t size)
    {
        ZHAO_LOG_ASSERT(size > m_fdEvents.size());
        m_fdEvents.resize(size);
        for (size_t i = 0; i < m_fdEvents.size(); i++)
        {
            if (!m_fdEvents[i])
            {
                m_fdEvents[i] = new FdEvent;
                m_fdEvents[i]->fd = i;
            }
        }
    }
    IOScheduler *IOScheduler::getThis()
    {
        // 父类转子类
        return dynamic_cast<IOScheduler *>(FiberScheduler::getThis());
    }

    void IOScheduler::tickle()
    {
        // 没有空闲线程
        if (m_idle_thread_num <= 0)
        {
            return;
        }
        int ret = write(m_pipefd[1], "T", 1);
        ASSERT(ret == 1);
    }
#define EVENTS_LIMITS 64
#define EPOLL_TIMEOUT 2000
    void IOScheduler::idle()
    {
        dbg_info << "IOScheduler::idle";

        epoll_event *epevents = new epoll_event[EVENTS_LIMITS];
        // share_ptr不直接支持数组
        std::shared_ptr<epoll_event> share_events(epevents, [](epoll_event *p)
                                                  { delete[] p; });

        while (true)
        {
            uint64_t next_timeout = 0;
            if (stopping(next_timeout)) // 在stopping中，会获取到距离下一个earlist定时器的时间
            {
                dbg_info << "IOScheduler::idle stopping";
                break;
            }
            int ret;
            do
            {
                if (next_timeout != ~0ull)
                {
                    next_timeout = (int)next_timeout > EPOLL_TIMEOUT
                                       ? EPOLL_TIMEOUT
                                       : next_timeout;
                }
                else
                {
                    next_timeout = EPOLL_TIMEOUT;
                }
                // 等待接收事件
                ret = epoll_wait(m_epfd, epevents, EVENTS_LIMITS, (int)next_timeout);
                if (ret <= 0 && errno == EINTR)
                {
                }
                else
                {
                    break;
                }
            } while (true);

            // 处理定时器到期事件
            std::vector<std::function<void()>> cbs;
            collectExpiredCb(cbs);
            if (!cbs.empty())
            {

                join(cbs.begin(), cbs.end());
                cbs.clear();
            }
            // 处理接收到的事件
            for (int i = 0; i < ret; i++)
            {
                epoll_event &event = epevents[i];
                // 当有其他任务或者协程加入调度器时，会触发这个管道事件
                if (event.data.fd == m_pipefd[0])
                {
                    uint8_t dummy;
                    while (read(m_pipefd[0], &dummy, 1) == 1)
                        ; // EPOLL_ET模式，需要读干净
                    continue;
                }
                // 其他事件
                FdEvent *fd_evt = (FdEvent *)event.data.ptr;
                Mutex::MutexLockGuardType lock(fd_evt->mutex);
                if (event.events & (EPOLLERR | EPOLLHUP))
                {
                    event.events |= (EPOLLIN | EPOLLOUT) & fd_evt->events;
                    ;
                }
                // epoll_wait返回的事件是动态的，什么发生了是什么事件，因此和注册时不同，需要获取和判断
                int tmp_events = NONE;
                // 获取发生的事件类型
                if (event.events & EPOLLIN)
                {
                    tmp_events |= READ;
                }
                if (event.events & EPOLLOUT)
                {
                    tmp_events |= WRITE;
                }
                if (tmp_events & fd_evt->events == NONE)
                {
                    continue;
                }
                // 获取注册但未发生事件
                int left_events = fd_evt->events & ~tmp_events;
                // 重新把剩余事件加入epoll树
                int ep_opt = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                epoll_event epevent;
                memset(&epevent, 0, sizeof(epevent));
                epevent.events = EPOLLET | left_events;
                ret = epoll_ctl(m_epfd, ep_opt, fd_evt->fd, &epevent);
                if (ret < 0)
                {
                    dbg_error << "IOScheduler::idle epoll_ctl error";
                    continue;
                }
                // 将已经发生的事件加入io调度器
                if (tmp_events & READ)
                {
                    fd_evt->trigger(READ);
                    --m_pendingEventCount;
                }
                if (tmp_events & WRITE)
                {
                    fd_evt->trigger(WRITE);
                    --m_pendingEventCount;
                }
            }
            // swapOut走，但没有离开作用于，因此cur.reset手动将share_ptr计数-1
            Fiber::Ptr cur = Fiber::getThis();

            auto raw_ptr = cur.get();
            cur.reset();

            raw_ptr->swapOut(); // 切回主协程调度其他协程
        }
    }
    bool IOScheduler::stopping()
    {
        uint64_t timeout = 0;
        return stopping(timeout);
    }

    void IOScheduler::onTimerInsertedAtFront()
    {
        tickle();
    }

    bool IOScheduler::stopping(uint64_t &timeout)
    {
        timeout = getEarliestTimer();
        return timeout == ~0ull && m_pendingEventCount == 0 && FiberScheduler::stopping();
    }

} // namespace zhao
