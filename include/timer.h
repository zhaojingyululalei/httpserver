#ifndef __TIMER_H
#define __TIMER_H

#include <vector>
#include <set>
#include <memory>
#include <functional>
#include "ipc.h"

namespace zhao
{
    class TimerScheduler;
    /**
     * @brief 定时器
     */
    class Timer : public std::enable_shared_from_this<Timer>
    {
        friend class TimerScheduler;

    public:
        /// 定时器的智能指针类型
        typedef std::shared_ptr<Timer> ptr;

        /**
         * @brief 取消定时器
         */
        bool cancel();

        /**
         * @brief 刷新设置定时器的执行时间
         */
        bool refresh();

        /**
         * @brief 重置delay和excute时间
         */
        bool reset(uint64_t delayms, bool from_now=true);

    private:
        /**
         * @brief 构造函数
         */
        Timer(uint64_t delayms, std::function<void()> cb,
              bool circle, TimerScheduler *scheduler);
        /**
         * @brief 构造函数
         */
        Timer(uint64_t excutems);

    private:
        /// 是否循环定时器
        bool m_circle = false;
        /// 执行周期
        uint64_t m_delayms = 0;
        /// 精确的执行时间
        uint64_t m_excutems = 0;
        /// 回调函数
        std::function<void()> m_cb;
        /// 定时器管理器
        TimerScheduler *m_scheduler = nullptr;

    private:
        /**
         * @brief 定时器比较仿函数
         */
        struct Comparator
        {
            /**
             * @brief 按执行时间(excutems)排序
             */
            bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
        };
    };

    /**
     * @brief 定时器管理器
     */
    class TimerScheduler
    {
        friend class Timer;

    public:
        /**
         * @brief 构造函数
         */
        TimerScheduler();

        /**
         * @brief 析构函数
         */
        virtual ~TimerScheduler();

        /**
         * @brief 添加定时器
         */
        Timer::ptr addTimer(uint64_t delayms, std::function<void()> cb, bool circle = false);

        /**
         * @brief 添加条件定时器
         */
        Timer::ptr addConditionTimer(uint64_t delayms, std::function<void()> cb, std::weak_ptr<void> cond, bool circle = false);

        /**
         * @brief 到最近一个定时器执行的时间间隔(毫秒),还有多久距离最近的一个定时器到期
         */
        uint64_t getEarliestTimer();

        /**
         * @brief 获取需要执行的定时器的回调函数列表
         */
        void collectExpiredCb(std::vector<std::function<void()>> &cbs);

        /**
         * @brief 是否有定时器
         */
        bool hasTimer();

    protected:
        /**
         * @brief 当有新的定时器插入到定时器的首部,执行该函数
         */
        virtual void onTimerInsertedAtFront() = 0;

        /**
         * @brief 将定时器添加到管理器中
         */
        void addTimer(Timer::ptr val, RWMutex::WriteLockGuardType &lock);

    private:
        /**
         * @brief 检测服务器时间是否被调后了
         */
        bool detectClockRollover(uint64_t now_ms);

    private:
        /// Mutex
        RWMutex m_mutex;
        /// 定时器集合
        std::set<Timer::ptr, Timer::Comparator> m_timers;
        /// 是否触发onTimerInsertedAtFront
        bool m_tickled = false;
        /// 上次执行时间
        uint64_t m_previouseTime = 0;
    };
} // namespace zhao

#endif