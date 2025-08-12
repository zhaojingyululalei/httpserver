#ifndef __ZHAO_FIBER_HPP__
#define __ZHAO_FIBER_HPP__

#include <ucontext.h>
#include <memory>
#include <functional>
#include <stddef.h>
#include <stdint.h>
namespace zhao
{
    class Fiber : public std::enable_shared_from_this<Fiber>
    {
    public:
        enum State
        {
            INIT,  // 未初始化
            READY, // 初始化完毕，
            HOLD,  // 就绪，但是就是不放入任务队列
            RUN,   // 执行
            TERM,  // 结束
            EXCEPT // 错误状态
        };
        typedef std::shared_ptr<Fiber> ptr;
        Fiber(std::function<void()> cb, size_t stacksize = 0);
        ~Fiber();
        static Fiber* getThis();
        static void setThis(Fiber* f);
        static void setThis(Fiber::ptr f);
        static ptr createMainFiber();
        static ptr getMainFiber();
        static size_t totalFibers();
        static uint64_t getFiberId();
        static void run(void);
        State getState() const {return m_state;}
        void setState(State state) {m_state = state;}
        void yieldToReady();
        void yieldToHold();
        /**
         * @brief 任务执行完，但是Fiber还未销毁，可以重复利用，给与新任务
         */
        void reset(std::function<void()> cb);
        void swapIn();
        void swapOut();

    private:
        /**
         * @brief 创建主协程
         */
        Fiber();
        uint64_t m_id = 0;
        ucontext_t m_ctx;
        std::function<void()> m_cb;
        size_t m_stacksize = 0;
        uint8_t *m_stack = nullptr;
        State m_state = INIT;
    };
} // namespace zhao

#endif