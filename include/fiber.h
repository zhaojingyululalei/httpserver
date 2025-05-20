#ifndef __FIBER_H
#define __FIBER_H

#include <functional>
#include <memory>
#include <ucontext.h>

namespace zhao
{

    class Fiber : public std::enable_shared_from_this<Fiber>
    {
    public:
        enum State
        {
            INIT,
            HOLD,
            EXEC,
            READY,
            TERM,
            ERROR
        };
        typedef std::shared_ptr<Fiber> Ptr;
        Fiber(std::function<void()> cb, size_t stack_size = 0, bool use_caller = false);
        ~Fiber();
        // 不销毁，重复利用，重置协程函数和栈
        void reset(std::function<void()> cb);
        //将当前子协程切换到执行
        void swapIn();
        //将当前子协程切换到后台，主协程执行
        void swapOut();
        //获取当前协程id
        uint64_t getId();
        //设置当前协程
        static void setThis(Fiber *fiber);
        //获取当前协程
        static Ptr getThis();
        //当前子协程到后台，并由主协程执行
        static void yieldToReady();
        static void yieldToHold();
        static uint64_t getTotalFibersnum();


    private:
        //用于构造主协程，主协程不需要栈和执行函数，同线程共享栈；每个线程有一个主协程
        Fiber();
        //所有子协程的入口
        static void entry(void);
        uint64_t  m_id = 0 ;
        uint64_t  m_stackSize = 0;
        State m_state = INIT;
        void *m_stack = nullptr;
        std::function<void()> m_cb;
        ucontext_t m_ctx;
    };
};


#endif