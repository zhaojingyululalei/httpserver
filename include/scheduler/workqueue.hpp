#ifndef __ZHAO_WORKQUEUE_HPP
#define __ZHAO_WORKQUEUE_HPP
#include <list>
#include <memory>
#include <functional>
#include "fiber/fiber.hpp"
#include "utils/utils.hpp"
namespace zhao
{
    /**
     * @brief 任务
     * @details cpu型任务，尽量交给线程执行，线程不足就扩展。IO型任务交给协程执行
     */
    struct Work
    {
    public:
        typedef std::shared_ptr<Work> ptr;
        Work(Fiber::ptr f, tid_t thr)
            : fiber(f), thread(thr)
        {
        }
        Work(Fiber::ptr *f, tid_t thr)
            : thread(thr)
        {
            fiber.swap(*f);
        }
        Work(std::function<void()> f, tid_t thr)
            : cb(f), thread(thr)
        {
        }
        Work(std::function<void()> *f, tid_t thr)
            : thread(thr)
        {
            cb.swap(*f);
        }
        Work()
            : thread(-1)
        {
        }

        Fiber::ptr fiber;
        /// 协程执行函数
        std::function<void()> cb;
        tid_t thread; // 线程id
    };
    class WorkQueue
    {
    public:
        typedef std::shared_ptr<WorkQueue> ptr;
        WorkQueue();
        ~WorkQueue();

        virtual void push(Work::ptr work);
        virtual Work::ptr pop();
        /**
         * @brief 将一段链表插入到最后面
         */
        virtual void push_range(std::list<Work::ptr> &works);
        /**
         * @brief 从最前面向后截取出一段链表
         */
        virtual std::list<Work::ptr> pop_range(int start,int len);
        /**
         * @brief 全部弹出
         */
        virtual std::list<Work::ptr> pop_range();
        virtual bool empty() const;
        virtual size_t size() const;

    private:
        std::list<Work::ptr> m_works;
    };
} // namespace zhao

#endif