#include "mem/common.hpp"
#include "common.hpp"
#include "mem/objectpool.hpp"
namespace zhao
{

    void *FreeList::obj_next_get(void *obj)
    {
        bool ret;
        if (!obj)
            return nullptr;
        uintptr_t next_addr = 0;
        ret = Phaddr<uintptr_t>::readFromPhysical(obj, next_addr);
        assert(ret);
        return (void *)next_addr;
    }
    /**
     * @brief 设置对象下一个对象
     */
    void FreeList::obj_next_assign(void *obj, void *next)
    {
        bool ret;
        if (!obj)
            return;

        uintptr_t next_addr = (uintptr_t)next;
        ret = Phaddr<uintptr_t>::writeToPhysical(obj, (uintptr_t)next_addr);
        assert(ret);
    }
    void FreeList::push(void *obj)
    {
        obj_next_assign(obj, m_freelist);
        m_freelist = obj;
        m_size++;
    }
    void FreeList::pushRange(void *start, void *end, size_t cnt)
    {

        obj_next_assign(end, m_freelist);
        m_freelist = start;
        m_size += cnt;
    }
    void FreeList::popRange(void *&start, void *&end, size_t cnt)
    {
        assert(m_size >= cnt);
        start = end = m_freelist;
        for (size_t i = 0; i < cnt - 1; i++)
        {
            end = obj_next_get(end);
        }
        m_freelist = obj_next_get(end);
        obj_next_assign(end, nullptr);
        m_size -= cnt;
    }
    /**
     * @brief 从freelist中获取一个空闲对象
     */
    void *FreeList::pop()
    {
        void *obj = nullptr;
        void *first_obj = m_freelist;
        obj = first_obj;
        // 把first_obj剔除链表
        void *next = obj_next_get(first_obj);
        m_freelist = next;
        m_size--;
        return obj;
    }
    /**
     * @brief 判断freelist是否为空
     */
    bool FreeList::is_empty()
    {
        return m_freelist == nullptr;
    }

    int FreeList::req_num_from_cc(size_t size)
    {
        m_token_bucket.try_consume(); // 申请令牌
        int tokens = m_token_bucket.get_tokens();
        int num = SizeAlignCalculator::sizeToNum(size);
        return std::min(num, tokens);
    }
    void *FreeList::getFirstObj()
    {
        return m_freelist;
    }
    void FreeList::setFirstObj(void *obj)
    {
        m_freelist = obj;
    }
    size_t SizeAlignCalculator::RoundUp(size_t size)
    {
        if (size <= 128)
        { // [1,128] 8B
            return Utils::Math::alignUp(size, 8);
        }
        else if (size <= 1024)
        { // [128+1,1024] 16B
            return Utils::Math::alignUp(size, 16);
        }
        else if (size <= 8 * 1024)
        { // [1024+1,8*1024] 128B
            return Utils::Math::alignUp(size, 128);
        }
        else if (size <= 64 * 1024)
        { // [8*1024+1,64*1024] 1024B
            return Utils::Math::alignUp(size, 1024);
        }
        else if (size <= 256 * 1024)
        { // [64*1024+1,256*1024] 8 * 1024B
            return Utils::Math::alignUp(size, 8 * 1024);
        }
        else
        { // 单次申请空间大于256KB，直接按照页来对齐
            return Utils::Math::alignUp(size, 1 << PAGE_SHIFT);
        }
    }
    // size_t SizeAlignCalculator::index(size_t size)
    // {
    //     assert(size <= TC_ALLOC_BYTES_MAX_SIZE);

