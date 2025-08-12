#ifndef __ZHAO_COMMON_H__
#define __ZHAO_COMMON_H__
#include <stdint.h>
#include <stddef.h>
#include <cassert>
#include <iostream>
#include <string.h>
#include "utils/utils.hpp"
#include "thread/ipc.hpp"
#include "utils/algrithm.hpp"
#include "mem/memdef.hpp"
namespace zhao
{


    typedef size_t pageid_t;
    /**
     * @brief 读写某地址中的内容
     */
    template <class T>
    class Phaddr
    {
    public:
        static bool writeToPhysical(void *phaddr, void *buf, size_t size)
        {
            if (!phaddr || !buf || size == 0)
            {
                return false;
            }

            memcpy(phaddr, buf, size);
            return true;
        }

        static bool writeToPhysical(void *phaddr, const T &val)
        {
            if (!phaddr)
            {
                return false;
            }

            // 将T类型的值写入物理地址
            memcpy(phaddr, &val, sizeof(T));
            return true;
        }
        // 读取系列函数
        static bool readFromPhysical(void *phaddr, void *buf, size_t size)
        {
            if (!phaddr || !buf || size == 0)
            {
                return false;
            }

            memcpy(buf, phaddr, size);
            return true;
        }

        static bool readFromPhysical(void *phaddr, T &val)
        {

            if (!phaddr)
            {
                return false;
            }

            // 从物理地址读取T类型的值
            memcpy(&val, phaddr, sizeof(T));
            return true;
        }
    };

    class FreeList
    {
    public:
        static void *obj_next_get(void *obj);
        /**
         * @brief 设置对象下一个对象
         */
        static void obj_next_assign(void *obj, void *next);

    public:
        FreeList() : m_token_bucket(10, 30, TC_REQUEST_FROM_CC_MAX_SIZE)
        {
        }
        /**
         * @brief 插入一个对象
         */
        void push(void *obj);
        /**
         * @brief 插入一连串对象,这里Span中已经切分好了，这片内存都是链表的形式
         */
        void pushRange(void *start, void *end,size_t cnt);
        /**
         * @brief 拿出一连串对象
         */
        void popRange(void *&start, void *&end,size_t cnt);
        /**
         * @brief 从freelist中获取一个空闲对象
         */
        void *pop();
        /**
         * @brief 判断freelist是否为空
         */
        bool is_empty();
        /**
         * @brief 该桶没有大小为size的块了，该向CC申请多少块
         */
        int req_num_from_cc(size_t size);
        /**
         * @brief 获取第一个obj
         */
        void *getFirstObj();
        /**
         * @brief 设置第一个obj
         */
        void setFirstObj(void *obj);
        size_t size() const {return m_size;};
        /**
         * @breif 获取令牌数量
         */
        size_t get_tokens(){
            return m_token_bucket.get_tokens();
        }
        size_t get_max_tokens(){
            return m_token_bucket.get_max_tokens();
        }
        DynamicTokenBucket&  get_token_bucket(){
            return m_token_bucket;
        }
        size_t m_total;//包含线程正在使用的，和freelist中存留的总和(总共向CC申请了多少块)
    private:
        void *m_freelist = nullptr;
        DynamicTokenBucket m_token_bucket; // 令牌桶算法，防止过度申请资源
        size_t m_size; //当前链表节点数
        
    };

    class SizeAlignCalculator
    {
        // 线程申请size的对齐规则：整体控制在最多10%左右的内碎片浪费
        //	size范围				对齐数				对应哈希桶下标范围
        // [1,128]					8B 对齐      		freelist[0,16)
        // [128+1,1024]			16B 对齐  			freelist[16,72)
        // [1024+1,8*1024]			128B 对齐  			freelist[72,128)
        // [8*1024+1,64*1024]		1024B 对齐    		freelist[128,184)
        // [64*1024+1,256*1024]	8*1024B 对齐  		freelist[184,208)

    public:
        /**
         * @brief 将要分配的内存大小对齐
         */
        static size_t RoundUp(size_t size);
        /**
         * @brief 计算去哪个桶获取空闲内存
         */
        static size_t index(size_t size);
        /**
         * @brief TC向CC申请size大小的块，CC一次性给TC多少个(避免频繁分配)
         */
        static size_t sizeToNum(size_t size);
        /**
         * @brief TC向CC申请size大小的块，CC没有，于是向PC申请管理多大页的Span合适?
         */
        static size_t sizeToPage(size_t size);

    private:
        static inline size_t _Index(size_t size, size_t align_shift)
        { /*这里align_shift是指对齐数的二进制位数。比如size为2的时候对齐数
          为8，8就是2^3，所以此时align_shift就是3*/
            return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
            // 这里_Index计算的是当前size所在区域的第几个下标，所以Index的返回值需要加上前面所有区域的哈希桶的个数
        }
    };

    struct Span
    {
        pageid_t page_id = 0; // 该span的起始页号
        size_t n_pages = 0;   // 该span管理的页数
        FreeList free_list;   // Span切分成若干块
        size_t out_count = 0;
        Span *pre = nullptr;
        Span *next = nullptr;
        size_t obj_size = 0;//该span被切分成若干obj_size大小的节点，连成链表
        bool inUse = false; //true,该span在CC中 ；false，该span在PC中
    };
    /**
     * @brief Span双向链表，对于CC来讲，每个桶就是SpanList
     */
    class SpanList
    {
    public:
        // 迭代器类
        class iterator
        {
        public:
            iterator(Span *span) : m_span(span) {}

            // 前置递增运算符
            iterator &operator++()
            {
                m_span = m_span->next;
                return *this;
            }

            // 后置递增运算符
            iterator operator++(int)
            {
                iterator temp = *this;
                m_span = m_span->next;
                return temp;
            }

            // 前置递减运算符
            iterator &operator--()
            {
                m_span = m_span->pre;
                return *this;
            }

            // 后置递减运算符
            iterator operator--(int)
            {
                iterator temp = *this;
                m_span = m_span->pre;
                return temp;
            }

            // 解引用运算符
            Span &operator*() const
            {
                return *m_span;
            }

            // 成员访问运算符
            Span *operator->() const
            {
                return m_span;
            }

            // 相等比较运算符
            bool operator==(const iterator &other) const
            {
                return m_span == other.m_span;
            }

            // 不等比较运算符
            bool operator!=(const iterator &other) const
            {
                return m_span != other.m_span;
            }
            Span *getRaw()
            {
                return m_span;
            }

        private:
            Span *m_span;
        };

        // 返回起始迭代器
        iterator begin()
        {
            return iterator(m_head->next);
        }

        // 返回结束迭代器
        iterator end()
        {
            return iterator(m_head);
        }

        // 返回起始const迭代器
        iterator begin() const
        {
            return iterator(m_head->next);
        }

        // 返回结束const迭代器
        iterator end() const
        {
            return iterator(m_head);
        }

    public:
        SpanList();
        /**
         * @brief 插到pos前面
         */
        void insert(Span *pos, Span *span);
        /**
         * @brief 删除span
         */
        void remove(Span *span);
        void push(Span *span);
        Span* pop();
        bool is_empty();
        Mutex &getMutex()
        {
            return m_mutex;
        }

    private:
        Span *m_head = nullptr;
        Mutex m_mutex; // 对于CC来讲，每个桶都有一个锁
    };
    
} // namespace zhao

#endif