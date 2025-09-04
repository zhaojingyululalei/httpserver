#ifndef __ZHAO_ADDRESS_HPP_
#define __ZHAO_ADDRESS_HPP_

#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <map>
namespace zhao
{

    class IPAddress;
    class Address;
    std::shared_ptr<IPAddress> createSubnetMask(int prefix_len);
    extern std::ostream &operator<<(std::ostream &os, const Address &addr);
    class Address
    {
    public:
        typedef std::shared_ptr<Address> ptr;
        Address() {}
        virtual ~Address() {}
        /**
         * @brief 返回协议族
         */
        int getFamily() const;
        virtual const sockaddr *getSocketAddress() const = 0;
        virtual sockaddr *getSocketAddress()  = 0;
        virtual socklen_t getAddrLen() const = 0;

        virtual std::string toString() const = 0;

        bool operator<(const Address &rhs) const;
        bool operator==(const Address &rhs) const ;
        bool operator!=(const Address &rhs) const;

        static Address::ptr create(const sockaddr *addr, socklen_t addrlen);
        /**
         * @brief 通过host地址返回对应条件的所有Address
         */
        static bool Lookup(std::vector<Address::ptr> &result, const std::string &host,
                           int family = AF_INET, int type = 0, int protocol = 0);
        /**
         * @brief 通过host地址返回对应条件的任意Address
         */
        static Address::ptr LookupAny(const std::string &host,
                                      int family = AF_INET, int type = 0, int protocol = 0);
        /**
         * @brief 通过host地址返回对应条件的任意IPAddress
         */
        static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string &host,
                                                             int family = AF_INET, int type = 0, int protocol = 0);

        /**
         * @brief 返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
         */
        static bool GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result,
                                          int family = AF_INET);
        /**
         * @brief 获取指定网卡的地址和子网掩码位数
         */
        static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>> &result, const std::string &iface, int family = AF_INET);

        friend std::ostream &operator<<(std::ostream &os, const Address &addr);
    };

    

    

}

#endif