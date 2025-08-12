#include "threadcache.hpp"
#include "mem/centralcache.hpp"
#include <cmath>
namespace zhao
{
    thread_local ThreadCache *t_thread_cache = nullptr;
    void *ThreadCache::malloc(size_t size)
    {
        assert(size <= TC_ALLOC_BYTES_MAX_SIZE);
        size_t index = SizeAlignCalculator::index(size);
        size_t align_size = SizeAlignCalculator::RoundUp(size);
        if (!m_freelists[index].is_empty())
        {
            // 有空闲内存
            return m_freelists[index].pop();
        }
        else
        {
            return requestFromCentralCache(align_size, index);
        }
    }
    void ThreadCache::free(void *obj, size_t size)
    {
        assert(obj);
        assert(size <= TC_ALLOC_BYTES_MAX_SIZE);
        size_t index = SizeAlignCalculator::index(size);
        m_freelists[index].push(obj);
    }
    void *ThreadCache::requestFromCentralCache(size_t align_size, size_t index)
    {
        // 向CC申请大小为size的块，计算一下要申请多少块合适。
        // 里面包含调节算法，计算出比较合适的块数
        size_t batch_num = m_freelists[index].req_num_from_cc(align_size);

        // 然后就是从CC中取出batch_num个大小为align_size的块
        void *start = nullptr;
        void *end = nullptr;

        size_t actual_num = CentralCache::getInstance()->offerRangeObj(start, end, batch_num, align_size);
        assert(actual_num >= 1);
        // 就从CC中取出一个块，直接返回给线程用即可
        if (actual_num == 1)
        {
            assert(start == end);
            return start;
        }
        else
        {
            void *next = FreeList::obj_next_get(start);
            m_freelists[index].pushRange(next, end, actual_num - 1);
            m_freelists[index].m_total += actual_num;
            return start;
        }
    }
    void ThreadCache::releaseToCentralCache(int index, size_t align_size)
    {
        // 归还多少块算法 归还力度和向CC申请频率成反比：频繁向CC申请，那么归还的就少，甚至不要归还，否则多归还些。
        //  令牌数量和向CC申请频率成正比，令牌数量越多，表示最近申请的越频繁
        size_t token_count = m_freelists[index].get_tokens();
        size_t max_tokens = m_freelists[index].get_max_tokens();
        size_t free_blocks = m_freelists[index].size();
        size_t used_blocks = m_freelists[index].m_total - free_blocks;
        size_t min_retain = 0;
        if(align_size <1024){
            min_retain = 10;
        }else if(align_size < 4*1024){
            min_retain = 5;
        }else{
            min_retain = 0;
        }
        //计算归还给CC多少块
        //TC将freelist中若干块取出，然后还给CC
        size_t release_num = m_freelists[index].get_token_bucket().calculateRelease(token_count,max_tokens,free_blocks,used_blocks,min_retain);
        if(release_num==0) return;
        void* start = nullptr;
        void*end = nullptr;
        //取出若干块
        m_freelists[index].popRange(start,end,release_num);
        //还给cc
        CentralCache::getInstance()->collectRangeObj(start,align_size);
    }
    
} // namespace zhao
