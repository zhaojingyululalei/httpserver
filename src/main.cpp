#include "log.h"
#include <thread>
#include <chrono>
#include <yaml-cpp/yaml.h>
#include "config.h"
extern void log_test(void);
extern void config_test(void);
extern void thread_test(void);
extern void fiber_test(void);
extern void fiberscheduler_test(void);
extern void ioscheduler_test(void);
extern void timerscheduler_test(void);
extern void hook_test(void);
void load_config(void)
{
    YAML::Node root = YAML::LoadFile("config/config.yaml");
    zhao::Config::loadYamlToConfig(root);
}
int main()
{
    load_config();
    // log_test();
    //config_test();
    //thread_test();
    //fiber_test();
    //fiberscheduler_test();
    //ioscheduler_test();
    //timerscheduler_test();
    hook_test();
    return 0;
}