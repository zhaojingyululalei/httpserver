// test_thread.cpp
#include "thread/thread.hpp"
#include "thread/ipc.hpp"
#include <iostream>
#include <unistd.h>
#include <functional>
using namespace zhao;

static Mutex mutex;
static void func1(int x)
{
    Mutex::MutexLockGuardType lock(mutex);
    std::cout << "Hello from thread: " << Thread::getCurThreadName() 
              << " with tid: " << Thread::getCurThreadId() 
              <<" arg:" << x 
              << std::endl;
    sleep(1);
    std::cout << "Thread " << Thread::getCurThreadName() << " finishing..." << std::endl;
}
void func2() {
    Mutex::MutexLockGuardType lock(mutex);
    std::cout << "Hello from thread: " << Thread::getCurThreadName() 
              << " with tid: " << Thread::getCurThreadId()
              << std::endl;
    sleep(1);
    std::cout << "Thread " << Thread::getCurThreadName() << " finishing..." << std::endl;
}
const std::string getTestName(const std::string name) {
    return name;
}
static void test01()
{
    std::cout << "Main thread name: " << Thread::getCurThreadName() 
    << " Main thread id: " << Thread::getCurThreadId()
     << std::endl;
   
    try {
        // 测试创建线程
        // Thread thread1(func2, "TestThread1");
        // Thread thread2(func2, "TestThread2");
        
        Thread thread1(std::bind(func1, 10),"TestThread1");
        Thread thread2(std::bind(func1, 20),"TestThread2");

        std::cout << "Threads created, waiting for completion..." << std::endl;
        
        // 等待线程完成
        thread1.join();
        thread2.join();
        
        std::cout << "All threads completed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return ;
    }
}
#include "log/logger.hpp"
static void log_func(void)
{
    for(int i=0;i<3;++i){

        MODULE_DEBUG("world", LOGGER_GET("system"))<<"nihao";
    }
}
#include <assert.h>
static void test02()
{
    //assert(false);
    Thread thread1(&log_func,"thread1");
    Thread thread2(&log_func,"thread2");
    Thread thread3(&log_func,"thread3");
    Thread thread4(&log_func,"thread4");
    Thread thread5(&log_func,"thread5");
    Thread thread6(&log_func,"thread6");

    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();
    thread5.join();
    thread6.join();


}
void thread_test() {
    
    test02();
    return ;
}