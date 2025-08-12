#ifndef __ZHAO_PAGECACHE_HPP__
#define __ZHAO_PAGECACHE_HPP__
#include "mem/common.hpp"
#include "thread/ipc.hpp"
#include "utils/singleton.hpp"
#include "utils/noncopyable.hpp"
#include "utils/hashmap.hpp"
namespace zhao
{
    class PageCache :public Noncopyable
    {
    public:
        static PageCache* getInstance()
        {
            return Singleton<PageCache>::getInstance();
        }
        /**
         * @brief PC向CC提供一个K页大小的Span
         */
        Span* offerSpan(int n_page);
        Mutex& getMutex(){
            return m_mutex;
        }
        /**
         * @brief 给一个obj对象，找到它所在的span。原理是obj>>PageShift获取页号，然后通过映射获取span
         */
        Span* mapObjToSpan(void* obj);
        /**
         * @brief 回收CC中的Span，要合并，A合并到B，A需要objectpool释放
         */
        void collectSpan(Span* span);
    private:
        SpanList m_spanlists[PC_SPANLIST_SIZE];
        HashMap<pageid_t, Span*> m_spanmap;
        Mutex m_mutex;
    };
} // namespace zhao

#endif