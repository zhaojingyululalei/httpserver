#include "scheduler/scheduler.hpp"
#include "log/logger.hpp"
#include "scheduler/scheduler.hpp"
#include <assert.h>
#define MODULE_NAME "scheduler"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))

namespace zhao
{
    //如果累积的任务量已经是 worker 数量的 g_proportion 倍了，就增加 一个worker
    ConfigVar<uint64_t>::ptr g_proportion = zhao::ConfigMgr::getInstance()->add<uint64_t>("scheduler.proportion", 8, "worker increase proportion");
    ConfigVar<uint64_t>::ptr g_max_worker = zhao::ConfigMgr::getInstance()->add<uint64_t>("scheduler.max_worker", 12, "max worker number");
    ConfigVar<uint64_t>::ptr g_min_worker = zhao::ConfigMgr::getInstance()->add<uint64_t>("scheduler.min_worker", 1, "min worker number");
    thread_local Scheduler *t_scheduler = nullptr;
    Scheduler::Scheduler(size_t worker_num, const std::string &name)
        : m_name(name), m_init_workers(worker_num)
    {
    }
    Scheduler::~Scheduler()
    {
        assert(m_auto_stop);
    }
    std::string Scheduler::getName() const
    {
        return m_name;
    }
    Scheduler *Scheduler::getThis()
    {
        return t_scheduler;
    }
    void Scheduler::setThis(Scheduler *s)
    {
        t_scheduler = s;
    }
    void Scheduler::start()
    {
        //setThis(this);
        dbg << "scheduler start";
        //m_delaymanager = std::make_shared<DelayManager>();
        assert(m_workerpool == nullptr);

        for (int i = 0; i < m_init_workers; i++)
        {
            addWorker(new Worker(this, "worker_" + std::to_string(i)));
        }
        m_iomanager = std::make_shared<IOManager>(this);
        forEachWorker(m_workerpool, [](Worker *w)
                      { dbg << w->toString(); });
    }
    void Scheduler::stop()
    {
        m_auto_stop = true;
        forEachWorker(m_workerpool, [](Worker *w)
                      {
        w->m_stopping = true;
        w->m_sem.notify(); });
        
        if (!m_workerpool)
            return;

        Worker *start = (Worker *)m_workerpool;
        Worker *curr = start->next;

        // 断开 & 删除
        do
        {
            Worker *to_delete = curr;
            curr = curr->next;
            delete to_delete;
        } while (curr && curr != start);

        delete start; // 最后删除头
        m_workerpool = nullptr;
        tickle_manager(); // 唤醒 IOManager 线程
        dbg << "scheduler stop";
    }
    void Scheduler::addWorker(Worker *worker)
    {
        if (!m_workerpool)
        {
            // 第一个 worker

            m_workerpool = worker;
            worker->next = worker;
            worker->pre = worker;
        }
        else
        {
            // 插入到 m_workerpool 前面（尾部）
            Worker *head = m_workerpool;
            Worker *tail = head->pre;

            worker->next = head;
            worker->pre = tail;
            tail->next = worker;
            head->pre = worker;
        }
        m_total_workers++;
        dbg<<"add a worker:" << worker->toString();
    }
    void Scheduler::deleteWorker(Worker *worker)
    {
        if (!m_workerpool)
            return;

        Worker *target = worker;

        if (target->next == target)
        {
            // 只有一个节点
            m_workerpool = nullptr;
        }
        else
        {
            // 调整指针
            target->pre->next = target->next;
            target->next->pre = target->pre;

            // 如果是头节点，更新 m_workerpool
            if (m_workerpool == target)
            {
                m_workerpool = target->next;
            }
        }
        // 断开自身，避免悬挂指针
        target->next = nullptr;
        target->pre = nullptr;
        delete worker;
        m_total_workers--;
        dbg << "delete a worker:" << worker->toString();
    }

    void Scheduler::tickle()
    {
        dbg << "Scheduler::tickle()";
        // 通知所有线程来任务了，
        // 遍历一遍worker，全部唤醒
        forEachWorker(m_workerpool, [](Worker *w)
                      {
            w->m_sem.notify();
        });
    }

    size_t Scheduler::getIdleWorkerNum()
    {
        size_t count = 0;
        if (!m_workerpool)
            return count;

        Worker *head = m_workerpool;
        Worker *cur = head;
        do
        {
            if (cur->getState() == Worker::Idle)
            {
                ++count;
            }
            cur = cur->next;
        } while (cur != head);
        return count;
    }

    size_t Scheduler::getactiveWorkerNum()
    {
        size_t count = 0;
        if (!m_workerpool)
            return count;

        Worker *head = m_workerpool;
        Worker *cur = head;
        do
        {
            if (cur->getState() == Worker::Running)
            {
                ++count;
            }
            cur = cur->next;
        } while (cur != head);
        return count;
    }

    int Scheduler::addEvent(int fd, IOManager::Event event, std::function<void()> cb)
    {
        return m_iomanager->addEvent(fd, event, cb);
    }
    bool Scheduler::delEvent(int fd, IOManager::Event event)
    {
        return m_iomanager->delEvent(fd, event);
    }
    bool Scheduler::cancelEvent(int fd, IOManager::Event event)
    {
        return m_iomanager->cancelEvent(fd, event);
    }
    bool Scheduler::cancelAll(int fd)
    {
        return m_iomanager->cancelAll(fd);
    }
}