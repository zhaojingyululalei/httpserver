#include "address/address.hpp"
#include "ipaddress.hpp"
#include "log/logger.hpp"
#include <netdb.h>
#include <ifaddrs.h>
#define MODULE_NAME "socket"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))
namespace zhao
{

    std::shared_ptr<IPAddress> createSubnetMask(int prefix_len)
    {
        if (prefix_len < 0 || prefix_len > 32)
        {
            throw std::invalid_argument("Invalid prefix length");
        }
        uint32_t mask = (prefix_len == 0) ? 0 : (0xFFFFFFFF << (32 - prefix_len));
        return std::make_shared<IPv4Address>(mask, 0);
    }

    int Address::getFamily() const
    {
        return getSocketAddress()->sa_family;
    }

    bool Address::operator<(const Address &rhs) const
    {
        socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
        int result = memcmp(getSocketAddress(), rhs.getSocketAddress(), minlen);
        if (result < 0)
        {
            return true;
        }
        else if (result > 0)
        {
            return false;
        }
        return getAddrLen() < rhs.getAddrLen();
    }

    bool Address::operator==(const Address &rhs) const
    {
        return getAddrLen() == rhs.getAddrLen() &&
               memcmp(getSocketAddress(), rhs.getSocketAddress(), getAddrLen()) == 0;
    }

    bool Address::operator!=(const Address &rhs) const
    {
        return !(*this == rhs);
    }

