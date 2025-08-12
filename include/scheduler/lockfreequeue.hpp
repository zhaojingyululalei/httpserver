#ifndef __ZHAO_LOCKFREEQUEUE_HPP__
#define __ZHAO_LOCKFREEQUEUE_HPP__
#include "scheduler/workqueue.hpp"
#include "thread/ipc.hpp"
#include <memory>
namespace zhao
{
    class LockFreeQueue : public WorkQueue
    {
    public:
        typedef std::shared_ptr<LockFreeQueue> ptr;
        LockFreeQueue()
        {
        }
        ~LockFreeQueue() {}
        void push(Work::ptr work) override;
        Work::ptr pop() override;
        void push_range(std::list<Work::ptr> &works) override;
        std::list<Work::ptr> pop_range(int start, int len) override;
        std::list<Work::ptr> pop_range();
        bool empty()  ;
        size_t size()  ;

    private:
        RWMutex m_mutex;
    };

} // namespace zhao

#endif