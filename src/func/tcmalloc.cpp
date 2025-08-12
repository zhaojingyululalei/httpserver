#include "tcmalloc.hpp"
#include "mem/objectpool.hpp"
#include "thread/thread.hpp"
#include "mem/memdef.hpp"
#include "mem/pagecache.hpp"
#include "hook.hpp"
namespace zhao
{
    static ObjectPool<ThreadCache> pool;

    void *tcmalloc(size_t size)
    {
        // CC中，最大的桶维持的块数也只有256KB，即第208个桶
        // 如果用户申请空间大于256KB，直接向TC申请
        void* ptr = nullptr;
        if (size > TC_ALLOC_BYTES_MAX_SIZE)
        {
            size_t align_size = SizeAlignCalculator::RoundUp(size);
            size_t n_pages = align_size >> PAGE_SHIFT; // 算出来要申请多少页

            PageCache::getInstance()->getMutex().lock();
            Span *span = PageCache::getInstance()->offerSpan(n_pages);
            span->obj_size = size;
            PageCache::getInstance()->getMutex().unlock();
            ptr = (void *)(span->page_id << PAGE_SHIFT);
            
        }
        else // 否则还是让TC分配
        {
            if (t_thread_cache == nullptr)
            {
                t_thread_cache = pool._new();
            }
            // std::cout<<  Thread::getCurThreadName()
            // << " t_thread_cache = " <<  t_thread_cache << std::endl;
            ptr = t_thread_cache->malloc(size);
            
        }
        return ptr;
    }
    /*glibc堆空间 5xxxxx，一般是5开头，需要用原始的free释放
    我们的mmap，一般是7xxxxx*/
    void tcfree(void *ptr)
    {
        if (ptr == nullptr)
        {
            return; 
        }
        //glibc用堆分配的内存，用原始free释放
        if((uintptr_t)ptr < 0x700000000000){
            __real_free(ptr);
        }
        size_t size = 0;
        // 前面分配空间时，PC->offerspan已经映射好了
        Span *span = PageCache::getInstance()->mapObjToSpan(ptr);
    
        if(!span){
            //如果这块空间不是我分配的，可能是其他动态库自己mmap分配的，就不管了
            return;
        }
        size = span->obj_size;
        if (size > TC_ALLOC_BYTES_MAX_SIZE)
        {

            // Span还给PC，回收合并
            PageCache::getInstance()->getMutex().lock();
            PageCache::getInstance()->collectSpan(span);
            PageCache::getInstance()->getMutex().unlock();
        }
        else
        {

            t_thread_cache->free(ptr, size);
        }
    }
}
