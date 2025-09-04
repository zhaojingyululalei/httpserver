#include "address/ipaddress.hpp"
#include "address/edian.hpp"
#include "log/logger.hpp"
#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#define MODULE_NAME "socket"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))
namespace zhao
{

    IPAddress::ptr IPAddress::create(const std::string &address, uint16_t port )
    {
        addrinfo hints, *results;
        memset(&hints, 0, sizeof(addrinfo));

        hints.ai_flags = AI_NUMERICHOST; // address是ip形式的字符串，不是域名，略过DNS解析
        hints.ai_family = AF_UNSPEC;
        // 解析得到的结果以链表形式保存在results中
        int ret = getaddrinfo(address.c_str(), NULL, &hints, &results);
        if (ret)
        {
            warn << "IPAddress::Create(" << address
                 << ", " << port << ") error=" << ret
                 << " errno=" << errno << " errstr=" << strerror(errno);
            return nullptr;
        }

        try
        {
            IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                Address::create(results->ai_addr, (socklen_t)results->ai_addrlen));
            if (result)
            {
                result->setPort(port);
            }
            freeaddrinfo(results);
            return result;
        }
        catch (...)
        {
            freeaddrinfo(results);
            return nullptr;
        }
    }
    IPv4Address::IPv4Address(const sockaddr_in &address)
    {
        m_addr = address;
    }

    IPv4Address::IPv4Address(uint32_t address, uint16_t port)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = Endian::autoSwap(port);
        m_addr.sin_addr.s_addr = Endian::autoSwap(address);
        setPort(port);
    }

    IPv4Address::IPv4Address(const std::string &address, uint16_t port)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = Endian::autoSwap(port);
        inet_pton(AF_INET, address.c_str(), &m_addr.sin_addr);
        setPort(port);
    }

    IPv4Address::IPv4Address(uint16_t port) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = Endian::autoSwap(port);
        m_addr.sin_addr.s_addr = INADDR_ANY;
        setPort(port);
    }

    const sockaddr *IPv4Address::getSocketAddress() const
    {
        return (sockaddr *)&m_addr;
    }

    socklen_t IPv4Address::getAddrLen() const
    {
        return sizeof(m_addr);
    }

    IPAddress::ptr IPv4Address::broadcastAddress(IPAddress::ptr subnet_mask)
    {
        auto ipv4_mask = std::dynamic_pointer_cast<IPv4Address>(subnet_mask);
        if (!ipv4_mask)
        {
            return nullptr;
        }
        uint32_t addr = ntohl(m_addr.sin_addr.s_addr);
        uint32_t mask = ntohl(ipv4_mask->m_addr.sin_addr.s_addr);
        uint32_t broadcast = addr | ~mask;
        return std::make_shared<IPv4Address>(broadcast, 0);
    }

    IPAddress::ptr IPv4Address::networkAddress(IPAddress::ptr subnet_mask)
    {
        auto ipv4_mask = std::dynamic_pointer_cast<IPv4Address>(subnet_mask);
        if (!ipv4_mask)
        {
            return nullptr;
        }
        uint32_t addr = ntohl(m_addr.sin_addr.s_addr);
        uint32_t mask = ntohl(ipv4_mask->m_addr.sin_addr.s_addr);
        uint32_t network = addr & mask;
        return std::make_shared<IPv4Address>(network, 0);
    }

    std::string IPv4Address::toString() const
    {
        std::stringstream ss;
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &m_addr.sin_addr, buf, sizeof(buf));
        ss << buf << ":" << getPort().getHostSeqPort();
        return ss.str();
    }
    uint32_t IPv4Address::toNetSeqAddress() const
    {
        return m_addr.sin_addr.s_addr;
    }
    uint32_t IPv4Address::toHostSeqAddress() const
    {
        return Endian::autoSwap(m_addr.sin_addr.s_addr);
    }
    // IPv6Address implementation
    IPv6Address::IPv6Address(const sockaddr_in6 &address)
    {
        m_addr = address;
    }

    IPv6Address::IPv6Address(uint32_t address, uint16_t port)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
        m_addr.sin6_port = htons(port);
        m_addr.sin6_addr = in6addr_any;
        setPort(port);
    }

    IPv6Address::IPv6Address(const std::string &address, uint16_t port)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
        m_addr.sin6_port = htons(port);
        inet_pton(AF_INET6, address.c_str(), &m_addr.sin6_addr);
        setPort(port);
    }

    const sockaddr *IPv6Address::getSocketAddress() const
    {
        return (sockaddr *)&m_addr;
    }

    socklen_t IPv6Address::getAddrLen() const
    {
        return sizeof(m_addr);
    }

    IPAddress::ptr IPv6Address::broadcastAddress(IPAddress::ptr subnet_mask)
    {
        // IPv6 doesn't support broadcast
        return nullptr;
    }

    IPAddress::ptr IPv6Address::networkAddress(IPAddress::ptr subnet_mask)
    {
        auto ipv6_mask = std::dynamic_pointer_cast<IPv6Address>(subnet_mask);
        if (!ipv6_mask)
        {
            return nullptr;
        }
        IPv6Address::ptr result = std::make_shared<IPv6Address>(*this);
        uint8_t *addr = (uint8_t *)&result->m_addr.sin6_addr;
        uint8_t *mask = (uint8_t *)&ipv6_mask->m_addr.sin6_addr;
        for (int i = 0; i < 16; ++i)
        {
            addr[i] &= mask[i];
        }
        return result;
    }

    std::string IPv6Address::toString() const
    {
        std::stringstream ss;
        char buf[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &m_addr.sin6_addr, buf, sizeof(buf));
        ss << "[" << buf << "]:" << getPort().getHostSeqPort();
        return ss.str();
    }

    std::ostream &operator<<(std::ostream &os, const IPAddress &addr)
    {
        return os << addr.toString();
    }
    std::ostream &operator<<(std::ostream &os, const IPv4Address &addr)
    {
        return os << addr.toString();
    }
    std::ostream &operator<<(std::ostream &os, const IPv6Address &addr)
    {
        return os << addr.toString();
    }
}