    //     // 每个区间有多少个链
    //     static int group_array[4] = {16, 56, 56, 56};
    //     if (size <= 128)
    //     {                           // [1,128] 8B -->8B就是2^3B，对应二进制位为3位
    //         return _Index(size, 3); // 3是指对齐数的二进制位位数，这里8B就是2^3B，所以就是3
    //     }
    //     else if (size <= 1024)
    //     { // [128+1,1024] 16B -->4位
    //         return _Index(size - 128, 4) + group_array[0];
    //     }
    //     else if (size <= 8 * 1024)
    //     { // [1024+1,8*1024] 128B -->7位
    //         return _Index(size - 1024, 7) + group_array[1] + group_array[0];
    //     }
    //     else if (size <= 64 * 1024)
    //     { // [8*1024+1,64*1024] 1024B -->10位
    //         return _Index(size - 8 * 1024, 10) + group_array[2] + group_array[1] + group_array[0];
    //     }
    //     else if (size <= 256 * 1024)
    //     { // [64*1024+1,256*1024] 8 * 1024B  -->13位
    //         return _Index(size - 64 * 1024, PAGE_SHIFT) + group_array[3] +
    //                group_array[2] + group_array[1] + group_array[0];
    //     }
    //     else
    //     {
    //         assert(false);
    //     }
    //     return -1;
    // }
    size_t SizeAlignCalculator::index(size_t size)
    {
        assert(size <= TC_ALLOC_BYTES_MAX_SIZE);

        // 每个区间有多少个链
        static int group_array[5] = {16, 56, 56, 56, 24}; // 注意这里改为5个区间

        if (size <= 128)
        {                           // [1,128] 8B -->8B就是2^3B，对应二进制位为3位
            return _Index(size, 3); // 3是指对齐数的二进制位位数，这里8B就是2^3B，所以就是3
        }
        else if (size <= 1024)
        { // [129,1024] 16B -->4位
            return _Index(size - 128, 4) + group_array[0];
        }
        else if (size <= 8 * 1024)
        { // [1025,8*1024] 128B -->7位
            return _Index(size - 1024, 7) + group_array[1] + group_array[0];
        }
        else if (size <= 64 * 1024)
        { // [8KB+1,64KB] 1024B -->10位
            return _Index(size - 8 * 1024, 10) + group_array[2] + group_array[1] + group_array[0];
        }
        else if (size <= 256 * 1024)
        { // [64KB+1,256KB] 8KB -->13位 (实际是PAGE_SHIFT)
            return _Index(size - 64 * 1024, 13) + group_array[3] +
                   group_array[2] + group_array[1] + group_array[0];
        }
        else
        {
            assert(false);
        }
        return -1;
    }

    ObjectPool<Span> span_pool;
    SpanList::SpanList()
    {
        m_head = span_pool._new();
        m_head->next = m_head;
        m_head->pre = m_head;
    }

    void SpanList::insert(Span *pos, Span *span)
    {
        assert(pos);
        assert(span);
        Span *pre = pos->pre;

        // 设置span的前后指针
        span->next = pos;
        span->pre = pre;

        // 更新前一个节点的next指针
        pre->next = span;

        // 更新pos节点的pre指针
        pos->pre = span;
    }
    void SpanList::remove(Span *span)
    {
        assert(span);

        // 获取前驱和后继节点
        Span *pre = span->pre;
        Span *next = span->next;

        // 更新前驱节点的next指针
        pre->next = next;

        // 更新后继节点的pre指针
        next->pre = pre;

        // 可选：清理span节点的指针
        span->pre = nullptr;
        span->next = nullptr;
        span->free_list.setFirstObj(nullptr);
    }

    void SpanList::push(Span *span)
    {
        // 将span插入到m_head和m_head->next之间（头插法）
        Span *first = m_head->next;

        // 设置span的前后指针
        span->next = first;
        span->pre = m_head;

        // 更新m_head的next指针
        m_head->next = span;

        // 更新原来第一个节点的pre指针
        first->pre = span;
    }
    Span *SpanList::pop()
    {
        if (is_empty())
        {
            return nullptr;
        }
        Span *ret = m_head->next;
        remove(m_head->next);
        return ret;
    }
    bool SpanList::is_empty()
    {
        return m_head->next == m_head;
    }
    size_t SizeAlignCalculator::sizeToNum(size_t size)
    {
        int num = TC_ALLOC_BYTES_MAX_SIZE / size;
        // 小对象，例如8B
        if (num > TC_REQUEST_FROM_CC_MAX_SIZE)
        {
            num = TC_REQUEST_FROM_CC_MAX_SIZE;
        }
        // 大对象，例如256KB
        if (num < 2)
        {
            num = 2;
        }
        return num;
    }

    size_t SizeAlignCalculator::sizeToPage(size_t size)
    {
        // TC向CC申请size大小的块时，单次申请的块数
        size_t num = sizeToNum(size);
        size_t n_page = num * size;
        n_page >>= PAGE_SHIFT;
        if (n_page == 0)
        {
            n_page = 1;
        }
        return n_page;
    }

} // namespace zhao
