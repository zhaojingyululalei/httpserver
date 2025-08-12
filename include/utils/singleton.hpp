#ifndef __ZHAO_SINGLETON_H__
#define __ZHAO_SINGLETON_H__
#include <memory>
namespace zhao
{
    template <class T>
    class Singleton
    {
    public:
        static T *getInstance()
        {
            static T t;
            return &t;
        }
    };

    template <class T>
    class SingletonPtr
    {
        public:
            static std::shared_ptr<T> getInstance()
            {
                static std::shared_ptr<T> t(new T);
                return t;
            }
    };
} // namespace zhao


#endif