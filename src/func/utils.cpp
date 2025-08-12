#include "utils/utils.hpp"
#include "utils.hpp"
#include <unistd.h>
#include <stdio.h>
#include <string>
#include "fiber/fiber.hpp"
namespace zhao
{
    tid_t Utils::getCurrentThreadId()
    {
        return syscall(SYS_gettid);
    }

    size_t Utils::Memory::getPageSize()
    {
        return getpagesize();
    }
    uint32_t Utils::getFiberId()
    {
        return Fiber::getFiberId();
    }
    std::string Utils::getCurrentThreadName()
    {
        char name[16];
        pthread_getname_np(pthread_self(), name, sizeof(name));
        return name;
    }
    size_t Utils::getCpuCoreCount()
    {
        return sysconf(_SC_NPROCESSORS_ONLN);
    }
} // namespace zhao
