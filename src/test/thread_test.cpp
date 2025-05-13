#include "thread.h"
#include <vector>
#include <iostream>
void function(void* arg){
    std::cout<<"thread_id: "<<zhao::Thread::getCur()->getTid()
    <<" thread_name: "<<zhao::Thread::getCurName()<<std::endl;  
}
static void test01(void){
    int thread_count = 5;
    std::vector<zhao::Thread::Ptr> threads;
    for (int i = 0; i < thread_count; i++) {
        zhao::Thread::Ptr thrd(new zhao::Thread(function, "name_"+std::to_string(i),nullptr));
        threads.push_back(thrd);
    }
    // 等待所有线程完成
    for (auto& thrd : threads) {
        thrd->join();
    }
}
void thread_test(){
    test01();
}