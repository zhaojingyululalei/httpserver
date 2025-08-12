
#include <yaml-cpp/yaml.h>
#include <iostream>
/**
 * @brief 遍历yaml Node
 */
static void traverse(const YAML::Node &node)
{
    if(node.IsMap()){
        for(auto it = node.begin(); it != node.end(); ++it){
            std::cout << it->first << ":" << std::endl;
            traverse(it->second);
        }
    }else if(node.IsSequence()){
        for(auto it = node.begin(); it != node.end(); ++it){
            traverse(*it);
        }
    }else if(node.IsScalar()){
        std::cout << node << std::endl;

    }
}
static void test02()
{
    YAML::Node root = YAML::LoadFile("config/log.yml");
    traverse(root);
}
static void test01(void)
{
    YAML::Node root = YAML::LoadFile("config/log.yml");
    std::cout << root << std::endl;
}
void yaml_test(){
    test02();
}