#include "scheduler/delaywork.hpp"

namespace zhao
{
    bool DelayWork::Comparator::operator()(const DelayWork::ptr &lhs, const DelayWork::ptr &rhs) const
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
        if (lhs->m_next < rhs->m_next)
        {
            return true;
        }
        if (rhs->m_next < lhs->m_next)
        {
            return false;
        }
        return lhs.get() < rhs.get();
    }

    bool DelayWork::cancel()
    {
        WriteLockGuard<RWMutex> lock(m_delaymanager->m_mutex);
        if (m_cb)
        {
            m_cb = nullptr;
            auto it = m_delaymanager->m_timers.find(shared_from_this());
            if (it != m_delaymanager->m_timers.end())
            {
                m_delaymanager->m_timers.erase(it);
                return true;
            }
        }
        return false;
    }

    bool DelayWork::refresh()
    {
        WriteLockGuard<RWMutex> lock(m_delaymanager->m_mutex);
        if (!m_cb)
        {
            return false;
        }
        auto it = m_delaymanager->m_timers.find(shared_from_this());
        if (it == m_delaymanager->m_timers.end())
        {
            return false;
        }
        m_delaymanager->m_timers.erase(it);
        m_next = Utils::Time::getCurrentMS() + m_ms;
        m_delaymanager->m_timers.insert(shared_from_this());
        return true;
    }

    bool DelayWork::reset(uint64_t ms, bool from_now)
    {
        if (ms == m_ms && !from_now)
        {
            return true;
        }
        uint64_t start = 0;
        if (from_now)
        {
            start = Utils::Time::getCurrentMS();
        }
        else
        {
            start = m_next - m_ms;
        }
        m_ms = ms;
        m_next = start + m_ms;
        return true;
    }

    bool DelayManager::empty()
    {
        RWMutex::ReadLockGuardType lock(m_mutex);
        return m_timers.empty();
    }

    void DelayManager::listExpiredWork(std::vector<std::function<void()>> &cbs)
    {
        uint64_t now_ms = Utils::Time::getCurrentMS();
        std::vector<DelayWork::ptr> expired;
        {
            RWMutex::ReadLockGuardType lock(m_mutex);
            if (m_timers.empty())
            {
                return;
            }
        }
        RWMutex::WriteLockGuardType lock(m_mutex);
        bool rollover = false;
        auto it = m_timers.begin();
        while (it != m_timers.end() && (*it)->getNext() <= now_ms)
        {
            expired.push_back(*it);
            ++it;
        }

        while (!expired.empty())
        {
            auto &timer = expired.front();
            cbs.push_back(timer->m_cb);
            if (timer->m_recurring)
            {
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            }
            else
            {
                timer->m_cb = nullptr;
            }
            expired.erase(expired.begin());
        }

        if (!m_timers.empty())
        {
            m_earliest = (*m_timers.begin())->getNext();
        }
        else
        {
            m_earliest = ~0ull;
        }
    }

    DelayWork::ptr DelayManager::addDelayWork(uint64_t ms, std::function<void()> cb, bool recurring)
    {
        DelayWork::ptr timer(new DelayWork(ms, cb, recurring, this));
        RWMutex::WriteLockGuardType lock(m_mutex);
        auto it = m_timers.insert(timer).first;
        m_earliest = (*m_timers.begin())->getNext();
        return timer;
    }

    DelayWork::ptr DelayManager::addConditionDelayWork(uint64_t ms, std::function<void()> cb,
                                                       std::weak_ptr<void> weak_cond,
                                                       bool recurring)
    {
        return addDelayWork(ms, [cb, weak_cond]()
                            {
        if(auto tmp = weak_cond.lock()) {
            cb();
        } }, recurring);
    }
}