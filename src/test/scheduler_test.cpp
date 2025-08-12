#include "scheduler/scheduler.hpp"
#include "utils/utils.hpp"
#include <vector>
#include <iostream>
#include <functional>
#include <assert.h>
#include <stdlib.h>
#define MODULE_NAME "scheduler"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))

static void test01()
{
//     std::vector<zhao::Worker::ptr> workers;
//     zhao::Worker::ptr worker1(new zhao::Worker("test01_1"));
//    zhao::Worker::ptr worker2(new zhao::Worker("test01_2"));
//     zhao::Worker::ptr worker3(new zhao::Worker("test01_3"));
//     workers.push_back(worker1);
//    workers.push_back(worker2);
//     workers.push_back(worker3);
//     for (size_t i = 0; i < workers.size(); i++)
//     {
//         std::cout << "worker name: " << workers[i]->getName() << std::endl;
//     }
    
}
static void test_func()
{
    std::cout << "fiber_id: "<< zhao::Utils::getFiberId() << std::endl;
}
static void test02()
{
    zhao::Scheduler::ptr scheduler(new zhao::Scheduler(2));
    scheduler->start();
    for (size_t i = 0; i < 10; i++)
    {
        scheduler->schedule(test_func);
    }
    scheduler->stop();
}
static void test03()
{
    zhao::Scheduler::ptr scheduler(new zhao::Scheduler(2));
    scheduler->start();

    std::vector<std::function<void()>> funcs;
    for (size_t i = 0; i < 10; ++i) {
        funcs.push_back([]() {
            std::cout << "fiber_id: " << zhao::Utils::getFiberId() << " from batch schedule" << std::endl;
        });
    }

    int j = 0;
    srand(time(nullptr)); // 初始化随机数种子
    
    while(j < 10)
    {
        if(j%2 == 0) 
        {
            scheduler->schedule(test_func);
        }
        else
        {
            scheduler->schedule(funcs.begin(), funcs.end());
        }
        int rand_time = rand() % 5 + 1; // 1~5秒
        sleep(rand_time); // 随机延时
        j++;
    }

    scheduler->stop();
}


static void test04() {
    zhao::Scheduler::ptr scheduler(new zhao::Scheduler(2));
    scheduler->start();

    // 创建一个pipe用于测试
    int fds[2];
    assert(!pipe(fds));

    // 测试添加读事件
    scheduler->addEvent(fds[0], zhao::IOManager::READ, [](){
        std::cout << "read event triggered" << std::endl;
    });

    
    #include <atomic>
    // 添加写事件但不让它立即执行
    std::atomic<bool> triggered{false};
    scheduler->addEvent(fds[1], zhao::IOManager::WRITE, [&triggered,&fds](){
        std::cout << "write event triggered" << std::endl;
        char buf[1] = {'a'};
        write(fds[1], buf, 1);
        triggered = true;
    });

    // 在事件触发前尝试取消
    if(!triggered) {
        assert(scheduler->cancelEvent(fds[1], zhao::IOManager::WRITE));
        std::cout << "cancel write event success" << std::endl;
    }
    // sleep 1s等待事件触发
    sleep(3);

    

    

    // // 再添加读写事件
    // scheduler->addEvent(fds[0], zhao::IOManager::READ, [](){
    //     std::cout << "read event triggered" << std::endl;
    // });
    // scheduler->addEvent(fds[1], zhao::IOManager::WRITE, [&triggered,&fds](){
    //     std::cout << "write event triggered" << std::endl;
    //     char buf[1] = {'a'};
    //     write(fds[1], buf, 1);
    //     triggered = true;
    // });

    // // 测试取消所有事件
    // assert(scheduler->cancelAll(fds[0]));
    // assert(scheduler->cancelAll(fds[1]));
    // std::cout << "cancel all events success" << std::endl;

    close(fds[0]);
    close(fds[1]);
    scheduler->stop();
}

static void test05() {
    zhao::Scheduler::ptr scheduler(new zhao::Scheduler(2));
    scheduler->start();
    scheduler->schedule([](){
        std::cout << "test05 schedule" << std::endl;
    });
    //sleep(2);
    // 测试一次性延时任务
    auto work1 = scheduler->addDelayWork(1000, [](){
        std::cout << "one shot timer triggered after 1000ms" << std::endl;
    });

    // 测试循环延时任务
    auto work2 = scheduler->addDelayWork(2000, [](){
        std::cout << "recurring timer triggered every 2000ms" << std::endl;
    }, true);

    // // 测试条件延时任务
    // auto cond = std::make_shared<int>(1); // 创建一个条件
    // auto weak_cond = std::weak_ptr<int>(cond);
    
    // auto work3 = scheduler->addConditionDelayWork(1500, []{
    //     std::cout << "condition timer triggered after 1500ms" << std::endl;
    // }, weak_cond);

    // // 等待5秒观察定时器效果
    // sleep(5);

    // // 测试取消定时器
    // work1->cancel();
    // std::cout << "cancel one shot timer" << std::endl;

    // // 测试重置定时器
    // work2->reset(1000, true);
    // std::cout << "reset recurring timer to 1000ms" << std::endl;

    // // 测试刷新定时器
    // work2->refresh();
    // std::cout << "refresh recurring timer" << std::endl;

    // // 清除条件使条件定时器失效
    // cond.reset();
    // std::cout << "reset condition" << std::endl;

    // // 再等待3秒观察效果
    // sleep(3);

    scheduler->stop();
    std::cout <<"end"<<std::endl;
}
static void test06()
{
    zhao::Scheduler* sche = zhao::Scheduler::getThis();
    sche->schedule([](){
        sleep(2);
        std::cout<<"test06 sleep2"<<std::endl;
    });
    sche->schedule([](){
        sleep(3);
        std::cout<<"test06 sleep3"<<std::endl;
    });
}
void scheduler_test() { 
    dbg<<"scheduler test";
    //test01();
    //test02();
    //test03();
    //test04();
    test05();
    //test06();

}