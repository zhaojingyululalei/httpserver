#ifndef __THREAD_H__
#define __THREAD_H__ 
#include <pthread.h>
#include <memory>
#include <thread>
#include <functional>
#include <string>
#include "utils.h"
namespace zhao
{
    
    
    class Thread
    {
    public:
        typedef std::shared_ptr<Thread> Ptr;
        typedef std::function<void(void*)> ThreadFunc;
        Thread(ThreadFunc func, const std::string &name,void*arg);
        ~Thread();

        
        void join();
        const std::string getName(){
            return m_name;
        }
        void setName(const std::string &name)
        {
            m_name = name;
        }
        tid_t getTid() const{
            return m_tid;
        }
        void setTid(tid_t tid){
            m_tid = tid;
        }
        pthread_t getPthread() const{
            return m_thread;
        }
        
        static  Thread* getCur();
        static const std::string getCurName();
        static void setCurName(const std::string &name);

    private:
        Thread(const Thread &) = delete;
        Thread(const Thread &&) = delete;
        Thread& operator=(const Thread &) = delete;
        static void* run(void* arg);
    private:
        pthread_t m_thread;
        tid_t m_tid;
        std::string m_name;
        ThreadFunc m_func;
        void* m_arg;

    };
} // namespace zhao


#endif