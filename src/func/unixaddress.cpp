#include "address/unixaddress.hpp"
#include <string.h>
#include <sstream>

namespace zhao
{
    static const size_t s_max_path_size = sizeof(((sockaddr_un *)0)->sun_path) - 1;
    UnixAddress::UnixAddress()
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_length = 0;
    }

    UnixAddress::~UnixAddress()
    {
    }

    UnixAddress::UnixAddress(const std::string &path)
    {
        if (path.size() > s_max_path_size)
        {
            throw std::invalid_argument("Path too long");
        }
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        strncpy(m_addr.sun_path, path.c_str(), sizeof(m_addr.sun_path) - 1);
        m_length = offsetof(sockaddr_un, sun_path) + path.size();
    }

    const sockaddr *UnixAddress::getSocketAddress() const
    {
        return (sockaddr *)&m_addr;
    }

    socklen_t UnixAddress::getAddrLen() const
    {
        return m_length;
    }

    std::string UnixAddress::getPath() const
    {
        return m_addr.sun_path;
    }

    std::string UnixAddress::toString() const
    {
        std::stringstream ss;
        ss << "unix:" << m_addr.sun_path;
        return ss.str();
    }
    std::ostream &operator<<(std::ostream &os, const UnixAddress &addr)
    {
        return os << addr.toString();
    }
}
