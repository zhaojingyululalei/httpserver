#include <vector>
#include <iostream>
#include <yaml-cpp/yaml.h>


#include "thread.h"
#include "log.h"
#include "config.h"
#include "utils.h"
void function(void* arg){
    YAML::Node root = YAML::LoadFile("config/config.yaml");
    zhao::Config::loadYamlToConfig(root);
    // static zhao::Logger::Ptr root_log = GET_ROOT_LOGGER();
    // ZHAO_LOG_FATAL(root_log) << "hello root"<<" i am thread:"<<zhao::getThreadName() << "\n ";
    static zhao::Logger::Ptr system_log = GET_LOGGER("system");
    ZHAO_LOG_FATAL(system_log) << "hello system"<<" i am thread:"<<zhao::getThreadName() << "\n ";
}
static void test01(void){
    YAML::Node root = YAML::LoadFile("config/config.yaml");
    zhao::Config::loadYamlToConfig(root);
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
static zhao::Logger::Ptr g_logger = GET_ROOT_LOGGER();
static void test02(){
    //ZHAO_LOG_INFO(g_logger) << zhao::backtraceToString();
    ZHAO_LOG_ASSERT(1 == 2);
}
void thread_test(){
     
    test02();
}