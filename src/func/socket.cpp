#include "socket.hpp"
#include "fdmanager.hpp"
#include "log/logger.hpp"
#include "address/unixaddress.hpp"
#include "address/ipaddress.hpp"
#include "hook.hpp"
#include "config/configvar.hpp"
#include "scheduler/scheduler.hpp"
#define MODULE_NAME "socket"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))
namespace zhao
{
    Socket::ptr Socket::CreateTCP(Address::ptr address)
    {
        Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDP(Address::ptr address)
    {
        Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
        sock->newSock();
        sock->m_isConnected = true;
        return sock;
    }

    Socket::ptr Socket::CreateTCPSocket()
    {
        Socket::ptr sock(new Socket(IPv4, TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDPSocket()
    {
        Socket::ptr sock(new Socket(IPv4, UDP, 0));
        sock->newSock();
        sock->m_isConnected = true;
        return sock;
    }

    Socket::ptr Socket::CreateTCPSocket6()
    {
        Socket::ptr sock(new Socket(IPv6, TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDPSocket6()
    {
        Socket::ptr sock(new Socket(IPv6, UDP, 0));
        sock->newSock();
        sock->m_isConnected = true;
        return sock;
    }

    Socket::ptr Socket::CreateUnixTCPSocket()
    {
        Socket::ptr sock(new Socket(UNIX, TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUnixUDPSocket()
    {
        Socket::ptr sock(new Socket(UNIX, UDP, 0));
        return sock;
    }
    Socket::Socket(int family, int type, int protocol) : m_family(family), m_type(type), m_protocol(protocol)
    {
    }

    Socket::~Socket()
    {
        close();
    }

    uint64_t Socket::getSendTimeout()
    {
        FileDescription::ptr ctx = FdMgr::getInstance()->get(m_sock); // 初始化描述符并设置非阻塞
        if (ctx)
        {
            return ctx->getTimeout(SO_SNDTIMEO);
        }
        return -1;
    }

    void Socket::setSendTimeout(uint64_t v)
    {
        struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
        setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
    }

    uint64_t Socket::getRecvTimeout()
    {
        FileDescription::ptr ctx = FdMgr::getInstance()->get(m_sock);
        if (ctx)
        {
            return ctx->getTimeout(SO_RCVTIMEO);
        }
        return -1;
    }

    void Socket::setRecvTimeout(uint64_t v)
    {
        struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
        setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
    }

    bool Socket::getOption(int level, int option, void *result, socklen_t *len)
    {
        int ret = getsockopt(m_sock, level, option, result, (socklen_t *)len);
        if (ret)
        {
            warn << "getOption sock=" << m_sock
                 << " level=" << level << " option=" << option
                 << " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    bool Socket::setOption(int level, int option, const void *result, socklen_t len)
    {
        int ret;
        ret = setsockopt(m_sock, level, option, result, (socklen_t)len);
        if (ret)
        {
            warn << "setOption sock=" << m_sock
                 << " level=" << level << " option=" << option
                 << " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    Socket::ptr Socket::accept()
    {
        // 客户端sock对象
        Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
        int clientsock = ::accept(m_sock, nullptr, nullptr);
        if (clientsock == -1)
        {
            error << "accept(" << m_sock << ") errno="
                  << errno << " errstr=" << strerror(errno);
            return nullptr;
        }
        if (sock->init(clientsock))
        {
            return sock;
        }
        return nullptr;
    }

    bool Socket::bind(const Address::ptr addr)
    {
        if (!isValid())
        {
            newSock();
            if (!isValid())
            {
                return false;
            }
        }

        if (addr->getFamily() != m_family)
        {
            error << "bind sock.family("
                  << m_family << ") addr.family(" << addr->getFamily()
                  << ") not equal, addr=" << addr->toString();
            return false;
        }

        // 安全向下转型（downcast）的操作符，它会检查对象的实际类型是否与目标类型兼容。
        UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(addr); // 如果是其他类型的地址，返回值是null
        if (uaddr)
        {                                     // 如果是unix地址
            unlink(uaddr->getPath().c_str()); // 删除该文件，防止上次服务异常退出导致文件未删除
        }

        if (::bind(m_sock, addr->getSocketAddress(), addr->getAddrLen()))
        {
            error << "bind error errrno=" << errno
                  << " errstr=" << strerror(errno);
            return false;
        }
        getLocalAddress();
        return true;
    }

    bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms)
    {
        m_remoteAddress = addr;
        if (!isValid())
        {
            newSock();
            if (!isValid())
            {
                return false;
            }
        }

        if (addr->getFamily() != m_family)
        {
            error << "connect sock.family("
                  << m_family << ") addr.family(" << addr->getFamily()
                  << ") not equal, addr=" << addr->toString();
            return false;
        }

        if (timeout_ms == (uint64_t)-1) // 用户想用非阻塞
        {
            // 用户手动设置非阻塞
            int flags = fcntl(m_sock, F_GETFL, 0);
            if (flags == -1)
                return false;
            fcntl(m_sock, F_SETFL, flags | O_NONBLOCK);
        }
        else
        {
            ConfigMgr::getInstance()->lookup<uint64_t>("tcp.connect.timeout")->setValue(timeout_ms);
        }

        if (::connect(m_sock, addr->getSocketAddress(), addr->getAddrLen()))
        {
            error << "sock=" << m_sock << " connect(" << addr->toString()
                  << ") error errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }

        m_isConnected = true;
        getRemoteAddress();
        getLocalAddress();
        return true;
    }

    bool Socket::reconnect(uint64_t timeout_ms)
    {
        if (!m_remoteAddress)
        {
            error << "reconnect m_remoteAddress is null";
            return false;
        }
        m_localAddress.reset();
        return connect(m_remoteAddress, timeout_ms);
    }

    bool Socket::listen(int backlog)
    {
        if (!isValid())
        {
            error << "listen error sock=-1";
            return false;
        }
        if (::listen(m_sock, backlog))
        {
            error << "listen error errno=" << errno
                  << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    bool Socket::close()
    {
        if (!m_isConnected && m_sock == -1)
        {
            return true;
        }
        m_isConnected = false;
        if (m_sock != -1)
        {
            ::close(m_sock);
            m_sock = -1;
        }
        return false;
    }

    int Socket::send(const void *buffer, size_t length, int flags)
    {
        if (isConnected())
        {
            return ::send(m_sock, buffer, length, flags);
        }
        return -1;
    }

    int Socket::send(const iovec *buffers, size_t length, int flags)
    {
        if (isConnected())
        {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec *)buffers;
            msg.msg_iovlen = length;
            return ::sendmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    int Socket::sendTo(const void *buffer, size_t length, const Address::ptr to, int flags)
    {
        if (isConnected())
        {
            return ::sendto(m_sock, buffer, length, flags, to->getSocketAddress(), to->getAddrLen());
        }
        return -1;
    }

    int Socket::sendTo(const iovec *buffers, size_t length, const Address::ptr to, int flags)
    {
        if (isConnected())
        {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec *)buffers;
            msg.msg_iovlen = length;
            msg.msg_name = to->getSocketAddress();
            msg.msg_namelen = to->getAddrLen();
            return ::sendmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    int Socket::recv(void *buffer, size_t length, int flags)
    {
        if (isConnected())
        {
            return ::recv(m_sock, buffer, length, flags);
        }
        return -1;
    }

    int Socket::recv(iovec *buffers, size_t length, int flags)
    {
        if (isConnected())
        {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec *)buffers;
            msg.msg_iovlen = length;
            return ::recvmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    int Socket::recvFrom(void *buffer, size_t length, Address::ptr from, int flags)
    {
        if (isConnected())
        {
            socklen_t len = from->getAddrLen();
            return ::recvfrom(m_sock, buffer, length, flags, from->getSocketAddress(), &len);
        }
        return -1;
    }

    int Socket::recvFrom(iovec *buffers, size_t length, Address::ptr from, int flags)
    {
        if (isConnected())
        {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec *)buffers;
            msg.msg_iovlen = length;
            msg.msg_name = from->getSocketAddress();
            msg.msg_namelen = from->getAddrLen();
            return ::recvmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    Address::ptr Socket::getRemoteAddress()
    {
        if (m_remoteAddress)
        {
            return m_remoteAddress;
        }

        Address::ptr result;
        switch (m_family)
        {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            throw std::runtime_error("Unknown address family");
            break;
        }
        socklen_t addrlen = result->getAddrLen();
        if (getpeername(m_sock, result->getSocketAddress(), &addrlen))
        {

            return nullptr;
        }
        if (m_family == AF_UNIX)
        {
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrLen(addrlen);
        }
        m_remoteAddress = result;
        return m_remoteAddress;
    }

    Address::ptr Socket::getLocalAddress()
    {
        if (m_localAddress)
        {
            return m_localAddress;
        }

        Address::ptr result;
        switch (m_family)
        {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            throw std::runtime_error("Unknown address family");
            break;
        }
        socklen_t addrlen = result->getAddrLen();
        if (getsockname(m_sock, result->getSocketAddress(), &addrlen))
        {
            error << "getsockname error sock=" << m_sock
                  << " errno=" << errno << " errstr=" << strerror(errno);
            return nullptr;
        }
        if (m_family == AF_UNIX)
        {
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrLen(addrlen);
        }
        m_localAddress = result;
        return m_localAddress;
    }

    bool Socket::isValid() const
    {
        return m_sock != -1;
    }

    int Socket::getError()
    {
        int _error = 0;
        socklen_t len = sizeof(_error);
        if (!getOption(SOL_SOCKET, SO_ERROR, &_error, &len))
        {
            _error = errno;
        }
        return _error;
    }

    std::string Socket::toString() const
    {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }

    bool Socket::cancelRead()
    {
        return Scheduler::getThis()->cancelEvent(m_sock, IOManager::READ);
    }

    bool Socket::cancelWrite()
    {
        return Scheduler::getThis()->cancelEvent(m_sock, IOManager::WRITE);
    }

    bool Socket::cancelAccept()
    {
        return Scheduler::getThis()->cancelEvent(m_sock, IOManager::READ);
    }

    bool Socket::cancelAll()
    {
        return Scheduler::getThis()->cancelAll(m_sock);
    }

    void Socket::initSock()
    {
        int val = 1;
        // 端口复用，time_wait不用等待立即重启
        setOption(SOL_SOCKET, SO_REUSEADDR, val);
        if (m_type == SOCK_STREAM)
        {
            // 禁用Nagle 算法
            /**
             * Nagle算法：
             * 自动合并小的 TCP 数据包（等待直到收到前一个包的 ACK 或积累足够数据）
             * 减少网络中小数据包的数量，提高网络效率
             */
            setOption(IPPROTO_TCP, TCP_NODELAY, val);
        }
    }

    void Socket::newSock()
    {
        m_sock = socket(m_family, m_type, m_protocol);
        if (m_sock != -1)
        {
            initSock();
        }
        else
        {
            error << "socket(" << m_family
                  << ", " << m_type << ", " << m_protocol << ") errno="
                  << errno << " errstr=" << strerror(errno);
        }
    }

    bool Socket::init(int sock)
    {
        FileDescription::ptr ctx = FdMgr::getInstance()->get(sock);
        if (ctx && ctx->isSocket() && !ctx->isClose())
        {
            m_sock = sock;
            m_isConnected = true;
            initSock();
            getLocalAddress();
            getRemoteAddress();
            return true;
        }
        return false;
    }

    std::ostream &operator<<(std::ostream &os, const Socket &sock)
    {
        os << "[Socket sock=" << sock.m_sock
           << " is_connected=" << sock.m_isConnected
           << " family=" << sock.m_family
           << " type=" << sock.m_type
           << " protocol=" << sock.m_protocol;

        if (sock.m_localAddress)
        {
            os << " local=" << sock.m_localAddress->toString();
        }

        if (sock.m_remoteAddress)
        {
            os << " remote=" << sock.m_remoteAddress->toString();
        }

        os << "]";
        return os;
    }

} // namespace zhao
