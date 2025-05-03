#ifndef __CONFIG_H
#define __CONFIG_H
#include <string>
#include <memory>
#include <boost/lexical_cast.hpp>
#include "log.h"
#include <map>
#include  <yaml-cpp/yaml.h>
namespace zhao
{
    /**
     * @brief 配置项
     */
    class ConfigItemBase
    {
    protected:
        std::string m_name;
        std::string m_desc;

    public:
        typedef std::shared_ptr<ConfigItemBase> Ptr;
        ConfigItemBase(std::string name, std::string desc) : m_name(name), m_desc(desc) {
            //把m_name转小写
            for (size_t i = 0; i < m_name.size(); i++)
            {
                if (m_name[i] >= 'A' && m_name[i] <= 'Z')
                    m_name[i] = m_name[i] - 'A' + 'a';
                
            }
        }
        virtual ~ConfigItemBase() {}

        std::string getName() const { return m_name; }
        std::string getDesciption() const { return m_desc; }

        virtual std::string toString(void) = 0;
        virtual bool fromString(const std::string &val) = 0;
    };

    template <class FROM, class TO>
    class BaseTypeConverter
    {
    public:
        TO operator()(const FROM &from)
        {
            return boost::lexical_cast<TO>(from);
        }
    };

    //仿函数  fromStr  toStr
    template <class T,class fromStr=BaseTypeConverter<std::string,T>,class toStr=BaseTypeConverter<T,std::string>   >
    class ConfigItem : public ConfigItemBase
    {
    private:
        T m_value;

    public:
        ConfigItem(const std::string &name, const std::string &desc, const T &value)
            : ConfigItemBase(name, desc), m_value(value) {

            }

        typedef std::shared_ptr<ConfigItem> Ptr;
        std::string toString() override
        {
            try
            {
                return toStr()(m_value); //临时对象仿函数
            }
            catch (const std::exception &e)
            {
                ZHAO_LOG_ERROR(GET_ROOT_LOGGER()) << "ConfigItem::toString() exception: " << e.what();
            }
            return "";
        }
        bool fromString(const std::string &val) override
        {
            try
            {
                m_value = fromStr()(val);//临时对象仿函数
                return true;
            }
            catch(const std::exception& e)
            {
                ZHAO_LOG_ERROR(GET_ROOT_LOGGER()) << "ConfigItem::fromString() exception: " << e.what();
            }
            return false;
        }
    };

    class Config
    {

    public:
        typedef std::shared_ptr<Config> Ptr;
        typedef std::map<std::string, ConfigItemBase::Ptr> ConfigMap;
        template<typename T>
        static typename ConfigItem<T>::Ptr lookup(const std::string &name,
                                                   const T &value,
                                                  const std::string &desc)
        {
            auto cfg = m_configs.find(name);
            //如果找到了直接返回
            if (cfg != m_configs.end())
            {
                return std::dynamic_pointer_cast<ConfigItem<T>>(cfg->second);
            }
            //没找到创建并返回
            m_configs[name] = std::make_shared<ConfigItem<T>>(name, desc, value);
            return std::dynamic_pointer_cast<ConfigItem<T>>(m_configs[name]);
            
        }
        template<typename T>
        static typename ConfigItem<T>::Ptr find(const std::string &name)
        {
            return std::dynamic_pointer_cast<ConfigItem<T>>(m_configs[name]);
        }

        static void loadYamlToConfig(const YAML::Node &root);
    private:
        static ConfigMap m_configs;
    };

} // namespace zhao


#endif