#include "fiber.h"
#include "log.h"
#include "config.h"
#include <atomic>
namespace zhao
{
    static std::atomic<uint64_t> sg_fiber_id = 0;
    static std::atomic<uint64_t> sg_fiber_count = 0;
    static thread_local Fiber* st_main_fiber = nullptr;
    static thread_local Fiber::Ptr stp_main_fiber = nullptr;
    static ConfigItem<uint64_t>::Ptr  g_fiber_stack_size = Config::lookup("system.fiber_stack_size", (uin64_t)(1024 * 1024 * 1), "fiber stack size");
    Fiber::Fiber(std::function<void()> cb, size_t stack_size, bool use_caller)
    {
        if (use_caller)
        {
            // 直接捕获当前上下文作为起点
            getcontext(&m_ctx);
            m_ctx.uc_stack.ss_sp = nullptr; // 标记使用调用者栈
            m_ctx.uc_stack.ss_size = 0;
            makecontext(&m_ctx, cb.target<void()>(), 0);
        }
        else
        {
            // 常规独立栈分配
            m_stack = malloc(stack_size);
            getcontext(&m_ctx);
            m_ctx.uc_stack.ss_sp = m_stack;
            m_ctx.uc_stack.ss_size = stack_size;
            makecontext(&m_ctx, cb.target<void()>(), 0);
        }
    }
} // namespace zhao
