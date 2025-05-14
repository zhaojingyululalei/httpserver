#include <iostream>
#include <time.h>

#include "log.h"
#include "ipc.h"
namespace zhao
{
    static Logger::Ptr g_logger = GET_LOGGER("system");
#define dbg_error ZHAO_LOG_ERROR(g_logger)
#define dbg_info ZHAO_LOG_INFO(g_logger)
#define dbg_warn ZHAO_LOG_WARN(g_logger)
    Semphore::Semphore(uint32_t count)
    {
        if (sem_init(&m_sem, 0, count) < 0)
        {
            throw std::logic_error("sem init error");
        }
    }
    Semphore::~Semphore()
    {
        sem_destroy(&m_sem);
    }

    void Semphore::wait()
    {

        if (sem_wait(&m_sem) == 0)
        {
            return;
        }
        throw std::logic_error("sem wait error");
    }
    void Semphore::notify()
    {
        if (sem_post(&m_sem) < 0)
        {
            throw std::logic_error("sem post error");
        }
    }
    int Semphore::tryWait()
    {
        return sem_trywait(&m_sem);
    }
    int Semphore::timewait(uint32_t ms)
    {
        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += ms / 1000;
        ts.tv_nsec += (ms % 1000) * 1000000;
        return sem_timedwait(&m_sem, &ts);
    }
}
