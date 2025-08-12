#include "config/configvar.hpp"
#define MODULE_NAME "config"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_ROOT())
#define warn MODULE_WARN(MODULE_NAME, LOGGER_ROOT())
#define error MODULE_ERROR(MODULE_NAME, LOGGER_ROOT())
namespace zhao
{

    ConfigVarBase::ptr Config::LookupBase(const std::string &name)
    {
        RWMutex::ReadLockGuardType lock(m_mutex);
        auto it = ConfigMgr::getInstance()->m_datas.find(name);
        return it == m_datas.end() ? nullptr : it->second;
    }
    //"A.B", 10
    // A:
    //   B: 10
    //   C: str
    /*ConfigVar的key：A.B  value: 10  就只是解析解析map类型，把嵌套的map类型弄成A.B的形式 */
    static void ListAllMember(const std::string &prefix,
                              const YAML::Node &node,
                              std::list<std::pair<std::string, const YAML::Node>> &output)
    {
        
        output.push_back(std::make_pair(prefix, node));
        if (node.IsMap())
        {
            for (auto it = node.begin();
                 it != node.end(); ++it)
            {
                ListAllMember(prefix.empty() ? it->first.Scalar()
                                             : prefix + "." + it->first.Scalar(),
                              it->second, output);
            }
        }
    }
    /**
     * @brief 将一个YAML::Node 转换成 ConfigVarMap
     */
    void Config::LoadFromYaml(const YAML::Node &root)
    {
        std::list<std::pair<std::string, const YAML::Node>> all_nodes;
        ListAllMember("", root, all_nodes);

        for (auto &i : all_nodes)
        {
            std::string key = i.first;
            if (key.empty())
            {
                continue;
            }

            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            ConfigVarBase::ptr var = LookupBase(key);

            if (var)
            {
                if (i.second.IsScalar())
                {
                    var->fromString(i.second.Scalar());
                }
                else
                {
                    std::stringstream ss;
                    ss << i.second;
                    var->fromString(ss.str());
                }
            }
        }
    }
}
