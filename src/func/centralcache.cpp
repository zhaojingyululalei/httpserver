#include "mem/centralcache.hpp"
#include "mem/common.hpp"
#include "mem/pagecache.hpp"
#include "centralcache.hpp"
namespace zhao
{
    size_t zhao::CentralCache::offerRangeObj(void *&start, void *&end, size_t batch_num, size_t align_size)
    {
        //通过align_size获取桶索引
        int idx = SizeAlignCalculator::index(align_size);
        m_spanlists[idx].getMutex().lock();
        Span *span = getSpan(idx, align_size);
        //返回的必须是有效的span
        assert(span != nullptr);
        assert(!span->free_list.is_empty());
        //取出batch_num个对象,并且把这段内存弄成链表
        start = end = span->free_list.getFirstObj();
        size_t actual_num = 1;

        size_t i = 0;
        while(i < batch_num - 1 && FreeList::obj_next_get(end) != nullptr){
            end = FreeList::obj_next_get(end);
            ++actual_num; //记录end走了几步
            ++i;
        }
        span->free_list.setFirstObj(FreeList::obj_next_get(end));
        span->out_count += actual_num;//分给tc多少块，就加多少
        FreeList::obj_next_assign(end, nullptr);
        m_spanlists[idx].getMutex().unlock();
        return actual_num;

    }
    Span *CentralCache::getSpan(int idx, size_t size)
    {
        //在对应的桶中存在有效的span，直接返回
        SpanList& target_spanlist = m_spanlists[idx];
        for (auto it = target_spanlist.begin(); it != target_spanlist.end(); ++it)
        {
            if(!it->free_list.is_empty()){
                return it.getRaw();
            }
        }
        target_spanlist.getMutex().unlock();
        //在对应的桶中，不存在有效的Span
        size_t n_page = SizeAlignCalculator::sizeToPage(size);
        //向PC申请n_page页的Span
        Mutex& pc_mutex = PageCache::getInstance()->getMutex();
        pc_mutex.lock();
        Span *span = PageCache::getInstance()->offerSpan(n_page);
        span->inUse = true;//该span正在cc中使用
        span->obj_size = size;
        pc_mutex.unlock();
        //切分span
        //申请到span后，按照size划分Span，组织成链表形式
        //页号*页大小 = 该页首地址
        uint8_t *start = (uint8_t*)(span->page_id << PAGE_SHIFT);
        uint8_t *end = start + span->n_pages * GET_PAGE_SIZE();
        span->free_list.setFirstObj(start);
        void* tail = start;
        start += size;
        while(start<end)
        {
            FreeList::obj_next_assign(tail, start);
            start+=size;
            tail = FreeList::obj_next_get(tail);
        }
        FreeList::obj_next_assign(tail, nullptr);

        //将切分好的span加入spanlist
        target_spanlist.getMutex().lock();
        target_spanlist.push(span);
        return span;
    }
}

void zhao::CentralCache::collectRangeObj(void *start, size_t size)
{
    //找到对应的桶，把块放里面
    size_t index = SizeAlignCalculator::index(size);
    m_spanlists[index].getMutex().lock();
    while (start)
    {
        void *next = FreeList::obj_next_get(start); 
        //找到该块对应的span
        Span* span = PageCache::getInstance()->mapObjToSpan(start); 
        //插入到span的free_list
        FreeList::obj_next_assign(start, span->free_list.getFirstObj());
        span->free_list.setFirstObj(start);
        span->out_count--;
        //如果tc把一整个span全部还完了，可以考虑提交给PC回收
        if(span->out_count == 0){
            m_spanlists[index].remove(span);
            m_spanlists[index].getMutex().unlock();

            PageCache::getInstance()->getMutex().lock();
            PageCache::getInstance()->collectSpan(span);
            PageCache::getInstance()->getMutex().unlock();

            m_spanlists[index].getMutex().unlock();
        }
        start = next;
    }

    m_spanlists[index].getMutex().unlock();
    return;
}