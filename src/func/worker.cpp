#include "scheduler/worker.hpp"
#include "log/logger.hpp"
#include "utils/utils.hpp"
#include "config/configvar.hpp"
#include "scheduler/worker.hpp"
#include "scheduler/scheduler.hpp"
#include "fiber/fiber.hpp"
#include "log/logger.hpp"
#include "hook.hpp"
#include <sstream>
#define MODULE_NAME "scheduler"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))
namespace zhao
{
    static uint64_t g_id = 0;
    static thread_local Fiber *s_main_fiber;
    //static Scheduler *m_scheduler;
    /*工人心跳，长时间不跳退出*/
    static auto g_heartbeat = zhao::ConfigMgr::getInstance()->add<uint64_t>("scheduler.worker.heartbeat", 3 * 1000, "detect worker heartbeat");
    /*工人能力，能处理的任务数 */
    static auto g_ability = zhao::ConfigMgr::getInstance()->add<size_t>("scheduler.worker.ability", 4, "worker`s ablity");
    Worker::Worker(Scheduler* scheduler,const std::string &name)
    {
        assert(scheduler);
        m_scheduler = scheduler;
        m_id = g_id++;
        if(name.empty())
        {
            m_name = "worker_" + std::to_string(m_id);
        }else{
            m_name = name;
        }
        
        m_state = Init;
        m_thread = Thread::ptr(new Thread(std::bind(&Worker::run, this), "worker"));
    }
    Worker::~Worker()
    {
        m_thread->join();
        dbg << m_name << "  destory";
    }
    void Worker::run()
    {
        s_main_fiber = Fiber::createMainFiber().get();
        Scheduler::setThis(m_scheduler);
        set_hook_enable(true);
        Work::ptr work = nullptr;
        // idle协程
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Worker::idle, this)));
        size_t ability = g_ability->getValue();
        while (true)
        {
            // 从本地任务队列中取个任务
            if (!m_local_queue.empty())
            {
                work = m_local_queue.pop();
            }
            else
            {
                // 从scheduler的无锁队列中批量取任务到本地，每个工人有个ability能力值，缺多少取多少
                size_t scheduler_has = m_scheduler->getWorkQueue().size();
                size_t need = ability > scheduler_has ? scheduler_has : ability;
                std::list<Work::ptr> works = m_scheduler->getWorkQueue().pop_range(0, need);
                m_local_queue.push_range(works);
                if (!m_local_queue.empty())
                {
                    work = m_local_queue.pop();
                }
            }
            //没有任务尝试去其他worker中偷任务
            if(!work){
                m_scheduler->forEachWorkerBreak(m_scheduler->getWorkerPool(), [this, &work](Worker *w)
                                           {
                    if (w != this && !w->m_local_queue.empty())
                    {
                        work = w->m_local_queue.pop();
                        if (work)
                        {
                            dbg << "worker steal work from " << w->toString();
                            return false; // 找到任务就退出
                        }
                    }
                    return true;
                });
            }
            //如果还是没任务
            if (!work)
            {
                if (m_stopping)
                {
                    break;
                }
                // 等待
                if (idle_fiber->getState() == Fiber::TERM)
                {
                    dbg << "idle fiber term";
                    break;
                }

                idle_fiber->swapIn();

                if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT)
                {
                    idle_fiber->setState(Fiber::HOLD);
                }else {
                    dbg << "idle fiber is term or except, exit";
                    break;
                }
                if (m_stopping && m_local_queue.empty())
                {
                    break;
                }
            }
            else
            {
                m_state = Running;
                last_run_time = Utils::Time::getCurrentMS();
                if (work->fiber && (work->fiber->getState() != Fiber::EXCEPT && work->fiber->getState() != Fiber::TERM))
                {
                    work->fiber->swapIn();

                    // 协程执行完，切回主协程后
                    if (work->fiber->getState() == Fiber::READY)
                    {                                       // swap_out后是ready状态
                        m_scheduler->schedule(work->fiber); // 继续调度该协程
                    }
                    else if (work->fiber->getState() != Fiber::TERM && work->fiber->getState() != Fiber::EXCEPT)
                    {
                        work->fiber->setState(Fiber::HOLD);
                    }
                }
                else if (work->cb)
                {
                    Fiber::ptr f(new Fiber(work->cb));
                    f->swapIn();
                    // 协程执行完，切回主协程后
                    if (f->getState() == Fiber::READY)
                    {                             // swap_out后是ready状态
                        m_scheduler->schedule(f); // 继续调度该协程
                    }
                    else if (f->getState() != Fiber::TERM && f->getState() != Fiber::EXCEPT)
                    {
                        f->setState(Fiber::HOLD);
                    }
                }
                else
                {
                    warn << "this is an empty work";
                }
                // work处理完毕
                work.reset();
            }
        }
        m_state = Term;
        
        dbg << "worker exit run";
    }
    std::string Worker::getName() const
    {
        return m_name;
    }
    void Worker::idle(void)
    {
        while (true)
        {
            m_state = Idle;
            dbg << "worker enter idle";
            m_sem.timewait(g_heartbeat->getValue());
            //每个worker唤醒后，遍历其他worker，如果有Term状态的Worker，就删除
            //后续交给iomanger线程来做，让他来删除
            if(m_scheduler)
            {
                if(m_scheduler->getIOManager())
                    m_scheduler->tickle_manager();
            }
            auto min_worker = zhao::ConfigMgr::getInstance()->lookup<uint64_t>("scheduler.min_worker");
            assert(min_worker);
            if(last_run_time + g_heartbeat->getValue() < Utils::Time::getCurrentMS()
            && m_scheduler->getTotalWorkerNum() > min_worker->getValue()) // 长时间没任务，并且当前worker数量大于最小值，就退出
            {
                break;
            }
            dbg << "worker exit idle";
            Fiber::getThis()->yieldToHold(); // 切回主协程
        }
        dbg << "worker heartbeat timeout, exit idle";
        return;
    }
    uint64_t Worker::idleTime()
    {
        uint64_t now = Utils::Time::getCurrentMS();
        return now - last_run_time;
    }
    tid_t Worker::getId() const
    {
        return m_thread->getId();
    }
    Worker::State Worker::getState() const
    {
        return m_state;
    }
    std::string Worker::toString() 
    {
        std::stringstream ss;
        ss << "Worker(name=" << m_name
           << ", id=" << getId()
           << ", state=" << static_cast<int>(m_state)
           << ", last_run_time=" << last_run_time
           << ", workqueue_size=" << m_local_queue.size() << ")";
        return ss.str();
    }
}
