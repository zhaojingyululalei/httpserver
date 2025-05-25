#include "fiber.h"
#include "log.h"
#include "config.h"
#include "fiberscheduler.h"
#include <atomic>
namespace zhao
{
    static Logger::Ptr g_logger = GET_LOGGER("system");
#define ASSERT(x) ZHAO_LOG_ASSERT(x)
#define dbg ZHAO_LOG_DEBUG(g_logger)
    static std::atomic<uint64_t> sg_fiber_id = 0;
    static std::atomic<uint64_t> sg_fiber_count = 0;
    static thread_local Fiber *st_cur_fiber = nullptr;
    static thread_local Fiber::Ptr stp_main_fiber = nullptr;
    static ConfigItem<uint64_t>::Ptr g_fiber_stack_size = Config::lookup("system.fiber_stack_size", (uint64_t)(1024 * 1024 * 1), "fiber stack size");
    // 创建主协程(主协程线程化)
    Fiber::Fiber()
    {
        m_state = EXEC;
        getcontext(&m_ctx);
        setThis(this);
        ++sg_fiber_count;
    }

    // 创建其他协程
    Fiber::Fiber(std::function<void()> cb, size_t stack_size, bool use_caller) : m_id(++sg_fiber_id), m_cb(cb)
    {
        sg_fiber_count++;
        m_stackSize = stack_size ? stack_size : g_fiber_stack_size->getvalue();
        m_stack = malloc(m_stackSize);
        // 常规独立栈分配
        getcontext(&m_ctx);
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stackSize;
        if (use_caller)
        {
            makecontext(&m_ctx, Fiber::call_entry, 0);
        }
        else
        {
            makecontext(&m_ctx, Fiber::entry, 0);
        }
    }
    Fiber::~Fiber()
    {
        --sg_fiber_count;
        if (m_stack)
        {
            // 非主协程
            ZHAO_LOG_ASSERT(m_state == TERM || m_state == INIT);
            m_stackSize = 0;
            free(m_stack);
            dbg << "one child fiber destroyed";
        }
        else
        {
            // 主协程
            ZHAO_LOG_ASSERT(!m_cb && m_state == EXEC);
            if (st_cur_fiber == this)
            {
                setThis(nullptr);
            }
            dbg << "one main fiber destroyed";
        }
    }

    void Fiber::reset(std::function<void()> cb)
    {
        ZHAO_LOG_ASSERT(m_stack && (m_state == TERM || m_state == INIT));
        m_cb = cb;
        getcontext(&m_ctx);
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stackSize;
        makecontext(&m_ctx, Fiber::entry, 0);
        m_state = INIT;
    }
    /**该函数是从主携程切换到子携程，但是主携程不一定是随线程创建的那个主携程 */
    void Fiber::swapIn()
    {
        int ret;
        ZHAO_LOG_ASSERT(m_state != EXEC);
        setThis(this);
        m_state = EXEC;
        // ret = swapcontext(&stp_main_fiber->m_ctx, &m_ctx);
        ret = swapcontext(&FiberScheduler::getMainFiber()->m_ctx, &m_ctx);
        ASSERT(ret == 0);
    }
    // 子协程退出不执行
    void Fiber::swapOut()
    {
        // setThis(stp_main_fiber.get());
        setThis(FiberScheduler::getMainFiber());
        if (swapcontext(&m_ctx, &FiberScheduler::getMainFiber()->m_ctx) != 0)
        {
            dbg << "swapcontext error";
        }
    }
    /**该函数是从当前线程的主携程切换到子携程 */
    void Fiber::call()
    {
        setThis(this);
        m_state = EXEC;
        // 对于user_caller来说 主携程是随线程创建的携程
        // 对于非user_caller，在fiberscheduler 中是mp_use_caller_fiber
        swapcontext(&stp_main_fiber->m_ctx, &m_ctx);
    }
    void Fiber::back()
    {
        int ret;
        setThis(stp_main_fiber.get());
        ret = swapcontext(&m_ctx, &stp_main_fiber->m_ctx);
        ASSERT(ret == 0);
    }
    uint64_t Fiber::getId()
    {
        return m_id;
    }
    void Fiber::setThis(Fiber *fiber)
    {
        st_cur_fiber = fiber;
    }
    Fiber::Ptr Fiber::getThis()
    {
        if (st_cur_fiber)
        {
            return st_cur_fiber->shared_from_this();
        }
        // 创建主协程并返回
        Fiber::Ptr main_fiber(new Fiber);
        stp_main_fiber = main_fiber;
        return main_fiber;
    }
    void Fiber::yieldToReady()
    {
        Fiber::Ptr cur = Fiber::getThis();
        cur->m_state = READY;
        cur->swapOut();
    }

    void Fiber::yieldToHold()
    {
        Fiber::Ptr cur = Fiber::getThis();
        cur->m_state = HOLD;
        cur->swapOut();
    }

    uint64_t Fiber::getTotalFibersnum()
    {
        return sg_fiber_count;
    }
    void Fiber::entry(void)
    {
        // 由于后面swap_out跳转走了，并没有离开作用域，需要手动计数-1
        Fiber::Ptr cur = Fiber::getThis();
        ZHAO_LOG_ASSERT(cur);
        try
        {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }
        catch (std::exception &e)
        {
            cur->m_state = ERROR;
            ZHAO_LOG_ERROR(GET_ROOT_LOGGER()) << "Fiber Except:" << e.what() << " fiber id = " << cur->getId();
        }
        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->swapOut(); // 子协程处理完毕返回主协程
        ZHAO_LOG_ASSERT(false);
    }

    void Fiber::call_entry(void)
    {
        Fiber::Ptr cur = Fiber::getThis();
        ZHAO_LOG_ASSERT(cur);
        try
        {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }
        catch (std::exception &e)
        {
            cur->m_state = ERROR;
            ZHAO_LOG_ERROR(GET_ROOT_LOGGER()) << "Fiber Except:" << e.what() << " fiber id = " << cur->getId();
        }
        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->back(); // 子协程处理完毕返回主协程
        ZHAO_LOG_ASSERT(false);
    }

} // namespace zhao
