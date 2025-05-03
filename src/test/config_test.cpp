#include "config.h"

#include "log.h"
#include  <yaml-cpp/yaml.h>
static void test1(void)
{

    zhao::Config::lookup("myKey", std::string("hello"), "desc");
    auto cfg = zhao::Config::find<std::string>("myKey");
    ZHAO_LOG_FMT_INFO(GET_ROOT_LOGGER(), "myKey = %s", cfg->toString().c_str());
}
static void test2(void)
{
    YAML::Node config = YAML::LoadFile("config/config.yaml");

    std::string name = config["name"].as<std::string>();
    int age = config["age"].as<int>();
    bool debug = config["debug"].as<bool>();

    std::cout << "Name: " << name << "\n";
    std::cout << "Age: " << age << "\n";
    std::cout << "Debug: " << (debug ? "true" : "false") << "\n";

  
}

#include <vector>
#include <string>

struct Appender {
    std::string type;
    std::string dest;
};

struct LoggerConfig {
    std::string name;
    std::string level;
    std::string formatter;
    std::vector<Appender> appenders;
};

std::vector<LoggerConfig> loadLoggers(const std::string& filename) {
    YAML::Node config = YAML::LoadFile(filename);
    std::vector<LoggerConfig> loggers;

    for (const auto& node : config["logs"]) { 
        LoggerConfig logger;
        logger.name = node["name"].as<std::string>();
        logger.level = node["level"].as<std::string>();
        logger.formatter = node["formatter"].as<std::string>();

        for (const auto& appenderNode : node["appenders"]) {
            Appender appender;
            appender.type = appenderNode["type"].as<std::string>();
            if (appender.type == "file") {
                appender.dest = appenderNode["dest"].as<std::string>();
            }
            logger.appenders.push_back(appender);
        }

        loggers.push_back(logger);
    }

    return loggers;
}

static void test3(void)
{
    auto loggers = loadLoggers("config/config.yaml");

    for (const auto& logger : loggers) {
        std::cout << "Logger: " << logger.name << "\n";
        std::cout << "  Level: " << logger.level << "\n";
        std::cout << "  Formatter: " << logger.formatter << "\n";
        for (const auto& app : logger.appenders) {
            std::cout << "  Appender Type: " << app.type;
            if (!app.dest.empty()) std::cout << ", Dest: " << app.dest;
            std::cout << "\n";
        }
        std::cout << "\n";
    }

}
static void test4(void)
{
    zhao::ConfigItem<int>::Ptr cfgint = zhao::Config::lookup("testint", (int)100, "test");
    zhao::ConfigItem<float>::Ptr cfgfloat = zhao::Config::lookup("testfloat", (float)3.1415f, "test");
    ZHAO_LOG_INFO(GET_ROOT_LOGGER()) << "before:" << cfgint->toString() << " " << cfgfloat->toString(); 

    YAML::Node root = YAML::LoadFile("config/config.yaml");
    zhao::Config::loadYamlToConfig(root);

    ZHAO_LOG_INFO(GET_ROOT_LOGGER()) << "after:" << cfgint->toString() << " " << cfgfloat->toString();

}
void config_test()
{
    test4();
}