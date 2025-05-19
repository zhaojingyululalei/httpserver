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

        void reset(std::function<void()> cb);
        void swapIn();
        void swapOut();

        static Ptr GetThis();
        static void yieldToReady();
        static void yieldToHold();
        static uint64_t getTotalFibersnum();


    private:
        Fiber();
        uint64_t  m_id = 0 ;
        uint64_t  m_stackSize = 0;
        State m_state = INIT;
        void *m_stack = nullptr;
        std::function<void()> m_cb;
        ucontext_t m_ctx;
    };
};

#endif