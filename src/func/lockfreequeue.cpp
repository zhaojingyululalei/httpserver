#include "scheduler/lockfreequeue.hpp"

namespace zhao
{
    void LockFreeQueue::push(Work::ptr work)
    {
        RWMutex::WriteLockGuardType lock(m_mutex);
        WorkQueue::push(work);
    }

    Work::ptr LockFreeQueue::pop()
    {
        RWMutex::WriteLockGuardType lock(m_mutex);
        return WorkQueue::pop();
    }

    void LockFreeQueue::push_range(std::list<Work::ptr> &works)
    {
        RWMutex::WriteLockGuardType lock(m_mutex);
        WorkQueue::push_range(works);
    }

    std::list<Work::ptr> LockFreeQueue::pop_range(int start, int len)
    {
        RWMutex::WriteLockGuardType lock(m_mutex);
        return WorkQueue::pop_range(start, len);
    }

    std::list<Work::ptr> LockFreeQueue::pop_range()
    {
        RWMutex::WriteLockGuardType lock(m_mutex);
        return WorkQueue::pop_range();
    }

    bool LockFreeQueue::empty() 
    {
        RWMutex::ReadLockGuardType lock(m_mutex);
        return WorkQueue::empty();
    }

    size_t LockFreeQueue::size() 
    {
        RWMutex::ReadLockGuardType lock(m_mutex);
        return WorkQueue::size();
    }
}