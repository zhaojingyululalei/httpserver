#include "log.h"
#include <thread>
#include <chrono>
#include <yaml-cpp/yaml.h>
#include "config.h"
extern void log_test(void);
extern void config_test(void);
extern void thread_test(void);
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
    thread_test();
    return 0;
}