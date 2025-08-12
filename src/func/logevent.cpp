#include "log/logevent.hpp"
#include "log/loglevel.hpp"
#include "log/logger.hpp"
#include "logevent.hpp"
namespace zhao
{
    LogEvent::LogEvent(const char *file, int32_t line, uint32_t elapse, tid_t thread_id, uint32_t fiber_id, uint64_t time, const std::string &thread_name)
        : m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id), m_time(time), m_threadName(thread_name)
    {
    }
    LogEventWrapper::LogEventWrapper(std::shared_ptr<Logger> logger, LogEvent::ptr event, LogLevel::Level level, const std::string &module_name)
        : m_event(event), m_logger(logger), m_level(level), m_module_name(module_name)
    {
        logger->addModule(module_name);
    }
    LogEventWrapper::~LogEventWrapper()
    {
        m_logger->log(m_event, m_level, m_module_name);
    }
}