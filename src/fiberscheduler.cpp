#include "fiberscheduler.h"
#include "log.h"
#include "utils.h"
namespace zhao
{
    static Logger::Ptr g_logger = GET_LOGGER("system");
#define dbg_error ZHAO_LOG_ERROR(g_logger)
#define dbg_info ZHAO_LOG_INFO(g_logger)
#define dbg_warn ZHAO_LOG_WARN(g_logger)
#define ASSERT(x) ZHAO_LOG_ASSERT(x)
    static thread_local FiberScheduler *st_fiber_scheduler = nullptr; // 每个线程都有一个调度器
    static thread_local Fiber *st_mainfiber = nullptr;                // 每个线程都有一个负责调度的协程
    FiberScheduler::FiberScheduler(uint32_t thread_num, bool use_caller, const std::string &name)
        : m_name(name), m_thread_num(thread_num)
    {
        ASSERT(thread_num > 0);
        ASSERT(getThis() == nullptr);
        if (use_caller)
        {
            Fiber::getThis(); // 创建主协程
            --thread_num;
            st_fiber_scheduler = this; // 当前线程参与调度才有调度器，否则没有
            // 创建子协程去调度，自身不参与调度
            mp_use_caller_fiber.reset(new Fiber(std::bind(&FiberScheduler::run, this), 0, true));
            Thread::setCurName(name);
            // 该线程参与调度的主协程设置为mp_use_caller_fiber
            st_mainfiber = mp_use_caller_fiber.get();
            m_use_caller_threadId = getThreadId();
            m_threadIds.push_back(m_use_caller_threadId);
        }
        else
        {
            m_use_caller_threadId = -1;
        }
        
        
        m_thread_num = thread_num;
    }
    FiberScheduler::~FiberScheduler()
    {
        ASSERT(m_stopping);
        if (getThis() == this)
        {
            setThis(nullptr);
        }
    }
    void FiberScheduler::start(void)
    {
        dbg_info << "FiberScheduler::start";    
        Mutex::MutexLockGuardType lock(m_mutex);
        if (!m_stopping)
        {
            return;
        }
        m_stopping = false;
        ASSERT(m_threads.empty());
        //m_threads.resize(m_thread_num);
        for (uint32_t i = 0; i < m_thread_num; ++i)
        {
            m_threads.push_back(std::shared_ptr<Thread>(new Thread(std::bind(&FiberScheduler::run, this), m_name + "_" + std::to_string(i), nullptr)));
            m_threadIds.push_back(m_threads[i]->getTid());
        }

        
    }
    void FiberScheduler::stop(void)
    {
        dbg_info << "FiberScheduler::stop";
        // ASSERT(m_use_caller_threadId != -1);
        if (m_use_caller_threadId == -1)
        {
            ASSERT(getThis() != this); // 一定是主线程调用的stop
        }
        else
        {
            ASSERT(getThis() == this); // 确保是主线程调用的stop
        }
        // FiberScheduler.stop 主动停止
        m_autostop = true;
        if (mp_use_caller_fiber && m_thread_num == 0 && (mp_use_caller_fiber->getState() == Fiber::TERM || mp_use_caller_fiber->getState() == Fiber::INIT))
        {
            m_stopping = true;
            if (stopping())
            {
                // 啥都是空的，不用处理
                return;
            }
        }
        m_stopping = true;
        // 把所有等待任务的线程唤醒，然后让他们停止
        for (size_t i = 0; i < m_thread_num; ++i)
        {
            tickle();
        }
        
        if (mp_use_caller_fiber)
        {
            tickle();
            if (!stopping())
            {
                mp_use_caller_fiber->call(); // 如果在start中执行，主线程会一直运行run，主线程没有执行stop的机会
            }
        }
        std::vector<Thread::Ptr> tempThreads;
        {
            Mutex::MutexLockGuardType lock(m_mutex);
            tempThreads.swap(m_threads);
        }
        for (auto &thread : tempThreads)
        {
            thread->join();
        }
    }
    FiberScheduler *FiberScheduler::getThis(void)
    {
        return st_fiber_scheduler;
    }
    void FiberScheduler::setThis(FiberScheduler *sched)
    {
        st_fiber_scheduler = sched;
    }

