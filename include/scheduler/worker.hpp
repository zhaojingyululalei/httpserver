#ifndef __ZHAO_WORKER_HPP__
#define __ZHAO_WORKER_HPP__ 
#include <cstddef>
#include <string>
#include <memory>
#include <functional>
#include "scheduler/workqueue.hpp"
#include "scheduler/lockfreequeue.hpp"
#include "thread/thread.hpp"
#include "thread/ipc.hpp"
namespace zhao
{
    class Scheduler;
    class Worker :public std::enable_shared_from_this<Worker>
    { 
    friend class Scheduler;//Scheduler可以访问worker的私有
    public:
    enum State
    {
        Init,
        Idle,
        Running,
        Term,
        Except,
    };
    typedef std::shared_ptr<Worker> ptr;
    Worker(Scheduler* scheduler,const std::string& name = "");
    ~Worker();
    void run(void);
    void idle(void);
    
    uint64_t idleTime();
    std::string getName() const;
    tid_t getId() const;
    State getState() const;
    std::string toString() ;
    private:
        Worker* next = nullptr;
        Worker* pre = nullptr;
        LockFreeQueue  m_local_queue; //
        //LockFreeQueue m_cache; //缓冲区
        Thread::ptr m_thread;
        State m_state;
        std::string m_name;
        uint64_t last_run_time;//上次执行任务的时间
        bool m_stopping = false;
        Semphore m_sem;//没任务等待
        uint64_t m_id;
        Scheduler* m_scheduler;
    };
} // namespace zhao

#endif