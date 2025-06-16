#ifndef __FIBERSCHEDULER_H__
#define __FIBERSCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include <functional>
#include <atomic>
#include "thread.h"
#include "ipc.h"
#include "fiber.h"
namespace zhao
{
    class FiberScheduler
    {
    public:
        typedef std::shared_ptr<FiberScheduler> Ptr;
        FiberScheduler(uint32_t thread_num = 1,bool use_caller = false,const std::string& name = "");
        virtual ~FiberScheduler();
        const std::string& getName() const;
        void start();
        void stop();
        template<class F>
        void join(F f,tid_t threadId = -1){ 
            if(f == nullptr){
                return;
            }
            
            bool need_tickle = false;
            {
                Mutex::MutexLockGuardType lock(m_mutex);
                if(m_fibers.empty()){
                    need_tickle = true;
                }
                FiberDefine fd(f,threadId);
                m_fibers.push_back(fd);
            }
            if(need_tickle){
                tickle();
            }

        }
        template<class STLIterator>
        void join(STLIterator begin,STLIterator end,tid_t threadId = -1){
            if(begin == end){
                return;
            }
            
            bool need_tickle = false;
            {
                Mutex::MutexLockGuardType lock(m_mutex);
                if(m_fibers.empty()){
                    need_tickle = true;
                }
                for(;begin != end;begin++){
                    FiberDefine fd(*begin,threadId);//用的是swap
                    m_fibers.push_back(fd);
                }
            }
            if(need_tickle){
                tickle();
            }
        }
        static FiberScheduler* getThis();
        static void setThis(FiberScheduler* sched);
        //获取负责调度的协程
        static Fiber* getMainFiber();
    protected:
        virtual void tickle();
        virtual bool stopping();
        virtual void idle();
        void run();
        size_t  m_thread_num = 0;
        std::atomic<size_t> m_activated_thread_num = {0};
        std::atomic<size_t> m_idle_thread_num = {0};
        bool m_stopping = true;
        bool m_autostop=  false;
        tid_t m_use_caller_threadId = -1;
        Fiber::Ptr mp_use_caller_fiber = nullptr;
        std::vector<tid_t> m_threadIds;
    private:
        struct FiberDefine{
            Fiber::Ptr m_fiber;
            std::function<void()> m_cb;
            tid_t m_threadId;

            FiberDefine():m_threadId(-1){
                ;
            }
            FiberDefine(Fiber::Ptr f,tid_t threadId):m_fiber(f),m_threadId(threadId){
                ;
            }
            FiberDefine(Fiber::Ptr* f,tid_t threadId):m_threadId(threadId){
                // 外面的智能指针引用计数为0
                m_fiber.swap(*f);
            }
            FiberDefine(std::function<void()> cb,tid_t threadId):m_cb(cb),m_threadId(threadId){
                ;
            }
            FiberDefine(std::function<void()>* cb,tid_t threadId):m_threadId(threadId){
                m_cb.swap(*cb);
            }
            void reset(void){
                m_fiber = nullptr;
                m_cb = nullptr;
                m_threadId = -1;
            }
        };
        std::string m_name;
        Mutex m_mutex;
        std::vector<Thread::Ptr> m_threads;//线程池
        std::list<FiberDefine> m_fibers; //任务队列
    };
} // namespace zhao

#endif