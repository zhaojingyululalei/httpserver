#include "logger.hpp"
#include "log/logconsole.hpp"
#include "log/logevent.hpp"
#include "log/logformatter.hpp"
#include "log/logger.hpp"
#include "log/loglevel.hpp"
#include "config/configvar.hpp"
#include <set>
#include <iostream>
namespace zhao
{
    zhao::Logger::Logger(const std::string &name) : m_name(name)
    {
        
        if (!m_formatter)
        {
            m_formatter = std::shared_ptr<LogFormatter>(new LogFormatter(LOG_DEFAULT_PATTERN));
        }
        addModule("root", LogLevel::DEBUG);
        // addConsole(std::make_shared<StdoutConsole>());
    }

    void Logger::log(LogEvent::ptr event, LogLevel::Level level, const std::string &module_name)
    {
        Mutex::MutexLockGuardType guard(m_mutex);
        // 如果日志存在该模块
        if (m_modules.find(module_name) != m_modules.end())
        {
            // 日志级别大于等于模块的日志界别
            if (level >= m_modules[module_name])
            {
                event->setModuleName(module_name);
                event->setLevel(level);
                event->m_logger = shared_from_this();
                if (m_appenders.empty())
                {
                    std::cout << "warning: no console for logger:" << getName() << std::endl;
                }
                for (auto &i : m_appenders)
                {
                    i->log(event); // 就输出到对应的console
                }
            }
        }
        //std::cout << module_name << "not exist" << std::endl;
    }
    void Logger::debug(LogEvent::ptr event, const std::string &module_name)
    {
        log(event, LogLevel::DEBUG, module_name);
    }
    void Logger::info(LogEvent::ptr event, const std::string &module_name)
    {
        log(event, LogLevel::INFO, module_name);
    }
    void Logger::warn(LogEvent::ptr event, const std::string &module_name)
    {
        log(event, LogLevel::WARN, module_name);
    }
    void Logger::error(LogEvent::ptr event, const std::string &module_name)
    {
        log(event, LogLevel::ERROR, module_name);
    }
    void Logger::fatal(LogEvent::ptr event, const std::string &module_name)
    {
        log(event, LogLevel::FATAL, module_name);
    }

