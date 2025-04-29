#ifndef __CONFIG_H
#define __CONFIG_H
#include <string>
#include <memory>
#include "log.h"

namespace zhao
{
    class ConfigItemBase
    {
    protected:
        std::string m_name;
        std::string m_desc;
    public:
        typedef std::shared_ptr<ConfigItemBase> Ptr;
        ConfigItemBase(std::string name,std::string desc):m_name(name),m_desc(desc){}
        virtual ~ConfigItemBase(){}

        std::string getName() const{return m_name;}
        std::string getDesciption()const {return m_desc;}

        virtual void toString(void) = 0;
        virtual bool fromString(std::string& val) = 0;
    };
    
    template <class T>
    class ConfigItem:public ConfigItemBase{
    private:
        T m_value;

    public:
        ConfigItem(const std::string &name,const std::string &desc,const T& value)
        :ConfigItemBase(name,desc),m_value(value){}

        typedef std::shared_ptr<ConfigItem> Ptr;
        std::string toString() override{

        }
        bool fromString() override{

        }
    };
    
} // namespace zhao



#endif