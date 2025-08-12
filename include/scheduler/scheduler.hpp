#ifndef __ZHAO_SCHEDULER_HPP__
#define __ZHAO_SCHEDULER_HPP__
#include <cstddef>
#include <string>
#include <memory>
#include <map>
#include <functional>
#include <assert.h>
#include "utils/utils.hpp"
#include "scheduler/worker.hpp"
#include "scheduler/lockfreequeue.hpp"
#include "scheduler/iomanager.hpp"
#include "config/configvar.hpp"
#include "scheduler/delaywork.hpp"
namespace zhao
{
    class IOManager;
    extern ConfigVar<uint64_t>::ptr g_proportion;
    extern ConfigVar<uint64_t>::ptr g_max_worker;
    extern ConfigVar<uint64_t>::ptr g_min_worker;
    class Scheduler;
    extern thread_local Scheduler *t_scheduler;
    class Scheduler:public DelayManager
    {
    friend class IOManager; // IOManager可以访问Scheduler的私有
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        Scheduler(size_t worker_num = Utils::getCpuCoreCount(), const std::string &name = "unknown");
        virtual ~Scheduler();
        std::string getName() const;
        static Scheduler *getThis();
        static void setThis(Scheduler *s);
        void start();
        void stop();

        void addWorker(Worker *worker);
        void deleteWorker(Worker *worker);
        size_t getIdleWorkerNum();
        size_t getactiveWorkerNum();
        size_t getTotalWorkerNum() const { return m_total_workers; }
        LockFreeQueue &getWorkQueue() { return m_work_list; }
        Worker *getWorkerPool() { return m_workerpool; }

        void tickle_manager(void){m_iomanager->tickle();}
        int addEvent(int fd, IOManager::Event event, std::function<void()> cb );
        bool delEvent(int fd, IOManager::Event event);
        bool cancelEvent(int fd, IOManager::Event event);
        bool cancelAll(int fd);
        

        template <class FiberOrCb>
        void schedule(FiberOrCb fb, tid_t thread = -1)
        {
            // 如果工作队列为空，添加一个任务后，需要通知其他工作线程
            bool need_tick = m_work_list.empty();
            Work::ptr work(new Work(fb, thread));
            if (work->fiber || work->cb)
                m_work_list.push(work);
            if (g_proportion->getValue() * m_total_workers <= m_work_list.size())
            {
                // 如果任务量已经是 worker 数量的 g_proportion 倍了，就增加 一个worker
                size_t new_worker_num = m_total_workers + 1;
                if (new_worker_num > g_max_worker->getValue())
                {
                    new_worker_num = g_max_worker->getValue();
                }
                assert(new_worker_num >= g_min_worker->getValue());
                if (new_worker_num > m_total_workers)
                {
                    addWorker(new Worker(this));
                    m_total_workers = new_worker_num;
                }
            }
            if (need_tick)
            {
                tickle();
            }
        }
        template <class InputIterator>
        void schedule(InputIterator begin, InputIterator end)
        {
            // 如果工作队列为空，添加一个任务后，需要通知其他工作线程
            bool need_tick = m_work_list.empty();
            while (begin != end)
            {
                schedule(*begin);
                ++begin;
            }
            if (g_proportion->getValue() * m_total_workers <= m_work_list.size())
            {
                size_t add_num = m_work_list.size() / (g_proportion->getValue() * m_total_workers);
                size_t new_worker_num = m_total_workers + add_num;
                if (new_worker_num > g_max_worker->getValue())
                {
                    new_worker_num = g_max_worker->getValue();
                }
                assert(new_worker_num >= g_min_worker->getValue());
                if (new_worker_num > m_total_workers)
                {
                    for (size_t i = m_total_workers; i < new_worker_num; ++i)
                    {
                        addWorker(new Worker(this));
                    }
                    m_total_workers = new_worker_num;
                }
            }
            if (need_tick)
            {
                tickle();
            }
        }

    public:
        template <class Func>
        void forEachWorker(Worker *head, Func f)
        {
            if (!head)
                return;
            Worker *start = head;
            Worker *curr = start;
            do
            {
                f(curr);
                curr = curr->next;
            } while (curr && curr != start);
        }
        template <class Func>
        void forEachWorkerBreak(Worker *head, Func f)
        {
            if (!head)
                return;
            Worker *start = head;
            Worker *curr = start;
            do
            {
                if (!f(curr)) // 返回false时提前退出
                    break;
                curr = curr->next;
            } while (curr && curr != start);
        }
        void tickle();

        bool stopping(){return m_auto_stop;}
        IOManager::ptr getIOManager(){return m_iomanager;}
    private:
        std::string m_name;
        Worker *m_workerpool = nullptr;
        
        size_t m_total_workers = 0;
        size_t m_init_workers = 0;
        LockFreeQueue m_work_list;
        bool m_auto_stop = false; // 是否手动调用了stop()
        IOManager::ptr m_iomanager; // IOManager实例
        //DelayManager::ptr m_delaymanager; // DelayManager实例
    };
} // namespace zhao

#endif