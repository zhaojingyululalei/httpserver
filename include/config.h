#ifndef __CONFIG_H
#define __CONFIG_H
#include <string>
#include <memory>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <functional>
#include <stdint.h>
#include <list>

#include "log.h"
#include "ipc.h"
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
        ConfigItemBase(std::string name, std::string desc) : m_name(name), m_desc(desc)
        {
            // 把m_name转小写
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

    template <class T>
    class BaseTypeConverter<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string &from)
        {

            YAML::Node node = YAML::Load(from); // node为数组类型
            // 如果node不是sequence类型，抛出异常
            if (!node.IsSequence())
                throw std::logic_error("node is not a sequence");
            typename std::vector<T> vec;
            for (size_t i = 0; i < node.size(); i++)
            {
                std::stringstream ss;
                ss << node[i];
                vec.push_back(BaseTypeConverter<std::string, T>()(ss.str()));
            }

            return vec;
        }
    };

    template <class T>
    class BaseTypeConverter<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T> &from)
        {
            YAML::Node node;
            for (auto i : from)
            {
                node.push_back(BaseTypeConverter<T, std::string>()(i));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class BaseTypeConverter<std::string, std::list<T>>
    {
    public:
        std::list<T> operator()(const std::string &from)
        {

            YAML::Node node = YAML::Load(from); // node为数组类型
            // 如果node不是sequence类型，抛出异常
            if (!node.IsSequence())
                throw std::logic_error("node is not a sequence");
            typename std::list<T> vec;
            for (size_t i = 0; i < node.size(); i++)
            {
                std::stringstream ss;
                ss << node[i];
                vec.push_back(BaseTypeConverter<std::string, T>()(ss.str()));
            }

            return vec;
        }
    };

    template <class T>
    class BaseTypeConverter<std::list<T>, std::string>
    {
    public:
        std::string operator()(const std::list<T> &from)
        {
            YAML::Node node;
            for (auto i : from)
            {
                node.push_back(BaseTypeConverter<T, std::string>()(i));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class BaseTypeConverter<std::string, std::set<T>>
    {
    public:
        std::set<T> operator()(const std::string &from)
        {

            YAML::Node node = YAML::Load(from); // node为数组类型
            // 如果node不是sequence类型，抛出异常
            if (!node.IsSequence())
                throw std::logic_error("node is not a sequence");
            typename std::set<T> set;
            for (size_t i = 0; i < node.size(); i++)
            {
                std::stringstream ss;
                ss << node[i];
                set.insert(BaseTypeConverter<std::string, T>()(ss.str()));
            }

            return set;
        }
    };

    template <class T>
    class BaseTypeConverter<std::set<T>, std::string>
    {
    public:
        std::string operator()(const std::set<T> &from)
        {
            YAML::Node node;
            for (auto i : from)
            {
                node.push_back(BaseTypeConverter<T, std::string>()(i));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    template <class T>
    class BaseTypeConverter<std::string, std::unordered_set<T>>
    {
    public:
        std::unordered_set<T> operator()(const std::string &from)
        {

            YAML::Node node = YAML::Load(from); // node为数组类型
            // 如果node不是sequence类型，抛出异常
            if (!node.IsSequence())
                throw std::logic_error("node is not a sequence");
            typename std::unordered_set<T> set;
            for (size_t i = 0; i < node.size(); i++)
            {
                std::stringstream ss;
                ss << node[i];
                set.insert(BaseTypeConverter<std::string, T>()(ss.str()));
            }

            return set;
        }
    };

    template <class T>
    class BaseTypeConverter<std::unordered_set<T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_set<T> &from)
        {
            YAML::Node node;
            for (auto i : from)
            {
                node.push_back(BaseTypeConverter<T, std::string>()(i));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    template <class T>
    class BaseTypeConverter<std::string, std::map<std::string, T>>
    {
    public:
        std::map<std::string, T> operator()(const std::string &from)
        {

            YAML::Node node = YAML::Load(from); // node为数组类型
            // 如果node不是sequence类型，抛出异常
            if (!node.IsMap())
                throw std::logic_error("node is not a map");
            typename std::map<std::string, T> map;
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                std::stringstream ss;
                ss << it->second;
                map.insert(std::make_pair(it->first.Scalar(), BaseTypeConverter<std::string, T>()(ss.str())));
            }

            return map;
        }
    };

    template <class T>
    class BaseTypeConverter<std::map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, T> &from)
        {
            YAML::Node node;
            for (auto i : from)
            {
                node[i.first] = YAML::Load(BaseTypeConverter<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class BaseTypeConverter<std::string, std::unordered_map<std::string, T>>
    {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &from)
        {

            YAML::Node node = YAML::Load(from); // node为数组类型
            // 如果node不是sequence类型，抛出异常
            if (!node.IsMap())
                throw std::logic_error("node is not a map");
            typename std::unordered_map<std::string, T> map;
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                std::stringstream ss;
                ss << it->second;
                map.insert(std::make_pair(it->first.Scalar(), BaseTypeConverter<std::string, T>()(ss.str())));
            }

            return map;
        }
    };

    template <class T>
    class BaseTypeConverter<std::unordered_map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &from)
        {
            YAML::Node node;
            for (auto i : from)
            {
                node[i.first] = YAML::Load(BaseTypeConverter<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    // 仿函数  fromStr  toStr
    template <class T, class fromStr = BaseTypeConverter<std::string, T>, class toStr = BaseTypeConverter<T, std::string>>
    class ConfigItem : public ConfigItemBase
    {

    public:
        ConfigItem(const std::string &name, const std::string &desc, const T &value)
            : ConfigItemBase(name, desc), m_value(value)
        {
        }

        typedef std::shared_ptr<ConfigItem> Ptr;
        typedef std::function<void(const T &old_value, const T &new_value)> onChangeCallback;
        T getvalue()
        {
            RWMutex::ReadLockGuardType lock(m_mutex);
            return m_value;
        }
        void setvalue(const T &value)
        {
            m_mutex.rdlock();
            if (value == m_value){

                m_mutex.unlock();
                return;
            }
            else
            {
                for (auto &i : m_onChangeCallbacks)
                {
                    i.second(m_value, value); // 值改变后调用注册的回调函数
                }
                m_mutex.unlock();
                {
                    RWMutex::WriteLockGuardType lock(m_mutex);
                    m_value = value;
                }
            }
        }
        std::string toString() override
        {
            try
            {
                RWMutex::ReadLockGuardType lock(m_mutex);
                return toStr()(m_value); // 临时对象仿函数
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
                setvalue(fromStr()(val)); // 临时对象仿函数
                return true;
            }
            catch (const std::exception &e)
            {
                ZHAO_LOG_ERROR(GET_ROOT_LOGGER()) << "ConfigItem::fromString() exception: " << e.what();
            }
            return false;
        }
        uint64_t addOnChangeCallback(onChangeCallback callback)
        {
            static uint64_t s_id = 0;
            RWMutex::WriteLockGuardType lock(m_mutex);
            m_onChangeCallbacks[s_id++] = callback;
            return s_id;
        }
        void delOnChangeCallback(uint64_t id)
        {
            RWMutex::WriteLockGuardType lock(m_mutex);
            m_onChangeCallbacks.erase(id);
        }
        onChangeCallback getOnChangeCallbacks(uint64_t id)
        {
            RWMutex::ReadLockGuardType lock(m_mutex);
            if (m_onChangeCallbacks.find(id) != m_onChangeCallbacks.end())
                return m_onChangeCallbacks[id];
            return nullptr;
        }
        void clearOnChangeCallbacks()
        {
            RWMutex::WriteLockGuardType lock(m_mutex);
            m_onChangeCallbacks.clear();
        }

    private:
        T m_value;
        std::map<uint64_t, onChangeCallback> m_onChangeCallbacks;
        RWMutex m_mutex;
    };

    class Config
    {

    public:
        typedef std::shared_ptr<Config> Ptr;
        typedef std::map<std::string, ConfigItemBase::Ptr> ConfigMap;
        /**
         * @breif 有就返回，没有创建
         */
        template <typename T>
        static typename ConfigItem<T>::Ptr lookup(const std::string &name,
                                                  const T &value,
                                                  const std::string &desc)
        {
            getMutex().rdlock();
            auto cfg = getConfigs().find(name);
            // 如果找到了直接返回
            if (cfg != getConfigs().end())
            {
                getMutex().unlock();
                return std::dynamic_pointer_cast<ConfigItem<T>>(cfg->second);
            }
            getMutex().unlock();
            // 没找到创建并返回
            getMutex().wrlock();
            auto item = std::make_shared<ConfigItem<T>>(name, desc, value);
            if (!item)
            {
                std::cerr << "Failed to create ConfigItem for " << name << std::endl;
            }
            getConfigs()[name] = item;
            getMutex().unlock();
            // getConfigs()[name] = std::make_shared<ConfigItem<T>>(name, desc, value);
            return std::dynamic_pointer_cast<ConfigItem<T>>(item);
        }
        /**
         * @brief 查找配置项，没有返回空指针
         */
        template <typename T>
        static typename ConfigItem<T>::Ptr find(const std::string &name)
        {
            RWMutex::ReadLockGuardType lock(getMutex());
            return std::dynamic_pointer_cast<ConfigItem<T>>(getConfigs()[name]);
        }

        static void loadYamlToConfig(const YAML::Node &root);

    private:
    //防止初始化顺序错误
        static RWMutex& getMutex()
        {
            static RWMutex s_mutex;
            return s_mutex;
        }
        static ConfigMap &getConfigs()
        {
            static ConfigMap s_configs;
            return s_configs;
        }
    };

} // namespace zhao

#endif