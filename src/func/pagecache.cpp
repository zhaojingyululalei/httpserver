#include "pagecache.hpp"
#include "mem/common.hpp"
#include "mem/objectpool.hpp"
namespace zhao
{
    extern ObjectPool<Span> span_pool;
    Span *PageCache::offerSpan(int n_page)
    {
        assert(n_page > 0);
        // 如果单次申请的页数超过128页时才需要向os申请，如果没有超过128页的还可以向pc申请
        // pc管理1页~128页 即最大512K的span(128个桶)  CC管理8B~256KB的块（208个桶）
        if (n_page > PC_SPANLIST_SIZE - 1)
        {
            // 直接向os申请
            void *ptr = (void *)mmap(nullptr, n_page * GET_PAGE_SIZE(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (ptr == MAP_FAILED)
            {
                throw std::bad_alloc();
            }

            // 开一个新的span，用来管理新的空间
            Span *span = span_pool._new();

            span->page_id = ((pageid_t)ptr >> PAGE_SHIFT); // 申请空间的对应页号
            span->n_pages = n_page;                        // 申请了多少页

            // 把这个span管理的首页映射到哈希中，后面在删除这个span的时候能找到就行
            m_spanmap[span->page_id] = span;
            // 不需要把这个span让pc管理，pc只能管小于128页的span

            return span;
        }

        // 找到idx桶
        int idx = n_page;
        // idx号桶有span
        if (!m_spanlists[idx].is_empty())
        {

            Span *span = m_spanlists[idx].pop();
            for (pageid_t i = 0; i < span->n_pages; i++)
            {
                m_spanmap[span->page_id + i] = span;
            }

            return span;
        }
        // idx号桶没span，但后面比idx更大的桶有span
        for (size_t i = idx + 1; i < PC_SPANLIST_SIZE; i++)
        {
            if (!m_spanlists[i].is_empty())
            {
                Span *n_span = m_spanlists[i].pop();
                // 划分成n_page(k)页和i(n)-n_page(k)页
                Span *k_span = span_pool._new();
                // k页
                k_span->page_id = n_span->page_id;
                k_span->n_pages = n_page;

                // n-k页
                n_span->page_id += n_page;
                n_span->n_pages -= n_page;

                // n-k页放回哈希桶
                m_spanlists[n_span->n_pages].push(n_span);

                // 映射该n-k页span到hashmap，以便后续合并快速查找
                m_spanmap[n_span->page_id] = n_span;                       // 映射start页----》span
                m_spanmap[n_span->page_id + n_span->n_pages - 1] = n_span; // 映射end页----》span

                // 映射分配出去的k页，每页都要映射
                for (pageid_t i = 0; i < k_span->n_pages; i++)
                {
                    m_spanmap[k_span->page_id + i] = k_span;
                }
                return k_span;
            }
        }

        // 彻底没有span，需要向系统申请
        size_t req_size = (PC_SPANLIST_SIZE - 1) << PAGE_SHIFT; // 128*pagesize
        void *memory = (uint8_t *)mmap(nullptr, req_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        Span *bigSpan = span_pool._new();
        bigSpan->page_id = ((pageid_t)memory) >> PAGE_SHIFT; // 转成页号
        bigSpan->n_pages = PC_SPANLIST_SIZE - 1;             // 128页
        m_spanlists[PC_SPANLIST_SIZE - 1].push(bigSpan);
        return offerSpan(n_page);
    }
    Span *PageCache::mapObjToSpan(void *obj)
    {
        pageid_t page_id = ((pageid_t)obj) >> PAGE_SHIFT;
        Mutex::MutexLockGuardType lock(m_mutex);
        Span** spanAddr = m_spanmap.find(page_id);
        if (!spanAddr)
        {
           return nullptr;
        }
        Span *span = *(spanAddr);
        return span;
        
    }
    void PageCache::collectSpan(Span *span)
    {
        // 如果还回来的页数大于128页，PC管理不了，只能管理1~128页的span
        // 直接还给os
        if (span->n_pages > PC_SPANLIST_SIZE - 1)
        {
            void *ptr = (void *)(span->page_id << PAGE_SHIFT); // 找到要释放的地址
            size_t size  = span->n_pages * GET_PAGE_SIZE();
            // 释放内存
            if (munmap(ptr, size) == -1)
            {
                std::cout << "ptr: " << ptr << " munmap fail"<<std::endl;
                throw std::bad_alloc();
            }
            span_pool._delete(span);
            return;
        }
        // 向左不断合并
        while (1)
        {
            pageid_t leftID = span->page_id - 1;        // 拿到左边相邻页
            Span** leftSpanAddr = m_spanmap.find(leftID);// 通过相邻页映射出对应span
            // 没有相邻span，停止合并
            if (leftSpanAddr == nullptr)
            {
                break;
            }
            Span *leftSpan = *(leftSpanAddr); 
            // 相邻span在cc中，停止合并
            if (leftSpan->inUse == true)
            {
                break;
            }

            // 相邻span与当前span合并后超过128页，停止合并
            if (leftSpan->n_pages + span->n_pages > PC_SPANLIST_SIZE - 1)
            {
                break;
            }

            // 当前span与相邻span进行合并
            span->page_id = leftSpan->page_id;
            span->n_pages += leftSpan->n_pages;

            // 将相邻span对象从桶中删掉
            m_spanlists[leftSpan->n_pages].remove(leftSpan);
            span_pool._delete(leftSpan); // // 用定长内存池删除span
        }

        // 向右不断合并
        while (1)
        {
            pageid_t rightID = span->page_id + span->n_pages; // 右边的相邻页
            Span** rightSpanAddr = m_spanmap.find(rightID);  // 通过相邻页找到对应span映射关系
            // 没有相邻span，停止合并
            if (rightSpanAddr == nullptr)
            {
                break;
            }
            Span *rightSpan = *(rightSpanAddr);   

            // 没有相邻span，停止合并
            if (rightSpan == nullptr)
            {
                break;
            }
            // 相邻span在cc中，停止合并
            if (rightSpan->inUse == true)
            {
                break;
            }

            // 相邻span与当前span合并后超过128页，停止合并
            if (rightSpan->n_pages + span->n_pages > PC_SPANLIST_SIZE - 1)
            {
                break;
            }

            // 当前span与相邻span进行合并
            span->n_pages += rightSpan->n_pages; // 往右边合并时不需要改span->page_id，
                                                 // 右边的会直接拼在span后面

            // 把桶里面的span删掉
            m_spanlists[rightSpan->n_pages].remove(rightSpan);
            span_pool._delete(rightSpan); // 用定长内存池删除span
        }
        
        //如果span不断合并之后，管理的页数已经大于128页，那么PC就管不了了，直接交给os
        if(span->n_pages > PC_SPANLIST_SIZE-1){
            void *ptr = (void *)(span->page_id << PAGE_SHIFT); // 找到要释放的地址
            size_t size  = span->n_pages * GET_PAGE_SIZE();
            // 释放内存
            if (munmap(ptr, size) == -1)
            {
                std::cout << "ptr: " << ptr << " munmap fail"<<std::endl;
                throw std::bad_alloc();
            }
            span_pool._delete(span);
        }
        else{ //走正常逻辑

            // 合并完毕，将合并的span挂到相应桶下
            m_spanlists[span->n_pages].push(span);
            span->inUse = false;
    
            // 映射当前span的边缘页
            m_spanmap[span->page_id] = span;
            m_spanmap[span->page_id + span->n_pages - 1] = span;
        }
    }
}