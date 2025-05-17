#ifndef __IPC_H
#define __IPC_H
#include<stdint.h>
#include<semaphore.h>
#include <atomic>
namespace zhao
{
    class Semphore
    {
    public:
        Semphore(uint32_t count = 0);
        ~Semphore();
        void wait();
        void notify();  
        int tryWait();
        int timewait(uint32_t ms);

    private:
        Semphore(const Semphore &) = delete;
        Semphore(const Semphore &&) = delete;
        Semphore& operator=(const Semphore &) = delete;
        sem_t m_sem;
    };


    template<class T>
    class LockGuard
    {
    public:
        LockGuard(T &mutex)
            :m_mutex(mutex)
        {
            lock();
        }
        ~LockGuard()
        {
            unlock();
        }
        void lock(){
            if(!m_locked){
                m_mutex.lock();
                m_locked = true;
            }
        }
        void unlock(){
            if(m_locked){
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        T m_mutex;
        
        bool m_locked;
    };

    class Mutex
    {
    public:
        typedef LockGuard<Mutex> MutexLockGuardType;
        Mutex(){
            pthread_mutex_init(&m_mutex, nullptr);
        }
        ~Mutex()
        {
            pthread_mutex_destroy(&m_mutex);
        }
        void lock()
        {
            pthread_mutex_lock(&m_mutex);
        }

        void unlock()
        {
            pthread_mutex_unlock(&m_mutex);
        }

    private:
        pthread_mutex_t m_mutex;
    };

    class SpinMutex
    {
    public:
        typedef LockGuard<SpinMutex> SpinLockGuardType;
        SpinMutex(){
            pthread_spin_init(&m_mutex, PTHREAD_PROCESS_PRIVATE);
        }
        ~SpinMutex()
        {
            pthread_spin_destroy(&m_mutex);
        }
        void lock()
        {
            pthread_spin_lock(&m_mutex);
        }
        void unlock()
        {
            pthread_spin_unlock(&m_mutex);
        }

    private:
        pthread_spinlock_t m_mutex;
    };


    template<class T>
    class ReadLockGuard
    {
    public:
        ReadLockGuard(T &mutex)
            :m_mutex(mutex)
        {
            lock();
        }
        ~ReadLockGuard()
        {
            unlock();
        }
        void lock(){
            if(!m_locked){
                m_mutex.rdlock();
                m_locked = true;
            }
        }
        void unlock(){
            if(m_locked){
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        T m_mutex;
        bool m_locked;
    };
    template<class T>
    class WriteLockGuard
    {
    public:
        WriteLockGuard(T &mutex)
            :m_mutex(mutex)
        {
            lock();
        }
        ~WriteLockGuard()
        {
            unlock();
        }
        void lock(){
            if(!m_locked){
                m_mutex.wrlock();
                m_locked = true;
            }
        }
        void unlock(){
            if(m_locked){
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        T m_mutex;
        bool m_locked;
    };
    class RWMutex
    {
    public:
        typedef ReadLockGuard<RWMutex> ReadLockGuardType;
        typedef WriteLockGuard<RWMutex> WriteLockGuardType;
        RWMutex(){
            pthread_rwlock_init(&m_lock, nullptr);
        }
        ~RWMutex()
        {
            pthread_rwlock_destroy(&m_lock);
        }
        void rdlock()
        {
            pthread_rwlock_rdlock(&m_lock);
        }
        void wrlock()
        {
            pthread_rwlock_wrlock(&m_lock);
        }
        void unlock()
        {
            pthread_rwlock_unlock(&m_lock);
        }
    private:
        pthread_rwlock_t m_lock;
    };

} // namespace zhao


#endif