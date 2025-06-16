#include "timer.h"
#include "utils.h"
namespace zhao
{
    bool Timer::cancel()
    {
        RWMutex::WriteLockGuardType lock(m_scheduler->m_mutex);
        if (m_cb)
        {
            m_cb = nullptr;
            auto it = m_scheduler->m_timers.find(shared_from_this());
            m_scheduler->m_timers.erase(it);
            return true;
        }
        return false;
    }

    bool Timer::refresh()
    {
        RWMutex::WriteLockGuardType lock(m_scheduler->m_mutex);
        if (!m_cb)
        {
            return false;
        }
        auto it = m_scheduler->m_timers.find(shared_from_this());
        if (it == m_scheduler->m_timers.end())
        {
            return false;
        }
        m_scheduler->m_timers.erase(it);
        m_excutems = getCurrentMS() + m_delayms;
        m_scheduler->m_timers.insert(shared_from_this());
        return true;
    }

    bool Timer::reset(uint64_t delayms, bool from_now)
    {
        if (delayms == m_delayms && !from_now)
        {
            return true;
        }
        RWMutex::WriteLockGuardType lock(m_scheduler->m_mutex);
        if (!m_cb)
        {
            return false;
        }
        auto it = m_scheduler->m_timers.find(shared_from_this());
        if (it == m_scheduler->m_timers.end())
        {
            return false;
        }
        m_scheduler->m_timers.erase(it);
        uint64_t start = from_now ? getCurrentMS() : m_excutems - m_delayms;
        m_delayms = delayms;
        m_excutems = start + m_delayms;
        m_scheduler->addTimer(shared_from_this(), lock);
        return true;
    }

    Timer::Timer(uint64_t delayms, std::function<void()> cb,
                 bool circle, TimerScheduler *scheduler)
        : m_circle(circle), m_delayms(delayms), m_cb(cb), m_scheduler(scheduler)
    {
        m_excutems = getCurrentMS() + m_delayms;
    }

    Timer::Timer(uint64_t excutems)
        : m_excutems(excutems)
    {
    }

    bool Timer::Comparator::operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const
    {
        if (!lhs && !rhs)
        {
            return false;
        }
        if (!lhs)
        {
            return true;
        }
        if (!rhs)
        {
            return false;
        }
        if (lhs->m_excutems < rhs->m_excutems)
        {
            return true;
        }
        if (rhs->m_excutems < lhs->m_excutems)
        {
            return false;
        }
        return lhs.get() < rhs.get(); // 执行时间都相同，按照对象地址排序
    }

    TimerScheduler::TimerScheduler()
    {
        m_previouseTime = getCurrentMS();
    }

    TimerScheduler::~TimerScheduler()
    {
    }

    Timer::ptr TimerScheduler::addTimer(uint64_t delayms, std::function<void()> cb, bool circle)
    {
        Timer::ptr timer(new Timer(delayms, cb, circle, this));
        RWMutex::WriteLockGuardType lock(m_mutex);
        addTimer(timer, lock);
        return timer;
    }
    static void OnTimer(std::weak_ptr<void> cond, std::function<void()> cb)
    {
        std::shared_ptr<void> tmp = cond.lock();
        if (tmp)
        {
            cb();
        }
    }
    Timer::ptr TimerScheduler::addConditionTimer(uint64_t delayms, std::function<void()> cb, std::weak_ptr<void> cond, bool circle)
    {
        return addTimer(delayms, std::bind(&OnTimer, cond, cb), circle);
    }

    uint64_t TimerScheduler::getEarliestTimer()
    {
        RWMutex::ReadLockGuardType lock(m_mutex);
        m_tickled = false;
        if (m_timers.empty())
        {
            return ~0ull;
        }

        const Timer::ptr &earlist = *m_timers.begin();
        uint64_t now_ms = getCurrentMS();
        if (now_ms >= earlist->m_excutems)
        {
            return 0; // 已经到期
        }
        else
        {
            return earlist->m_excutems - now_ms; // 还有多少ms到期
        }
    }

    void TimerScheduler::collectExpiredCb(std::vector<std::function<void()>> &cbs)
    {
        uint64_t now_ms = getCurrentMS();
        std::vector<Timer::ptr> expired;
        {
            RWMutex::ReadLockGuardType lock(m_mutex);
            if (m_timers.empty())
            {
                return; // 没有定时器
            }
        }
        RWMutex::WriteLockGuardType lock(m_mutex);
        if (m_timers.empty())
        {
            return;
        }
        bool rollover = detectClockRollover(now_ms);
        // 如果没有发生时间回拨，并且最早到期的定时器时间仍大于当前时间，则没有需要处理的过期定时器，直接返回。
        if (!rollover && ((*m_timers.begin())->m_excutems > now_ms))
        {
            return;
        }

        Timer::ptr now_timer(new Timer(now_ms));
        // 如果检测到时间回拨，则认为所有定时器都已过期。
        // 否则使用 lower_bound 找到第一个大于等于当前时间的定时器（未超时的定时器）。
        auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
        while (it != m_timers.end() && (*it)->m_excutems == now_ms)
        {
            ++it;
        }
        // 然后把前面所有已经超时的定时器取出并放到expired中
        expired.insert(expired.begin(), m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);
        cbs.reserve(expired.size());

        for (auto &timer : expired)
        {
            cbs.push_back(timer->m_cb); // 将每个定时器的回调函数 m_cb 添加到 cbs 中。
            // 如果是重复定时器
            if (timer->m_circle)
            {

                timer->m_excutems = now_ms + timer->m_delayms; // 更新其下次触发时间
                m_timers.insert(timer);                        // 重新插入到 m_timers 中，等待下一次触发
            }
            else
            {
                timer->m_cb = nullptr; // 如果是非重复定时器，则将其回调函数置空，表示不再执行
            }
        }
    }

    bool TimerScheduler::hasTimer()
    {
        RWMutex::ReadLockGuardType lock(m_mutex);
        return !m_timers.empty();
    }

    void TimerScheduler::addTimer(Timer::ptr val, RWMutex::WriteLockGuardType &lock)
    {
        // 如果刚刚插入的定时器，是set中最早的一个，那么，需要唤醒epoll_wait并修改timeout
        auto it = m_timers.insert(val).first;
        // 优化效率，不要每次加入一个定时器都tickle，如果一瞬间有很多addTimer加入的定时器，那么，就只tickle一次
        bool is_early = (it == m_timers.begin()) && !m_tickled;
        if (is_early)
        {
            m_tickled = true;
        }
        lock.unlock();

        if (is_early)
        {
            onTimerInsertedAtFront(); // 执行tickle，唤醒epoll_wait
        }
    }

    bool TimerScheduler::detectClockRollover(uint64_t now_ms)
    {
        bool rollover = false;
        if (now_ms < m_previouseTime &&
            now_ms < (m_previouseTime - 60 * 60 * 1000))
        {
            rollover = true;
        }
        m_previouseTime = now_ms;
        return rollover;
    }

} // namespace zhao