    // 工厂函数，在这个接口创建对应类型的地址
    Address::ptr Address::create(const sockaddr *addr, socklen_t addrlen)
    {
        if (addr == nullptr)
        {
            return nullptr;
        }

        Address::ptr result;
        switch (addr->sa_family)
        {
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in *)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6 *)addr));
            break;
        default:
            throw std::invalid_argument("Address::create invalid family");
            break;
        }
        return result;
    }
    /**
     * std::vector<Address::ptr> result;
     * bool success = Address::Lookup(result, "192.168.1.1:80", AF_INET, SOCK_STREAM, IPPROTO_TCP);*/
    bool Address::Lookup(std::vector<Address::ptr> &result, const std::string &host,
                         int family, int type, int protocol)
    {
        addrinfo hints, *results, *next;
        hints.ai_flags = 0;
        hints.ai_family = family;
        hints.ai_socktype = type;
        hints.ai_protocol = protocol;
        hints.ai_addrlen = 0;
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;

        std::string node;
        const char *service = NULL;

        // 查看是不是ipv6格式，例如"[::1]:8080"
        if (!host.empty() && host[0] == '[')
        {
            const char *endipv6 = (const char *)memchr(host.c_str() + 1, ']', host.size() - 1);
            if (endipv6)
            {
                // TODO check out of range
                if (*(endipv6 + 1) == ':')
                {
                    service = endipv6 + 2;
                }
                node = host.substr(1, endipv6 - host.c_str() - 1);
            }
        }

        // 检查是不是ipv4格式,例如"192.168.1.1:80"
        if (node.empty())
        {
            service = (const char *)memchr(host.c_str(), ':', host.size());
            if (service)
            {
                if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1))
                {
                    node = host.substr(0, service - host.c_str());
                    ++service;
                }
            }
        } // node="192.168.1.1";service = "80"

        if (node.empty())
        {
            node = host;
        }
        // hints 指定了 AF_INET + SOCK_STREAM + IPPROTO_TCP。
        // node="192.168.1.1";service = "80"
        int ret = getaddrinfo(node.c_str(), service, &hints, &results);
        if (ret)
        {
            dbg << "Address::Lookup getaddress(" << host << ", "
                << family << ", " << type << ") err=" << ret << " errstr="
                << gai_strerror(ret);
            return false;
        }

        // 把result放在vector中
        next = results;
        while (next)
        {
            result.push_back(create(next->ai_addr, (socklen_t)next->ai_addrlen));
            std::cout << "addr=" << ((sockaddr_in*)next->ai_addr)->sin_addr.s_addr<<" port:" << ntohs(((sockaddr_in*)next->ai_addr)->sin_port) << std::endl;
            //dbg <<"addr"<< ((sockaddr_in*)next->ai_addr)->sin_addr.s_addr;
            next = next->ai_next;
        }

        freeaddrinfo(results);
        return !result.empty();
    }
    /**
     * @brief 通过host地址返回对应条件的任意Address
     */
    Address::ptr Address::LookupAny(const std::string &host,
                                    int family, int type, int protocol)
    {
        std::vector<Address::ptr> result;
        if (Lookup(result, host, family, type, protocol))
        {
            return result[0];
        }
        return nullptr;
    }
    /**
     * @brief 通过host地址返回对应条件的任意IPAddress
     */
    std::shared_ptr<IPAddress> Address::LookupAnyIPAddress(const std::string &host,
                                                           int family, int type, int protocol)
    {
        std::vector<Address::ptr> result;
        if (Lookup(result, host, family, type, protocol))
        {

            for (auto &i : result)
            {
                IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
                if (v)
                {
                    return v;
                }
            }
        }
        return nullptr;
    }
    template <class T>
    static uint32_t CountBytes(T value)
    {
        uint32_t result = 0;
        for (; value; ++result)
        {
            value &= value - 1;
        }
        return result;
    }
    /**
     * @brief 返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
     */
    bool Address::GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result,
                                        int family )
    {
        struct ifaddrs *next, *results;
        if (getifaddrs(&results) != 0)
        {
            dbg << "Address::GetInterfaceAddresses getifaddrs "
                   " err="
                << errno << " errstr=" << strerror(errno);
            return false;
        }

        try
        {
            for (next = results; next; next = next->ifa_next)
            {
                Address::ptr addr;
                uint32_t prefix_len = ~0u;
                if (family != AF_UNSPEC && family != next->ifa_addr->sa_family)
                {
                    continue;
                }
                switch (next->ifa_addr->sa_family)
                {
                case AF_INET:
                {
                    addr = create(next->ifa_addr, sizeof(sockaddr_in));
                    uint32_t netmask = ((sockaddr_in *)next->ifa_netmask)->sin_addr.s_addr;
                    prefix_len = CountBytes(netmask);
                }
                break;
                case AF_INET6:
                {
                    addr = create(next->ifa_addr, sizeof(sockaddr_in6));
                    in6_addr &netmask = ((sockaddr_in6 *)next->ifa_netmask)->sin6_addr;
                    prefix_len = 0;
                    for (int i = 0; i < 16; ++i)
                    {
                        prefix_len += CountBytes(netmask.s6_addr[i]);
                    }
                }
                break;
                default:
                    break;
                }

                if (addr)
                {
                    result.insert(std::make_pair(next->ifa_name,
                                                 std::make_pair(addr, prefix_len)));
                }
            }
        }
        catch (...)
        {
            error << "Address::GetInterfaceAddresses exception";
            freeifaddrs(results);
            return false;
        }
        freeifaddrs(results);
        return !result.empty();
    }
    /**
     * @brief 获取指定网卡的地址和子网掩码位数
     */
    bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>> &result, const std::string &iface, int family )
    {
        if (iface.empty() || iface == "*")
        {
            if (family == AF_INET || family == AF_UNSPEC)
            {
                result.push_back(std::make_pair(Address::ptr(new IPv4Address), 0u));  // 使用新的构造函数
            }
            if (family == AF_INET6 || family == AF_UNSPEC)
            {
                result.push_back(std::make_pair(Address::ptr(new IPv6Address), 0u));
            }
            return true;
        }

        std::multimap<std::string, std::pair<Address::ptr, uint32_t>> results;

        if (!GetInterfaceAddresses(results, family))
        {
            return false;
        }

        auto its = results.equal_range(iface);
        for (; its.first != its.second; ++its.first)
        {
            result.push_back(its.first->second);
        }
        return !result.empty();
    }
    std::ostream &operator<<(std::ostream &os, const Address &addr)
    {
        return os << addr.toString();
    }
}
