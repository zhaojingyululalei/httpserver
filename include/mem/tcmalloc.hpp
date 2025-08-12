#ifndef __ZHAO_TCMALLOC_HPP__
#define __ZHAO_TCMALLOC_HPP__ 
#include "mem/threadcache.hpp"
namespace zhao { 
    void* tcmalloc(size_t size);
    void tcfree(void* ptr);
}
#endif