#ifndef __ZHAO_DELAY_WORK_HPP__
#define __ZHAO_DELAY_WORK_HPP__

#include "thread/ipc.hpp"
#include <memory>
#include <set>
#include <functional>
#include <vector>
#include "utils/utils.hpp"
namespace zhao
{
    class DelayManager;
    class DelayWork:public std::enable_shared_from_this<DelayWork>
    {
    public:
        typedef std::shared_ptr<DelayWork> ptr;

        // 构造函数
        DelayWork(uint64_t ms, std::function<void()> cb, bool recurring, DelayManager* manager)
            : m_ms(ms)
            , m_cb(cb)
            , m_recurring(recurring)
            , m_delaymanager(manager) {
            m_next = Utils::Time::getCurrentMS() + m_ms;
        }

       

        // 获取下次执行时间
        uint64_t getNext() const { return m_next; }
        
        // 新增: 获取任务间隔时间
        uint64_t getMs() const { return m_ms; }
        
        // 新增: 是否是循环任务
        bool isRecurring() const { return m_recurring; }
        /**
         * @brief 取消延时任务
         */
        bool cancel();
        /**
         * @brief 刷新延时任务
         */
        bool refresh();
        /**
         * @brief 重置延时任务
         */
        bool reset(uint64_t ms, bool from_now);
        struct Comparator
        {
            /**
             * @brief 比较定时器的智能指针的大小(按执行时间排序)
             */
            bool operator()(const DelayWork::ptr &lhs, const DelayWork::ptr &rhs) const;
        };
    
        /// 是否循环定时器
        bool m_recurring = false;
        /// 执行周期
        uint64_t m_ms = 0;
        /// 精确的执行时间
        uint64_t m_next = 0;
        /// 回调函数
        std::function<void()> m_cb;
        //
        DelayManager* m_delaymanager;


    };

    class DelayManager:public std::enable_shared_from_this<DelayManager>{
    public:
        friend class DelayWork; // DelayWorker可以访问DelayManager的私有
        typedef std::shared_ptr<DelayManager> ptr;
        bool empty();
        uint64_t getEarliestNext(void){
            RWMutex::ReadLockGuardType lock(m_mutex);
            return m_earliest;}
        void listExpiredWork(std::vector<std::function<void()> >& cbs);
        DelayWork::ptr addDelayWork(uint64_t ms, std::function<void()> cb
                        ,bool recurring = false);
        DelayWork::ptr addConditionDelayWork(uint64_t ms, std::function<void()> cb
                        ,std::weak_ptr<void> weak_cond
                        ,bool recurring = false);
    private:
        RWMutex m_mutex;
        std::set<DelayWork::ptr, DelayWork::Comparator> m_timers;
        uint64_t m_earliest = 0; //最早的定时器的执行时间


    };
}

#endif