#ifndef __ZHAO_IPADDRESS_HPP_
#define __ZHAO_IPADDRESS_HPP_

#include "address.hpp"
#include "port.hpp"
#include <string>
namespace zhao
{
    class IPAddress;
    class IPv4Address;
    class IPv6Address;
    extern std::ostream &operator<<(std::ostream &os, const IPAddress &addr);
    extern std::ostream &operator<<(std::ostream &os, const IPv4Address &addr);
    extern std::ostream &operator<<(std::ostream &os, const IPv6Address &addr);

    class IPAddress:public Address
    {
    public:
        typedef std::shared_ptr<IPAddress> ptr;
        IPAddress() = default;
        virtual ~IPAddress(){}
        static IPAddress::ptr create(const std::string& address, uint16_t port = 0);
        /** 
         * @brief 获取该地址的广播地址
         */
        virtual IPAddress::ptr broadcastAddress(IPAddress::ptr subnet_mask) = 0;
        /**
         * @brief 获取网段地址
         */
        virtual IPAddress::ptr networkAddress(IPAddress::ptr subnet_mask) = 0;
        /**
         * @brief 设置端口
         * @param port 主机序端口
         */
        void setPort(Port &port){
            m_port = port;
        }
        void setPort(port_t port){
            m_port.setPort(port);
        }
        Port getPort() const {
            return m_port;
        }
        friend std::ostream &operator<<(std::ostream &os, const IPAddress &addr);
    private:
        Port m_port;
    };

    

    class IPv4Address:public IPAddress
    {
    public:
        typedef std::shared_ptr<IPv4Address> ptr;
        IPv4Address(const sockaddr_in& address);
        IPv4Address(uint32_t address, uint16_t port ); 
        explicit IPv4Address(uint16_t port = 0);  // 新增，用于创建任意地址的构造函数
        explicit IPv4Address(const std::string& address, uint16_t port = 0);
        
        static IPv4Address::ptr create(const std::string& address = "0.0.0.0",port_t port= 0){
            return std::make_shared<IPv4Address>(address, port);
        }
        const sockaddr* getSocketAddress()  const override;
        sockaddr *getSocketAddress() override
        {
            return (sockaddr*)&m_addr;
        }
        socklen_t getAddrLen() const override;
        IPAddress::ptr broadcastAddress(IPAddress::ptr subnet_mask) override;
        IPAddress::ptr networkAddress(IPAddress::ptr subnet_mask) override;
        std::string toString() const override;
        uint32_t toNetSeqAddress() const;
        uint32_t toHostSeqAddress() const;
        friend std::ostream &operator<<(std::ostream &os, const IPv4Address &addr);
    private:
        sockaddr_in m_addr;
    };

    class IPv6Address:public IPAddress
    {
    public:
        typedef std::shared_ptr<IPv6Address> ptr;
        IPv6Address() = default;
        IPv6Address(const sockaddr_in6& address);
        IPv6Address(uint32_t address , uint16_t port = 0);
        IPv6Address(const std::string& address , uint16_t port = 0);
        static IPv6Address::ptr create(const std::string& address = "::", port_t port = 0) {
            return std::make_shared<IPv6Address>(address, port);
        }
        sockaddr* getSocketAddress()  override
        {
            return (sockaddr*)&m_addr;
        }
        const sockaddr* getSocketAddress()  const override;
        socklen_t getAddrLen() const override;
        /**
         * @brief 获取广播地址
         */
        IPAddress::ptr broadcastAddress(IPAddress::ptr subnet_mask) override;
        /**
         * @brief 获取网段地址
         */
        IPAddress::ptr networkAddress(IPAddress::ptr subnet_mask) override;
        std::string toString() const override;

        friend std::ostream &operator<<(std::ostream &os, const IPv6Address &addr);
    private:
        sockaddr_in6 m_addr;
    };
    


}

#endif