#include "fiber/fiber.hpp"
#include "config/configvar.hpp"
#include "log/logger.hpp"
#include <assert.h>
#include <atomic>
#include <iostream>
#include <stdlib.h>

#define MODULE_NAME "fiber"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))
namespace zhao
{
    static std::atomic<uint64_t> s_fiber_id{0};
    static std::atomic<uint64_t> s_fiber_count{0};
    static thread_local Fiber* t_cur_fiber;
    static thread_local Fiber::ptr t_main_fiber;
    static auto g_fiber_stack_size = zhao::ConfigMgr::getInstance()->add<uint64_t>("fiber.stacksize", 128*1024, "fiber stack size");

    Fiber::Fiber()
    {
        m_state = RUN;
        s_fiber_count++;
        m_id = ++s_fiber_id;
        getcontext(&m_ctx);
        
    }
    Fiber::Fiber(std::function<void()> cb, size_t stacksize)
        : m_cb(cb), m_id(++s_fiber_id)
    {
        if (!t_main_fiber)
        {
            Fiber::createMainFiber();
            dbg << "create a main fiber";
        }
        ++s_fiber_count;
        m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
        m_stack = (uint8_t*)malloc(m_stacksize);
        getcontext(&m_ctx);
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;
        m_ctx.uc_link = nullptr;
        makecontext(&m_ctx, &Fiber::run, 0);
        dbg << "create Fiber id=" << m_id << " total=" << s_fiber_count;
    }
    Fiber::~Fiber()
    {
        --s_fiber_count;
        dbg << "Fiber::~Fiber id=" << m_id << " total=" << s_fiber_count;
        if (m_stack)
        {
            //assert(m_state == TERM || m_state == EXCEPT || m_state == INIT);

            free((void*)m_stack);
            m_stack = nullptr;
        }
        else
        {
            // 如果是主协程都销毁了
            assert(!m_cb);
            assert(m_state == RUN);

            // 普通协程销毁，可能还有其他协程要执行，因此当前协程可能是其他协程
            // 但是主协程都销毁了，肯定得设置为nullptr
            if (t_cur_fiber == this)
            {
                setThis(nullptr);
            }
        }
        
    }
    Fiber* Fiber::getThis()
    {
        if (!t_cur_fiber)
        {
            error << "Fiber::getThis dbg.may be not create main fiber";
            //throw std::runtime_error("Fiber::getThis dbg.may be not create main fiber");
            return nullptr;
        }
        return t_cur_fiber;
    }
    void Fiber::setThis(Fiber* f)
    {
        t_cur_fiber = f;
    }
    void Fiber::setThis(Fiber::ptr f)
    {
        t_cur_fiber = f.get();
    }
    Fiber::ptr Fiber::createMainFiber()
    {
        t_main_fiber = std::shared_ptr<Fiber>(new Fiber());
        setThis(t_main_fiber);
        return t_main_fiber;
    }
     Fiber::ptr Fiber::getMainFiber()
     {
        return t_main_fiber;
     }
    size_t Fiber::totalFibers()
    {
        return s_fiber_count;
    }
    void Fiber::run(void)
    {
        Fiber* cur = t_cur_fiber;
        try
        {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }
        catch (std::exception &e)
        {
            cur->m_state = EXCEPT;
            warn << "fiber run expect:" << e.what() << " fiber id=" << cur->m_id;
        }
        catch (...)
        {
            cur->m_state = EXCEPT;
            warn << "fiber run expect:" << "unknow " << " fiber id=" << cur->m_id;
        }
        //协程切出去了，栈帧没有销毁，cur这个shared_ptr无法减1，因此在这里调用reset手动减1
        cur->swapOut();
    }
    uint64_t Fiber::getFiberId()
    {
        if(!t_cur_fiber){
            //dbg << "Fiber::getFiberId dbg.may be not create main fiber";
           // throw std::runtime_dbg("Fiber::getFiberId dbg.may be not create main fiber");
            return 0;
        }
        return t_cur_fiber->m_id;
    }
    void Fiber::yieldToReady()
    {
        m_state = READY;
        swapOut();
    }
    void Fiber::yieldToHold()
    {
        m_state = HOLD;
        swapOut();
    }
    void Fiber::reset(std::function<void()> cb)
    {
        assert(m_stack);
        assert(m_state == TERM || m_state == EXCEPT || m_state == INIT);
        m_cb = cb;

        getcontext(&m_ctx);
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;
        m_ctx.uc_link = nullptr;
        makecontext(&m_ctx, (void (*)(void))Fiber::run, 0);
        dbg << "reset a Fiber id=" << m_id << " total=" << s_fiber_count;
    }
    void Fiber::swapIn()
    {
        setThis(shared_from_this());
        assert(m_state != RUN);
        m_state = RUN;
        if (swapcontext(&t_main_fiber->m_ctx, &m_ctx))
        {
            error << "swapcontext error";
            throw std::runtime_error("swapcontext error");
        }
    }
    void Fiber::swapOut()
    {
        setThis(t_main_fiber);
        if (swapcontext(&m_ctx, &t_main_fiber->m_ctx))
        {
            error << "swapcontext error";
            throw std::runtime_error("swapcontext error");
        }
    }
}