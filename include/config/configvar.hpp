#ifndef __ZHAO_CONFIGVAR_H__
#define __ZHAO_CONFIGVAR_H__
#include <memory>
#include "log/logger.hpp"
#include "utils/singleton.hpp"
#include "config/configextent.hpp"
#include <functional>
#define MODULE_NAME "config"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_ROOT())
#define warn MODULE_WARN(MODULE_NAME, LOGGER_ROOT())
#define error MODULE_ERROR(MODULE_NAME, LOGGER_ROOT())
namespace zhao
{
    class ConfigVarBase
    {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;
        ConfigVarBase(const std::string &name, const std::string &description)
            : m_name(name), m_description(description)
        {
        }
        const std::string &getName() const { return m_name; }
        const std::string &getDescription() const { return m_description; }
        virtual ~ConfigVarBase() {}
        virtual std::string toString() = 0;
        virtual bool fromString(const std::string &val) = 0;

    protected:
        std::string m_name;
        std::string m_description;
    };

    /**
     * @brief 基础类型转换
     */
    template <class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        typedef std::shared_ptr<ConfigVar> ptr;
        typedef std::function<void(const T &old_value, const T &new_value)> on_change_cb;
        ConfigVar(const std::string &name, const T &val, const std::string &description)
            : ConfigVarBase(name, description), m_val(val)
        {
        }
        std::string toString() override
        {
            try
            {
                RWMutex::ReadLockGuardType lock(m_mutex);
                // return boost::lexical_cast<std::string>(m_val);
                return ToStr()(m_val); // ToStr()临时对象
            }
            catch (...)
            {
                warn << "boost::lexical_cast<std::string>(m_val) error!" << "convert" << typeid(m_val).name() << " to string error\n";
            }
            return "";
        }
        bool fromString(const std::string &val) override
        {
            try
            {

                // m_val = boost::lexical_cast<T>(val);
                setValue(FromStr()(val)); // FromStr()临时对象
                return true;
            }
            catch (...)
            {
                warn << "boost::lexical_cast<" << typeid(T).name() << ">(val) error!" << "convert string to " << typeid(T).name() << " error\n";
            }
            return false;
        }

        const T &getValue() 
        {
            RWMutex::ReadLockGuardType lock(m_mutex);
            return m_val;
        }
        void setValue(const T &val)
        {
            RWMutex::WriteLockGuardType lock(m_mutex);
            if (val == m_val)
            { // 注：实现类的时候需要实现==号重载
                return;
            }
            for (auto &i : m_cbs)
            {
                i.second(m_val, val);
            }
            m_val = val;
        }
        std::string getType()
        {
            RWMutex::ReadLockGuardType lock(m_mutex);
            return typeid(T).name();
        }

        /**
         * @brief 添加变化回调函数
         * @return 返回该回调函数对应的唯一id,用于删除回调
         */
        uint64_t addListener(on_change_cb cb)
        {
            static uint64_t s_fun_id = 0;
            RWMutex::WriteLockGuardType lock(m_mutex);
            ++s_fun_id;
            m_cbs[s_fun_id] = cb;
            return s_fun_id;
        }

        /**
         * @brief 删除回调函数
         * @param[in] key 回调函数的唯一id
         */
        void delListener(uint64_t key)
        {
            RWMutex::WriteLockGuardType lock(m_mutex);
            m_cbs.erase(key);
        }

        /**
         * @brief 获取回调函数
         * @param[in] key 回调函数的唯一id
         * @return 如果存在返回对应的回调函数,否则返回nullptr
         */
        on_change_cb getListener(uint64_t key)
        {
            RWMutex::ReadLockGuardType lock(m_mutex);
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }

        /**
         * @brief 清理所有的回调函数
         */
        void clearListener()
        {
            RWMutex::WriteLockGuardType lock(m_mutex);
            m_cbs.clear();
        }

    private:
        T m_val;
        std::map<uint64_t, on_change_cb> m_cbs;
        RWMutex m_mutex;
    };

    class Config
    {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;
        /**
         * @brief 查找配置项是否存在
         */
        template <typename T>
        typename ConfigVar<T>::ptr lookup(const std::string &name)
        {
            RWMutex::ReadLockGuardType lock(m_mutex);
            auto it = m_datas.find(name);
            if (it != m_datas.end())
            {
                return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            }
            return nullptr;
        }
        /**
         * @brief 添加配置项
         */
        template <typename T>
        typename ConfigVar<T>::ptr add(const std::string &name, const T &value, const std::string &description = "")
        {
            auto new_var = lookup<T>(name);
            RWMutex::WriteLockGuardType lock(m_mutex);
            if (new_var)
            {
                warn << name << "already exists";
                return new_var;
            }
            else
            {
                new_var = std::make_shared<ConfigVar<T>>(name, value, description);
                m_datas[name] = new_var;
                return new_var;
            }
        }

        void LoadFromYaml(const YAML::Node &root);
        ConfigVarBase::ptr LookupBase(const std::string &name);

    private:
        std::map<std::string, ConfigVarBase::ptr> m_datas;
        RWMutex m_mutex;
    };
    typedef Singleton<Config> ConfigMgr;
}

#undef MODULE_NAME
#undef dbg
#undef warn
#undef error
#endif