#include "config/configvar.hpp"

#include <string>
#include <iostream>
static void test01(void)
{
    // 获取配置管理器实例
    zhao::Config* configMgr = zhao::ConfigMgr::getInstance();

    // 添加配置项
    auto portConfig = configMgr->add<int>("server.port", 8080, "服务器端口");
    auto nameConfig = configMgr->add<std::string>("server.name", "MyServer", "服务器名称");

    // 查找配置项
    auto foundPort = configMgr->lookup<int>("server.port");
    if (foundPort)
    {
        std::cout << "Port: " << foundPort->toString() << std::endl;
    }

    // 如果配置项不存在，lookup会返回nullptr
    auto notExist = configMgr->lookup<double>("non.existent");
    if (!notExist)
    {
        std::cout << "Config not found" << std::endl;
    }
}
static auto g_int_val = zhao::ConfigMgr::getInstance()->add<int>("server.port", 8080, "服务器端口");
static auto g_float_val = zhao::ConfigMgr::getInstance()->add<float>("server.timeout", 4.2, "服务器超时时间");
static auto g_string_val = zhao::ConfigMgr::getInstance()->add<std::string>("server.name", "Zhao", "服务器名称");
static void test02()
{
    
    std::cout << "Port: " << g_int_val->toString() << std::endl;
    std::cout << "Port: " << g_int_val->getValue() << std::endl;

}
static void test03()
{
    std::cout << "before:"<<std::endl;
    std::cout << "Port: " << g_int_val->getValue() << std::endl;
    std::cout << "timeout: " << g_float_val->getValue() << std::endl;
    std::cout << "name: " << g_string_val->getValue() << std::endl;
    YAML::Node root = YAML::LoadFile("config/log.yml");
    zhao::ConfigMgr::getInstance()->LoadFromYaml(root);
    std::cout << "after:"<<std::endl;

    std::cout << "Port: " << g_int_val->getValue() << std::endl;
    std::cout << "timeout: " << g_float_val->getValue() << std::endl;
    std::cout << "name: " << g_string_val->getValue() << std::endl;

}
/**
 * @brief 测试偏特化
 */
static void test04()
{
    // 添加各种类型的配置项
    auto basic_string = zhao::ConfigMgr::getInstance()->add<std::string>("basic_string", "", "基础字符串测试");
    auto basic_int = zhao::ConfigMgr::getInstance()->add<int>("basic_int", 0, "基础整数测试");
    auto basic_double = zhao::ConfigMgr::getInstance()->add<double>("basic_double", 0.0, "基础浮点数测试");
    auto basic_bool = zhao::ConfigMgr::getInstance()->add<bool>("basic_bool", false, "基础布尔值测试");
    
    // vector类型测试
    auto vector_int = zhao::ConfigMgr::getInstance()->add<std::vector<int>>("vector_int", std::vector<int>{}, "整数向量测试");
    auto vector_string = zhao::ConfigMgr::getInstance()->add<std::vector<std::string>>("vector_string", std::vector<std::string>{}, "字符串向量测试");
    auto vector_double = zhao::ConfigMgr::getInstance()->add<std::vector<double>>("vector_double", std::vector<double>{}, "浮点数向量测试");
    
    // list类型测试
    auto list_int = zhao::ConfigMgr::getInstance()->add<std::list<int>>("list_int", std::list<int>{}, "整数列表测试");
    auto list_string = zhao::ConfigMgr::getInstance()->add<std::list<std::string>>("list_string", std::list<std::string>{}, "字符串列表测试");
    
    // set类型测试
    auto set_int = zhao::ConfigMgr::getInstance()->add<std::set<int>>("set_int", std::set<int>{}, "整数集合测试");
    auto set_string = zhao::ConfigMgr::getInstance()->add<std::set<std::string>>("set_string", std::set<std::string>{}, "字符串集合测试");
    
    // unordered_set类型测试
    auto unordered_set_int = zhao::ConfigMgr::getInstance()->add<std::unordered_set<int>>("unordered_set_int", std::unordered_set<int>{}, "整数无序集合测试");
    
    // map类型测试
    auto map_string_int = zhao::ConfigMgr::getInstance()->add<std::map<std::string, int>>("map_string_int", std::map<std::string, int>{}, "字符串到整数映射测试");
    auto map_string_string = zhao::ConfigMgr::getInstance()->add<std::map<std::string, std::string>>("map_string_string", std::map<std::string, std::string>{}, "字符串到字符串映射测试");
    
    // unordered_map类型测试
    auto unordered_map_string_double = zhao::ConfigMgr::getInstance()->add<std::unordered_map<std::string, double>>("unordered_map_string_double", std::unordered_map<std::string, double>{}, "字符串到浮点数无序映射测试");
    
    std::cout << "=== 加载YAML前的默认值 ===" << std::endl;
    std::cout << "basic_string: " << basic_string->getValue() << std::endl;
    std::cout << "basic_int: " << basic_int->getValue() << std::endl;
    std::cout << "vector_int size: " << vector_int->getValue().size() << std::endl;
    std::cout << "map_string_int size: " << map_string_int->getValue().size() << std::endl;
    
    // 加载YAML配置文件
    try {
        YAML::Node root = YAML::LoadFile("config/test.yml");
        zhao::ConfigMgr::getInstance()->LoadFromYaml(root);
        
        std::cout << "\n=== 加载YAML后的值 ===" << std::endl;
        std::cout << "basic_string: " << basic_string->getValue() << std::endl;
        std::cout << "basic_int: " << basic_int->getValue() << std::endl;
        std::cout << "basic_double: " << basic_double->getValue() << std::endl;
        std::cout << "basic_bool: " << (basic_bool->getValue() ? "true" : "false") << std::endl;
        
        // 测试vector
        auto vec_int = vector_int->getValue();
        std::cout << "vector_int: ";
        for (const auto& val : vec_int) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
        
        // 测试map
        auto m_str_int = map_string_int->getValue();
        std::cout << "map_string_int: ";
        for (const auto& pair : m_str_int) {
            std::cout << "{" << pair.first << ": " << pair.second << "} ";
        }
        std::cout << std::endl;
        
        // 测试set
        auto s_int = set_int->getValue();
        std::cout << "set_int: ";
        for (const auto& val : s_int) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
        
        std::cout << "\n=== 测试完成 ===" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "加载YAML文件时出错: " << e.what() << std::endl;
    }
}
class Person{
public:
    std::string m_name;
    int m_age;
    bool m_sex;
    Person(){}
    Person(std::string name, int age, bool sex):m_name(name), m_age(age), m_sex(sex)
    { 
    }
    ~Person()
    {
    }
    std::string toString() const
    {
        std::stringstream ss;
        ss << "Person(" << "name=" << m_name << " age=" << m_age << " sex=" << m_sex << ")";
        return ss.str();
    }

