#include "thread/thread.hpp"
#include "thread.hpp"
#include <iostream>
namespace zhao
{
    static thread_local Thread *t_thread = nullptr;
    /**
     * Thread对象都销毁了，m_thread还在，那么就交给操作系统回收
     */
    Thread::~Thread()
    {
        if(m_thread){
            pthread_detach(m_thread);
        }
    }
    Thread::Thread(std::function<void()> cb, const std::string &name)
    {
        int ret;
        if(name.empty()){
            m_name = "UNKNOWN";
        }else{
            m_name = name;
        }
        m_cb = cb;
        ret = pthread_create(&m_thread, nullptr, &Thread::run, this);
        if(ret!=0)
        {
            std::cout<<"pthread_create error"<<std::endl;
            throw std::logic_error("pthread_create error");
        }
        m_sem.wait(); //  创建线程的主线程等着 该线程初始化完毕
    }
    void *Thread::run(void *arg)
    {
        Thread* thread = static_cast<Thread*>(arg);
        t_thread = thread;
        thread->m_tid = Utils::getCurrentThreadId();
        pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

        std::function<void()> cb;
        cb.swap(thread->m_cb);
        thread->m_sem.notify();

        cb();
        return nullptr;
    }
    void Thread::join()
    {
        int ret;
        if(m_thread){
            ret = pthread_join(m_thread, nullptr);
            if(ret !=0)
            {
                //std::cout<<"pthread_join error"<<std::endl;
                throw std::logic_error("pthread_join error");
            }
            m_thread = 0;
            t_thread = nullptr;
        }
    }
    Thread *Thread::getThis()
    {
        return t_thread;
    }

    const std::string& Thread::getCurThreadName()
    {
        static std::string main_name = "main";
        if(t_thread)
        {
            return t_thread->m_name;
        }else{
            return main_name;
        }
    }

    const tid_t Thread::getCurThreadId()
    {
        if(t_thread){
            return t_thread->m_tid;
        }else{
            return Utils::getCurrentThreadId();
        }
    }
}
