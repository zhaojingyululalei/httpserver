#ifndef __ZHAO_THREAD_H
#define __ZHAO_THREAD_H 

#include <string>
#include <functional>
#include <memory>
#include <pthread.h>
#include "utils/noncopyable.hpp"
#include "utils/utils.hpp"
#include "ipc.hpp"
namespace zhao
{
    class Thread : public Noncopyable
    { 
        public:
            typedef std::shared_ptr<Thread> ptr;
            Thread() = default;
            ~Thread() ;
            Thread(std::function<void()> cb,const std::string& name);
            void join();
            
            static Thread* getThis();
            static const std::string& getCurThreadName();
            static const tid_t getCurThreadId();
            tid_t getId() const {return m_tid;}
            const std::string& getName() const{return m_name;}
            void setName(const std::string& name){m_name = name;}
            static void* run(void* arg);
        private:
            std::string m_name;
            tid_t m_tid;
            pthread_t m_thread;
            std::function<void()> m_cb;
            Semphore m_sem;

    };
    
} // namespace zhao


#endif