    bool operator==(const Person &rhs) const
    {
        return m_name == rhs.m_name && m_age == rhs.m_age && m_sex == rhs.m_sex;
    }
};
namespace zhao { 
     /*复杂类型拓展 */
    template <>
    class LexicalCast<std::string, Person>
    {
    public:
        Person operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            Person p;
            p.m_name = node["name"].as<std::string>();
            p.m_age = node["age"].as<int>();
            p.m_sex = node["sex"].as<bool>();
            return p;
        }
    };

    template <>
    class LexicalCast<Person, std::string>
    {
    public:
        std::string operator()(const Person &p)
        {
            YAML::Node node;
            node["name"] = p.m_name;
            node["age"] = p.m_age;
            node["sex"] = p.m_sex;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}
static void test05()
{
    std::cout << "\n=== 测试嵌套复杂类型 ===" << std::endl;
    auto class_person = zhao::ConfigMgr::getInstance()->add<Person>("class_person", Person(), "人员映射测试");
    
    // 添加嵌套map类型配置项
    auto class_map = zhao::ConfigMgr::getInstance()->add<std::map<std::string, Person>>("class_map", std::map<std::string, Person>{}, "人员映射测试");
    
    // 添加vector类型配置项
    auto class_vector = zhao::ConfigMgr::getInstance()->add<std::vector<Person>>("class_vector", std::vector<Person>{}, "人员向量测试");
    
    std::cout << "=== 加载YAML前的默认值 ===" << std::endl;
    std::cout << "class_map size: " << class_map->getValue().size() << std::endl;
    std::cout << "class_vector size: " << class_vector->getValue().size() << std::endl;
    
    // 加载YAML配置文件
    try {
        YAML::Node root = YAML::LoadFile("config/test.yml");
        zhao::ConfigMgr::getInstance()->LoadFromYaml(root);
        
        std::cout << "\n=== 加载YAML后的值 ===" << std::endl;
        //测试class_Person
        auto cPerson = class_person->getValue();
        std::cout << cPerson.toString() << std::endl;
        // 测试class_map
        auto cmap = class_map->getValue();
        std::cout << "class_map: " << std::endl;
        for (const auto& pair : cmap) {
            std::cout << "  " << pair.first << ": " << pair.second.toString() << std::endl;
        }
        
        // 测试class_vector
        auto cvec = class_vector->getValue();
        std::cout << "class_vector: " << std::endl;
        for (size_t i = 0; i < cvec.size(); ++i) {
            std::cout << "  [" << i << "]: " << cvec[i].toString() << std::endl;
        }
        
        std::cout << "\n=== 嵌套复杂类型测试完成 ===" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "加载YAML文件时出错: " << e.what() << std::endl;
    }

}
void config_test(void)
{
    //
    test05();
}