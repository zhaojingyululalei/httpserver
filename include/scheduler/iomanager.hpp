#ifndef __ZHAO_IOMANAGER_HPP__
#define __ZHAO_IOMANAGER_HPP__
#include "thread/thread.hpp"
// #include "scheduler/scheduler.hpp"
#include <memory>
#include <sys/epoll.h>
#include <stdlib.h>
#include <atomic>
#include <fcntl.h>
#include "fiber/fiber.hpp"
#include "thread/ipc.hpp"
namespace zhao
{
    class Scheduler;
    class IOManager
    {
    public:
        /**
         * @brief IO事件
         */
        enum Event
        {
            /// 无事件
            NONE = 0x0,
            /// 读事件(EPOLLIN)
            READ = 0x1,
            /// 写事件(EPOLLOUT)
            WRITE = 0x4,
        };
        typedef std::shared_ptr<IOManager> ptr;
        IOManager(Scheduler* scheduler);
        virtual ~IOManager();
        void tickle(); // 通知IOManager有任务需要处理
        void contextResize(size_t size);

        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
        bool delEvent(int fd, Event event);
        bool cancelEvent(int fd, Event event);
        bool cancelAll(int fd);
    private:
        struct FdContext
        {
            typedef Mutex MutexType;
            /**
             * @brief 事件上线文类
             */
            struct EventContext
            {
                /// 事件执行的调度器
                Scheduler *scheduler = nullptr;
                /// 事件协程
                Fiber::ptr fiber;
                /// 事件的回调函数
                std::function<void()> cb;
            };

            EventContext &getContext(Event event);
            void resetContext(EventContext &ctx);

            void triggerEvent(Event event);

            /// 读事件上下文
            EventContext read;
            /// 写事件上下文
            EventContext write;
            /// 事件关联的句柄
            int fd = 0;
            /// 当前的事件
            Event events = NONE;
            /// 事件的Mutex
            MutexType mutex;
        };
        void run();
        Thread::ptr m_thread;
        int m_epfd = 0;
        int m_tickleFds[2]; // 用于通知IOManager的管道
        std::vector<FdContext*> m_fdContexts;
        std::atomic<size_t> m_pendingEventCount = {0};
        Scheduler* m_scheduler;
        RWMutex m_mutex;
    };
} // namespace zhao

#endif