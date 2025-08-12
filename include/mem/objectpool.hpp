#ifndef __ZHAO_OBJECTPOOL_HPP__
#define __ZHAO_OBJECTPOOL_HPP__

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <cassert>
#include "thread/ipc.hpp"
#include "utils/utils.hpp"
#include "mem/memdef.hpp"
#include "mem/common.hpp"
namespace zhao
{

    /*注：对象前8字节存储next对象的首地址*/
    template <class T>
    class ObjectPool
    {
    public:
        T *_new()
        {
            Mutex::MutexLockGuardType lock(m_mutex);
            T *obj = nullptr;
            // 有回收到的T对象，可以重复利用
            if (!m_freelist.is_empty())
            {
                void *first_obj = m_freelist.pop();
                obj = static_cast<T *>(first_obj);
            }
            else // freelist为空,去memory上分配
            {
                // 如果剩余空间不够T对象，就扩容memory，否则不用扩容
                if (m_remain < sizeof(T))
                {
                    m_remain = EXPAND_SIZE;
                    m_memory = (uint8_t *)mmap(nullptr, m_remain, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                    if (m_memory == MAP_FAILED)
                    {
                        throw std::bad_alloc();
                    }
                    assert(((uintptr_t)m_memory % GET_PAGE_SIZE()) == 0);
                }
                // 获取T对象地址
                obj = (T *)m_memory;
                assert(obj != nullptr);
                size_t obj_size = sizeof(T) < sizeof(void *) ? sizeof(void *) : sizeof(T); // 判断一下T的大小，小于指针就给一个指针大小，大于指针就还是T的大小
                m_memory += obj_size;                                                      // m_memory向后移动obj_size
                m_remain -= obj_size;
            }
            new (obj) T; // 定位new 在已经分配的内存地址 obj 上构造一个 T 类型的对象， 触发构造函数来初始化对象，不会去堆分配内存
            return obj;
        }
        /**
         * @brief 数组分配功能
         */
        T *_new_array(size_t count)
        {
            Mutex::MutexLockGuardType lock(m_mutex);
            size_t total_size = count * sizeof(T);
            if (m_remain < total_size)
            {
                
                m_remain =(total_size / EXPAND_SIZE +1) * EXPAND_SIZE;
                m_memory = (uint8_t *)mmap(nullptr, m_remain, PROT_READ | PROT_WRITE,
                                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                if (m_memory == MAP_FAILED)
                {
                    std::cout <<"total_size= "<<total_size << " m_remain= "<<m_remain << std::endl;
                    throw std::bad_alloc();
                }
            }

            T *array = reinterpret_cast<T *>(m_memory);
            m_memory += total_size;
            m_remain -= total_size;

            // 初始化数组元素（对于指针数组不需要构造）
            if constexpr (!std::is_pointer_v<T>)
            {
                for (size_t i = 0; i < count; ++i)
                {
                    new (&array[i]) T();
                }
            }

            return array;
        }
        void _delete(T *obj)
        {
            Mutex::MutexLockGuardType lock(m_mutex);
            obj->~T();
            // 插入freelist中,头插
            m_freelist.push((void *)obj);
        }

    private:
        Mutex m_mutex;
        uint8_t *m_memory = nullptr;
        FreeList m_freelist;
        // void *m_freelist = nullptr; //指向第一个obj对象(T类型首地址)
        size_t m_capacity = 0; // 内存池容量
        size_t m_remain = 0;   // 剩余容量
    };

} // namespace zhao

#endif