#include "config.h"
#include <map>
#include <list>
#include <yaml-cpp/yaml.h>
namespace zhao
{
    Config::ConfigMap Config::m_configs;

    static void getAllYamlNode(const std::string prefix, const YAML::Node &node, std::list<std::pair<std::string, YAML::Node>> &nodesList)
    {
        nodesList.push_back(std::make_pair(prefix, node));
        if (node.IsMap())
        {
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                
                std::string newPrefix = prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar();
                // 递归处理子节点
                getAllYamlNode(newPrefix, it->second, nodesList);
            }
        }
    }
    void Config::loadYamlToConfig(const YAML::Node &root)
    {
        std::list<std::pair<std::string, YAML::Node>> nodesList;

        getAllYamlNode("", root, nodesList);
        // 遍历所有yaml_node
        for (auto &listnode : nodesList)
        {
            std::string key = listnode.first;
            if (key.empty())
            {
                continue;
            }
            // 查找有没有定义该配置项，如果没定义，这个yaml_node不处理
            // 如果找到了该配置项，修改值
            auto cfgitem = m_configs.find(key);
            if (cfgitem != m_configs.end())
            {
                ConfigItemBase::Ptr configItem = cfgitem->second;
                if (configItem)
                {
                    if (listnode.second.IsScalar())
                        // 如果是标量，直接赋值标量值，通过boost::lexer解析
                        configItem->fromString(listnode.second.Scalar());
                    else
                    {
                        // 如果不是标量，直接把yaml_node原形式字符串复制给配置项
                        std::stringstream ss;
                        ss << listnode.second;
                        configItem->fromString(ss.str());
                    }
                }
            }
        }
    }
} // namespace zhao
