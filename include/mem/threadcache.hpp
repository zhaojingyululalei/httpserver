#ifndef __ZHAO_THREADCACHE_H__
#define __ZHAO_THREADCACHE_H__
#include "mem/common.hpp"

namespace zhao
{
    class ThreadCache
    {
        public:
        /**
         * @brief 申请内存
         */
        void *malloc(size_t size);
        /**
         * @brief 释放内存
         */
        void free(void *obj, size_t size);
        /**
         * @brief TC空间不够，向CC申请更多
         * @param align_size 申请内存块大小
         * @param index 桶索引 TC 和 CC 桶索引含义相同
         */
        void *requestFromCentralCache(size_t align_size, size_t index);
        /**
         * @brief 向CC归还内存块
         */
        void releaseToCentralCache(int index, size_t align_size);
        
        private:
        FreeList m_freelists[TC_FREELIST_SIZE];

        
    };
    extern thread_local ThreadCache *t_thread_cache;

} // namespace zhao

#endif