#ifndef __IOSCHEDULER_H__
#define __IOSCHEDULER_H__
#include "fiberscheduler.h"
#include "ipc.h"
#include <memory>
#include "timer.h"
namespace zhao
{

    class IOScheduler : public FiberScheduler,public TimerScheduler
    {
    public:
        typedef std::shared_ptr<IOScheduler> ptr;

        enum EventType
        {
            /// 无事件
            NONE = 0x0,
            /// 读事件(EPOLLIN)
            READ = 0x1,
            /// 写事件(EPOLLOUT)
            WRITE = 0x4,
        };

    private:
        struct FdEvent
        {
            struct EventMethod
            {
                FiberScheduler *scheduler = nullptr;
                Fiber::Ptr fiber;
                std::function<void()> cb;
            };

            EventMethod &get(EventType type);
            void reset(EventMethod &em);
            void trigger(EventType type);

            EventMethod read;
            EventMethod write;
            int fd = -1;
            EventType events = NONE;
            Mutex mutex;
        };
    
    public:
        IOScheduler(size_t threads = 1,bool use_caller = true, const std::string& name = "");
        ~IOScheduler();
        int  addEvent(int fd, EventType type, std::function<void()> cb = nullptr);
        bool delEvent(int fd, EventType type);
        bool cancelEvent(int fd, EventType type);
        bool cancelAll(int fd);
        void fdeventResize(size_t size);
        static IOScheduler* getThis();
    protected:
        void tickle() override;
        void idle() override;
        bool stopping() override;
        void onTimerInsertedAtFront() override;
        bool stopping(uint64_t& timeout);
    private:
        int m_epfd = -1;
        int m_pipefd[2];
        std::atomic<size_t> m_pendingEventCount = {0};
        RWMutex m_mutex;
        std::vector<FdEvent *> m_fdEvents;

    };

};

#endif