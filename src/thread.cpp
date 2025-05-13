#include "thread.h"
#include "log.h"
#define THREAD_DEFAULT_NAME "UNKOWN"
namespace zhao
{
    static Logger::Ptr g_logger = GET_LOGGER("system");
#define dbg_error ZHAO_LOG_ERROR(g_logger)
#define dbg_info ZHAO_LOG_INFO(g_logger)
#define dbg_warn ZHAO_LOG_WARN(g_logger)

    static thread_local Thread *cur_thread = nullptr;
    Thread::Thread(ThreadFunc func, const std::string &name, void *arg)
    {
        int ret;
        if (name.empty())
        {
            setName(THREAD_DEFAULT_NAME);
        }
        else
        {
            setName(name);
        }
        m_func = func;
        ret = pthread_create(&m_thread, nullptr, run, this);
        if (ret < 0)
        {
            dbg_error << "pthread_create error: " << name;
            throw std::logic_error("pthread_create error");
        }
        m_arg = arg;
    }

    Thread::~Thread()
    {
        if (cur_thread)
        {
            pthread_detach(cur_thread->m_thread);
        }
    }

    Thread *Thread::getCur()
    {
        return cur_thread;
    }

    const std::string Thread::getCurName()
    {
        return cur_thread->m_name;
    }

    void Thread::setCurName(const std::string &name)
    {
        if (cur_thread)
            cur_thread->setName(name);
    }

    void Thread::join()
    {
        int ret;
        if (cur_thread)
        {
            ret = pthread_join(cur_thread->m_thread, nullptr);
            if (ret < 0)
            {
                dbg_error << "pthread_join error: " << cur_thread->m_name;
                throw std::logic_error("pthread_join error");
            }
            m_thread = 0;
            cur_thread = nullptr;
        }
    }

    void *Thread::run(void *arg)
    {
        Thread *thread = (Thread *)arg;
        thread->m_tid = getThreadId();
        cur_thread = thread;
        pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

        ThreadFunc func;
        func.swap(thread->m_func);
        func(thread->m_arg);
        // thread->m_func(thread->m_arg);
        return nullptr;
    }
}