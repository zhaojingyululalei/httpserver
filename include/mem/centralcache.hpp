#ifndef __ZHAO_CENTRALCACHE_HPP__
#define __ZHAO_CENTRALCACHE_HPP__
#include "mem/common.hpp"
#include "thread/ipc.hpp"
#include "utils/noncopyable.hpp"
#include "utils/singleton.hpp"
#include "mem/memdef.hpp"
namespace zhao { 
    class CentralCache : public Noncopyable {
    public:
        static CentralCache* getInstance()
        {
            return Singleton<CentralCache>::getInstance();
        }
        /**
         * @brief 从CentralCache中申请内存块,具体就是从Span结构中取
         * @param[out] start 申请到的内存块起始地址
         * @param[out] end 申请到的内存块结束地址
         * @param[in] batch_num 申请的块数
         * @param[in] align_size 申请的块大小 
         * @return 实际申请到的块数，不一定等于batch_num
         */
        size_t offerRangeObj(void* &start,void* &end,size_t batch_num,size_t align_size);
        /**
         * @brief 找个空闲span，没有就向PC申请，然后返回Span
         */
        Span* getSpan(int idx, size_t size);
        /**
         * @brief 回收TC传来的空闲块(obj)
         */
        void collectRangeObj(void* start,size_t size);
    private:
        SpanList m_spanlists[TC_SPANLIST_SIZE];
    };
}

#endif  