    Fiber *FiberScheduler::getMainFiber()
    {

        return st_mainfiber;
    }
    void FiberScheduler::tickle()
    {
        dbg_info << "FiberScheduler::tickle";
    }
    bool FiberScheduler::stopping()
    {
        Mutex::MutexLockGuardType lock(m_mutex);
        return m_autostop && m_stopping && m_fibers.empty() && m_activated_thread_num == 0;
    }
    void FiberScheduler::idle()
    {
        dbg_info << "FiberScheduler::idle";
        // while(!stopping()){
        //     Fiber::yieldToHold();
        // }
    }
    void FiberScheduler::run()
    {
        dbg_info << "FiberScheduler::run";
        setThis(this);
        if (getThreadId() != m_use_caller_threadId)
        {
            // 给每个非主线程创建主协程，主线程的主线程已经在FiberScheduler构造中创建好了
            st_mainfiber = Fiber::getThis().get();
        }
        Fiber::Ptr idle_fiber = std::make_shared<Fiber>(std::bind(&FiberScheduler::idle, this), 0, false);
        Fiber::Ptr cb_fiber = nullptr;
        FiberDefine fd;
        while (true)
        {
            fd.reset();
            bool tickle_other = false;
            bool activated = false;
            {
                Mutex::MutexLockGuardType lock(m_mutex);
                auto it = m_fibers.begin();
                while (it != m_fibers.end())
                {
                    if (it->m_threadId != -1 && it->m_threadId != Thread::getCur()->getTid())
                    {
                        tickle_other = true;
                        ++it;
                        continue;
                    }
                    ASSERT(it->m_cb || it->m_fiber);
                    if (it->m_fiber && it->m_fiber->getState() == Fiber::State::EXEC)
                    {
                        ++it;
                        continue;
                    }

                    fd = *it;
                    m_fibers.erase(it);
                    ++m_activated_thread_num;
                    activated = true;
                    break;
                }
            }
            if (tickle_other)
            {
                tickle();
            }
            if (fd.m_fiber && (fd.m_fiber->getState() != Fiber::State::TERM || fd.m_fiber->getState() != Fiber::State::ERROR))
            {
                
                fd.m_fiber->swapIn();
                --m_activated_thread_num;
                if (fd.m_fiber->getState() == Fiber::State::READY)
                {
                    join(fd.m_fiber, fd.m_threadId);
                }
                else if (fd.m_fiber->getState() != Fiber::State::TERM && fd.m_fiber->getState() != Fiber::State::ERROR)
                {
                    ASSERT(fd.m_fiber->getState() == Fiber::State::HOLD);
                }
                fd.reset();
            }
            else if (fd.m_cb)
            {
                if (cb_fiber)
                {
                    cb_fiber->reset(fd.m_cb);
                }
                else
                {
                    cb_fiber.reset(new Fiber(fd.m_cb));
                }
                fd.reset();
                cb_fiber->swapIn();
                --m_activated_thread_num;
                if (cb_fiber->getState() == Fiber::State::READY)
                {
                    join(cb_fiber, fd.m_threadId);
                    cb_fiber.reset();
                }
                else if (cb_fiber->getState() == Fiber::State::TERM || cb_fiber->getState() == Fiber::State::ERROR)
                {
                    cb_fiber->reset(nullptr);
                }
                else
                {
                    ASSERT(cb_fiber->getState() == Fiber::State::HOLD);
                    cb_fiber.reset();
                }
                
            }
            else
            {
                if(activated){
                    --m_activated_thread_num;
                    continue;
                }
                // 没有任务执行，去执行idle
                if (idle_fiber->getState() == Fiber::State::TERM || idle_fiber->getState() == Fiber::State::ERROR)
                {
                    break;
                }
                ++m_idle_thread_num;
                idle_fiber->swapIn();
                --m_idle_thread_num;
                if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::ERROR)
                {
                    ASSERT(idle_fiber->getState() == Fiber::READY|| idle_fiber->getState() == Fiber::HOLD);
                }

            }
        }
    }
};
