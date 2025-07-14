
#ifndef __SOCKET_H
#define __SOCKET_H

#include <memory>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "address.h"
#include "noncopyable.h"

namespace zhao {

/**
 * @brief Socket封装类
 */
class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    /**
     * @brief Socket类型
     */
    enum Type {
        /// TCP类型
        TCP = SOCK_STREAM,
        /// UDP类型
        UDP = SOCK_DGRAM
    };

    /**
     * @brief Socket协议簇
     */
    enum Family {
        /// IPv4 socket
        IPv4 = AF_INET,
        /// IPv6 socket
        IPv6 = AF_INET6,
        /// Unix socket
        UNIX = AF_UNIX,
    };

    /**
     * @brief 创建TCP Socket(满足地址类型)
     */
    static Socket::ptr  createTCP(zhao::Address::ptr address);

    /**
     * @brief 创建UDP Socket(满足地址类型)
     */
    static Socket::ptr  createUDP(zhao::Address::ptr address);

    /**
     * @brief 创建IPv4的TCP Socket
     */
    static Socket::ptr  createTCPSocket();

    /**
     * @brief 创建IPv4的UDP Socket
     */
    static Socket::ptr  createUDPSocket();

    /**
     * @brief 创建IPv6的TCP Socket
     */
    static Socket::ptr  createTCPSocket6();

    /**
     * @brief 创建IPv6的UDP Socket
     */
    static Socket::ptr  createUDPSocket6();

    /**
     * @brief 创建Unix的TCP Socket
     */
    static Socket::ptr  createUnixTCPSocket();

    /**
     * @brief 创建Unix的UDP Socket
     */
    static Socket::ptr  createUnixUDPSocket();

    /**
     * @brief Socket构造函数
     */
    Socket(int family, int type, int protocol = 0);

    /**
     * @brief 析构函数
     */
    virtual ~Socket();

    /**
     * @brief 获取发送超时时间(毫秒)
     */
    int64_t getSendTimeout();

    /**
     * @brief 设置发送超时时间(毫秒)
     */
    void setSendTimeout(int64_t v);

    /**
     * @brief 获取接受超时时间(毫秒)
     */
    int64_t getRecvTimeout();

    /**
     * @brief 设置接受超时时间(毫秒)
     */
    void setRecvTimeout(int64_t v);

    /**
     * @brief 获取sockopt @see getsockopt
     */
    bool getOption(int level, int option, void* result, socklen_t* len);

    /**
     * @brief 获取sockopt模板 @see getsockopt
     */
    template<class T>
    bool getOption(int level, int option, T& result) {
        socklen_t length = sizeof(T);
        return getOption(level, option, &result, &length);
    }

    /**
     * @brief 设置sockopt @see setsockopt
     */
    bool setOption(int level, int option, const void* result, socklen_t len);

    /**
     * @brief 设置sockopt模板 @see setsockopt
     */
    template<class T>
    bool setOption(int level, int option, const T& value) {
        return setOption(level, option, &value, sizeof(T));
    }

    /**
     * @brief 接收connect链接
     */
    virtual Socket::ptr accept();

    /**
     * @brief 绑定地址
     */
    virtual bool bind(const Address::ptr addr);

    /**
     * @brief 连接地址
     */
    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);

    virtual bool reconnect(uint64_t timeout_ms = -1);

    /**
     * @brief 监听socket
     */
    virtual bool listen(int backlog = SOMAXCONN);

    /**
     * @brief 关闭socket
     */
    virtual bool close();

    /**
     * @brief 发送数据
     */
    virtual int send(const void* buffer, size_t length, int flags = 0);

    /**
     * @brief 发送数据
     */
    virtual int send(const iovec* buffers, size_t length, int flags = 0);

    /**
     * @brief 发送数据
     */
    virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);

    /**
     * @brief 发送数据
     */
    virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0);

    /**
     * @brief 接受数据
     */
    virtual int recv(void* buffer, size_t length, int flags = 0);

    /**
     * @brief 接受数据
     */
    virtual int recv(iovec* buffers, size_t length, int flags = 0);

    /**
     * @brief 接受数据
     */
    virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);

    /**
     * @brief 接受数据
     */
    virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0);

    /**
     * @brief 获取远端地址
     */
    Address::ptr getRemoteAddress();

    /**
     * @brief 获取本地地址
     */
    Address::ptr getLocalAddress();

    /**
     * @brief 获取协议簇
     */
    int getFamily() const { return m_family;}

    /**
     * @brief 获取类型
     */
    int getType() const { return m_type;}

    /**
     * @brief 获取协议
     */
    int getProtocol() const { return m_protocol;}

    /**
     * @brief 返回是否连接
     */
    bool isConnected() const { return m_isConnected;}

    /**
     * @brief 是否有效(m_sock != -1)
     */
    bool isValid() const;

    /**
     * @brief 返回Socket错误
     */
    int getError();

    /**
     * @brief 输出信息到流中
     */
    virtual std::ostream& dump(std::ostream& os) const;

    virtual std::string toString() const;

    /**
     * @brief 返回socket句柄
     */
    int getSocket() const { return m_sock;}

    /**
     * @brief 取消读
     */
    bool cancelRead();

    /**
     * @brief 取消写
     */
    bool cancelWrite();

    /**
     * @brief 取消accept
     */
    bool cancelAccept();

    /**
     * @brief 取消所有事件
     */
    bool cancelAll();
protected:
    /**
     * @brief 初始化socket
     */
    void initSock();

    /**
     * @brief 创建socket
     */
    void newSock();

    /**
     * @brief 初始化sock
     */
    virtual bool init(int sock);
protected:
    /// socket句柄
    int m_sock;//无需存放远端sockfd，通过getpeername可以直接获取远端地址
    /// 协议簇
    int m_family;
    /// 类型
    int m_type;
    /// 协议
    int m_protocol;
    /// 是否连接
    bool m_isConnected;
    /// 本地地址
    Address::ptr m_localAddress;
    /// 远端地址
    Address::ptr m_remoteAddress;
};


/**
 * @brief 流式输出socket*/
std::ostream& operator<<(std::ostream& os, const Socket& sock);

}

#endif