    void Logger::addConsole(LogConsole::ptr console)
    {
        Mutex::MutexLockGuardType guard(m_mutex);
        // 为console添加默认格式,logger有m_formatter,用logger的，没有就用默认的。
        if (!console->getFormatter())
        {
            LogFormatter::ptr fmt;

            if (m_formatter)
                fmt = m_formatter;
            else
                fmt = LogFormatter::ptr(new LogFormatter(LOG_DEFAULT_PATTERN));
            console->setFormatter(fmt);
        }
        m_appenders.push_back(console);
    }
    void Logger::delConsole(LogConsole::ptr console)
    {
        Mutex::MutexLockGuardType guard(m_mutex);
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it)
        {
            if (*it == console)
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::addModule(const std::string &name, LogLevel::Level level)
    {
        Mutex::MutexLockGuardType guard(m_mutex);
        if (!m_masks.empty() && !m_excludes.empty())
        {
            std::cout << "invalid logger config " << getName() << ":  exclude_modules and mask_modules can not be both set" << std::endl;
            throw std::logic_error("invalid logger config");
        }
        else if (!m_masks.empty())
        {
            if (m_masks.find(name) != m_masks.end())
                return; // 如果在mask中找到了，说明该模块已经被屏蔽了，不用添加了
        }
        else if(!m_excludes.empty())
        {
            if (m_excludes.find(name) == m_excludes.end())
                return; // 如果在exclude中找不到，说明该模块被屏蔽，不用添加了
        }
        if (m_modules.find(name) != m_modules.end())
        {
            //std::cout << "module " << name << " already exist" << std::endl;
            return;
        }
        m_modules[name] = level;
    }
    void Logger::delModule(const std::string &name)
    {
        Mutex::MutexLockGuardType guard(m_mutex);
        if (m_modules.find(name) == m_modules.end())
        {
            std::cout << "module " << name << " not exist" << std::endl;
            return;
        }
        m_modules.erase(name);
    }

    void Logger::modifyModule(const std::string &name, LogLevel::Level new_level)
    {
        if (m_modules.find(name) == m_modules.end())
        {
            addModule(name, new_level);
            return;
        }
        Mutex::MutexLockGuardType guard(m_mutex);
        m_modules[name] = new_level;
    }

    void Logger::maskModule(const std::string &name)
    {
        Mutex::MutexLockGuardType guard(m_mutex);
        if (m_modules.find(name) == m_modules.end())
        {
            std::cout << "module " << name << " not exist" << std::endl;
            return;
        }
        m_modules[name] = LogLevel::OFF;
    }

    void Logger::maskExcludeModule(const std::vector<std::string> &names)
    {
        Mutex::MutexLockGuardType guard(m_mutex);
        // 遍历所有指定的模块名称
        for (const auto &name : names)
        {
            // 检查模块是否存在
            if (m_modules.find(name) == m_modules.end())
            {
                std::cout << "module " << name << " not exist" << std::endl;
                continue; // 继续处理下一个模块
            }
        }

        // 遍历所有模块，将未在names中指定的模块都屏蔽掉
        for (auto it = m_modules.begin(); it != m_modules.end(); ++it)
        {
            // 如果当前模块在指定的names列表中，则跳过（不屏蔽）
            bool should_keep = false;
            for (const auto &name : names)
            {
                if (it->first == name)
                {
                    should_keep = true;
                    break;
                }
            }

            // 如果不在指定列表中，则屏蔽该模块
            if (!should_keep)
            {
                m_modules[it->first] = LogLevel::OFF;
            }
        }
    }

    std::string Logger::getName()
    {
        return m_name;
    }
    LogManager::LogManager()
    {
        // m_root.reset(new Logger("root"));
        // // m_root->addConsole(std::make_shared<StdoutConsole>());
        // addLogger(m_root);
    }
    Logger::ptr LogManager::getLogger(const std::string &name)
    {
        if (m_loggers.find(name) != m_loggers.end())
        {
            return m_loggers[name];
        }
        else
        {
            std::cout << "logger not found" << std::endl;
            throw std::invalid_argument("logger not found");
        }
    }

    struct LogConsoleDefine
    {
        enum Type
        {
            STDOUT,
            FILE,
            STDERR
        };
        Type type;
        std::string filename;
        bool operator==(const LogConsoleDefine &other) const
        {
            return type == other.type && filename == other.filename;
        }
    };
    struct LoggerDefine
    {
        std::string name;
        std::string formatter;
        std::vector<LogConsoleDefine> consoles;
        std::set<std::string> exclude_modules;
        std::set<std::string> mask_modules;
        bool operator==(const LoggerDefine &other) const
        {
            return name == other.name && formatter == other.formatter && consoles == other.consoles;
        }

        bool operator<(const LoggerDefine &other) const
        {
            return name < other.name;
        }
    };

    // 添加配置项
    static ConfigVar<std::set<LoggerDefine>>::ptr g_log_defines =
        zhao::ConfigMgr::getInstance()->add("logs", std::set<LoggerDefine>(), "logs config");

    /**
     * @brief 为logs配置项注册回调函数
     */
    class LogIniter
    {
    public:
        LogIniter()
        {
            
            g_log_defines->addListener([](const std::set<LoggerDefine> &old_value,
                                          const std::set<LoggerDefine> &new_value)
                                       {
              ///  std::cout << "on_logger_conf_changed"<<std::endl;
            for(auto& i : new_value) {
                auto it = old_value.find(i);
                zhao::Logger::ptr logger;
                if(it == old_value.end()) {
                    //没找到：新增logger
                    //logger = SYLAR_LOG_NAME(i.name);
                    logger = std::make_shared<zhao::Logger>(i.name);
                    LOGGER_ADD(logger);
                } else {
                    //找到了但是不相等：修改logger
                    if(!(i == *it)) {
                        logger = LOGGER_GET(i.name);
                    } else {
                        continue;
                    }
                }
                
                if(!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }
                if(!i.mask_modules.empty() && !i.exclude_modules.empty()){
                    std::cout<<"logger: "<<i.name<<" mask_modules and exclude_modules can not be set at the same time"<<std::endl;
                    throw std::invalid_argument("logger: " + i.name + " mask_modules and exclude_modules can not be set at the same time");
                }
                if(!i.mask_modules.empty()){
                    logger->getMasks() = i.mask_modules;
                }
                if(!i.exclude_modules.empty()){
                    logger->getExcludeModules() = i.exclude_modules;
                }

                logger->clearConsoles();
                for(auto& a : i.consoles) {
                    zhao::LogConsole::ptr ap;
                    if(a.type == LogConsoleDefine::FILE) {
                        ap.reset(new FileConsole(a.filename));
                    } else if(a.type == LogConsoleDefine::STDOUT) {
                        ap.reset(new StdoutConsole);
                    }
                    
                    logger->addConsole(ap);
                }
            }

            //旧的比新的多出来的，就是需要删除掉的
            for(auto& i : old_value) {
                auto it = new_value.find(i);
                if(it == new_value.end()) {
                    //删除logger
                    LOGGER_DEL(i.name);
                }
            } });
        }
    };
    static LogIniter s_log_initer; // 在main之前注册回调，loadYaml时加载配置，并调用回调

    template <>
    class LexicalCast<std::string, LoggerDefine>
    {
    public:
        LoggerDefine operator()(const std::string &v)
        {
            YAML::Node n = YAML::Load(v);
            LoggerDefine ld;

            if (!n["name"].IsDefined())
            {
                std::cout << "log config error: name is null, " << n
                          << std::endl;
                throw std::logic_error("log config name is null");
            }
            ld.name = n["name"].as<std::string>();

            // 处理formatter字段
            if (n["formatter"].IsDefined())
            {
                ld.formatter = n["formatter"].as<std::string>();
            }

            // 处理module.mask和module.mask_exclude字段
            if (n["module"].IsDefined())
            {
                if (n["module"]["mask"].IsDefined())
                {
                    for (size_t i = 0; i < n["module"]["mask"].size(); ++i)
                    {
                        ld.mask_modules.insert(n["module"]["mask"][i].as<std::string>());
                    }
                }

                if (n["module"]["mask_exclude"].IsDefined())
                {
                    for (size_t i = 0; i < n["module"]["mask_exclude"].size(); ++i)
                    {
                        ld.exclude_modules.insert(n["module"]["mask_exclude"][i].as<std::string>());
                    }
                }
            }

            // 处理appenders字段
            if (n["appenders"].IsDefined())
            {
                for (size_t x = 0; x < n["appenders"].size(); ++x)
                {
                    auto a = n["appenders"][x];
                    if (!a["type"].IsDefined())
                    {
                        std::cout << "log config error: appender type is null, " << a
                                  << std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();
                    LogConsoleDefine lcd;

                    if (type == "FileConsole")
                    {
                        lcd.type = LogConsoleDefine::FILE;
                        if (!a["file"].IsDefined())
                        {
                            std::cout << "log config error: fileappender file is null, " << a
                                      << std::endl;
                            continue;
                        }
                        lcd.filename = a["file"].as<std::string>();
                    }
                    else if (type == "StdoutConsole")
                    {
                        lcd.type = LogConsoleDefine::STDOUT;
                    }
                    else
                    {
                        std::cout << "log config error: appender type is invalid, " << a
                                  << std::endl;
                        continue;
                    }

                    ld.consoles.push_back(lcd);
                }
            }
            return ld;
        }
    };

    template <>
    class LexicalCast<LoggerDefine, std::string>
    {
    public:
        std::string operator()(const LoggerDefine &i)
        {
            YAML::Node n;
            n["name"] = i.name;

            if (!i.formatter.empty())
            {
                n["formatter"] = i.formatter; // 与YAML文件保持一致的拼写
            }

            // 序列化module.mask和module.mask_exclude
            if (!i.mask_modules.empty())
            {
                YAML::Node mask_node(YAML::NodeType::Sequence);
                for (const auto &module : i.mask_modules)
                {
                    mask_node.push_back(module);
                }
                n["module"]["mask"] = mask_node;
            }

            if (!i.exclude_modules.empty())
            {
                YAML::Node exclude_node(YAML::NodeType::Sequence);
                for (const auto &module : i.exclude_modules)
                {
                    exclude_node.push_back(module);
                }
                n["module"]["mask_exclude"] = exclude_node;
            }

            // 序列化appenders
            if (!i.consoles.empty())
            {
                YAML::Node appenders_node(YAML::NodeType::Sequence);
                for (const auto &a : i.consoles)
                {
                    YAML::Node na;
                    if (a.type == LogConsoleDefine::FILE)
                    {
                        na["type"] = "FileLogAppender";
                        na["file"] = a.filename;
                    }
                    else if (a.type == LogConsoleDefine::STDOUT)
                    {
                        na["type"] = "StdoutLogAppender";
                    }
                    else if (a.type == LogConsoleDefine::STDERR)
                    {
                        na["type"] = "StderrLogAppender";
                    }
                    appenders_node.push_back(na);
                }
                n["appenders"] = appenders_node;
            }

            std::stringstream ss;
            ss << n;
            return ss.str();
        }
    };
}
