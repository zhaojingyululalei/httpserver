#include "config.h"

#include "log.h"
#include <yaml-cpp/yaml.h>
#include <vector>
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

struct Appender
{
    std::string type;
    std::string dest;
};

struct LoggerConfig
{
    std::string name;
    std::string level;
    std::string formatter;
    std::vector<Appender> appenders;
};

std::vector<LoggerConfig> loadLoggers(const std::string &filename)
{
    YAML::Node config = YAML::LoadFile(filename);
    std::vector<LoggerConfig> loggers;

    for (const auto &node : config["logs"])
    {
        LoggerConfig logger;
        logger.name = node["name"].as<std::string>();
        logger.level = node["level"].as<std::string>();
        logger.formatter = node["formatter"].as<std::string>();

        for (const auto &appenderNode : node["appenders"])
        {
            Appender appender;
            appender.type = appenderNode["type"].as<std::string>();
            if (appender.type == "file")
            {
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

    for (const auto &logger : loggers)
    {
        std::cout << "Logger: " << logger.name << "\n";
        std::cout << "  Level: " << logger.level << "\n";
        std::cout << "  Formatter: " << logger.formatter << "\n";
        for (const auto &app : logger.appenders)
        {
            std::cout << "  Appender Type: " << app.type;
            if (!app.dest.empty())
                std::cout << ", Dest: " << app.dest;
            std::cout << "\n";
        }
        std::cout << "\n";
    }
}
// static void test4(void)
// {

// #define XX(type, name, prefix, des) \
//     zhao::ConfigItem<type>::Ptr cfg##name = zhao::Config::lookup(#prefix "." #name, (type)0, des);
//     XX(int,port, system, "port number");
//     XX(float,value, system, "value number");
//     XX(std::vector<int>,vecs, system, "vecs number");
//     XX(std::list<int>,list, system, "list number");
// #undef XX
    
//     // zhao::ConfigItem<int>::Ptr cfgint = zhao::Config::lookup("system.port", (int)100, "test");
//     // zhao::ConfigItem<float>::Ptr cfgfloat = zhao::Config::lookup("system.value", (float)3.1415f, "test");
//     // zhao::ConfigItem<std::vector<int>>::Ptr cfgvec = zhao::Config::lookup("system.vecs", std::vector<int>{4, 5, 6}, "test");
//     // zhao::ConfigItem<std::list<int>>::Ptr cfglist = zhao::Config::lookup("system.list", std::list<int>{7, 8, 9}, "test");
//     // zhao::ConfigItem<std::set<int>>::Ptr cfgs = zhao::Config::lookup("system.set", std::set<int>{10,11,12}, "test");
//     // zhao::ConfigItem<std::map<int,int>>::Ptr cfgmap = zhao::Config::lookup("system.map", std::map<int,int>{{13,14},{15,16}}, "test");
//     // zhao::ConfigItem<std::unordered_map<int,int>>::Ptr cfgumap = zhao::Config::lookup("system.umap", std::unordered_map<int,int>{{17,18},{19,20}}, "test");
//     // zhao::ConfigItem<std::unordered_set<int>>::Ptr cfguset = zhao::Config::lookup("system.uset", std::unordered_set<int>{21,22,23}, "test");
//     ZHAO_LOG_INFO(GET_ROOT_LOGGER()) << "before:\n" ;
// #define XX(name)\
//     ZHAO_LOG_INFO(GET_ROOT_LOGGER()) << "##name" << ":" << cfg##name->toString() << " \n";
//     XX(port);
//     XX(value);
//     XX(vecs);
//     //XX(list);
// #undef XX

//     YAML::Node root = YAML::LoadFile("config/config.yaml");
//     zhao::Config::loadYamlToConfig(root);

// #define XX(type,name,prefix) \
//     cfg##name = zhao::Config::find<type>(#prefix "." #name);
//     XX(int,port, system);
//     XX(float,value, system);
//     XX(std::vector<int>,vecs, system);
//     ///XX(std::list<int>,list, system);
// #undef XX
//     // cfgint = zhao::Config::find<int>("system.port");
//     // cfgfloat = zhao::Config::find<float>("system.value");
//     // cfgvec = zhao::Config::find<std::vector<int>>("system.vecs");
//     // cfglist = zhao::Config::find<std::list<int>>("system.list");
//     ZHAO_LOG_INFO(GET_ROOT_LOGGER()) << "after:\n" ;
// #define XX(name)\
//     ZHAO_LOG_INFO(GET_ROOT_LOGGER()) << "##name" << ":" << cfg##name->toString() << " \n";
//     XX(port);
//     XX(value);
//     XX(vecs);
//     //XX(list);
// #undef XX
// }

class Student{
public:
    int age;
    std::string name;
    bool sex;
    Student()
    {}
    Student(int age, std::string name, bool sex)
        : age(age), name(name), sex(sex)
    {}

    std::string toString() const
    {
        std::stringstream ss;
        ss << "age:" << age << " name:" << name << " sex:" << sex;
        return ss.str();
    }
};
namespace zhao
{
    template <>
    class BaseTypeConverter<std::string,Student>
    {
    public:
        Student operator()(const std::string &from)
        { 
            Student stu;
            YAML::Node node = YAML::Load(from); //node为数组类型
            //如果node不是sequence类型，抛出异常
            if (!node.IsMap())
                throw std::logic_error("node is not a map");
            stu.age = node["age"].as<int>();
            stu.name = node["name"].as<std::string>();
            stu.sex = node["sex"].as<bool>();
            return  stu;
        }
    };

    template <>
    class BaseTypeConverter<Student,std::string>
    {
    public:
        std::string operator()(const Student &from)
        {
            YAML::Node node;
            node["age"] = from.age;
            node["name"]=from.name;
            node["sex"] = from.sex;
            std::stringstream ss;
            ss<<node;
            return ss.str();
        }
    };
} // namespace zhao



#define LOG_CONFIG_VALUE(name) \
    ZHAO_LOG_INFO(GET_ROOT_LOGGER()) << #name << ":" << cfg##name->toString() << "\n";
static void test4(void)
{
    
#define CONFIG_ITEM(type, name, prefix, des,val) \
    zhao::ConfigItem<type>::Ptr cfg##name = zhao::Config::lookup(#prefix "." #name, (type)val, des);

    CONFIG_ITEM(int, port, system, "port number",8080);
    CONFIG_ITEM(float, value, system, "value number",3.1415f);
    CONFIG_ITEM(std::vector<int>, vecs, system, "vecs number",std::vector<int>{1});
    CONFIG_ITEM(std::list<int>, list, system, "list number",std::list<int>{2});
    CONFIG_ITEM(std::set<int>, set, system, "set number",std::set<int>{3});
    CONFIG_ITEM(std::unordered_set<int>, uset, system, "uset number",std::unordered_set<int>{4});
    //CONFIG_ITEM(std::map<std::string,int>, map, system, "map number",std::map<std::string,int>{{a,15}});
    zhao::ConfigItem<std::map<std::string,int>>::Ptr cfgmap = zhao::Config::lookup("system.map", std::map<std::string,int>{{"a",15}}, "map number");
    zhao::ConfigItem<std::unordered_map<std::string,int>>::Ptr cfgumap = zhao::Config::lookup("system.umap", std::unordered_map<std::string,int>{{"b",16}}, "umap number");
    zhao::ConfigItem<Student>::Ptr cfgstu = zhao::Config::lookup("system.stu", Student(18,"zhao",true), "stu number");
    zhao::ConfigItem<std::map<std::string,Student>>::Ptr cfgstu_map = zhao::Config::lookup("class.stu_map", std::map<std::string,Student>{{"zhao",Student(18,"zhao",true)}}, "stu_map number");
#undef CONFIG_ITEM

    // 输出配置项初始值
    ZHAO_LOG_INFO(GET_ROOT_LOGGER()) << "before:\n";
    LOG_CONFIG_VALUE(port);
    LOG_CONFIG_VALUE(value);
    LOG_CONFIG_VALUE(vecs);
    LOG_CONFIG_VALUE(list);
    LOG_CONFIG_VALUE(set);
    LOG_CONFIG_VALUE(uset);
    LOG_CONFIG_VALUE(map);
    LOG_CONFIG_VALUE(umap);
    LOG_CONFIG_VALUE(stu);
    LOG_CONFIG_VALUE(stu_map);

    // 加载 YAML 配置文件并更新配置项
    YAML::Node root = YAML::LoadFile("config/config.yaml");
    zhao::Config::loadYamlToConfig(root);

    // 重新获取配置项并输出更新后的值
#define FIND_CONFIG(type, name, prefix) \
    cfg##name = zhao::Config::find<type>(#prefix "." #name);

    FIND_CONFIG(int, port, system);
    FIND_CONFIG(float, value, system);
    FIND_CONFIG(std::vector<int>, vecs, system);
    FIND_CONFIG(std::list<int>, list, system);
    FIND_CONFIG(std::set<int>, set, system);
    FIND_CONFIG(std::unordered_set<int>, uset, system);
    cfgmap = zhao::Config::find<std::map<std::string,int>>("system.map");
    cfgumap = zhao::Config::find<std::unordered_map<std::string,int>>("system.umap");
    cfgstu = zhao::Config::find<Student>("system.stu");
    cfgstu_map = zhao::Config::find<std::map<std::string,Student>>("class.stu_map");
#undef FIND_CONFIG

    ZHAO_LOG_INFO(GET_ROOT_LOGGER()) << "after:\n";
    LOG_CONFIG_VALUE(port);
    LOG_CONFIG_VALUE(value);
    LOG_CONFIG_VALUE(vecs);
    LOG_CONFIG_VALUE(list);
    LOG_CONFIG_VALUE(set);
    LOG_CONFIG_VALUE(uset);
    LOG_CONFIG_VALUE(map);
    LOG_CONFIG_VALUE(umap);
    LOG_CONFIG_VALUE(stu);
    LOG_CONFIG_VALUE(stu_map);
}


void config_test()
{
    test4();
}