#ifndef __ZHAO_TCPSERVER_HPP_
#define __ZHAO_TCPSERVER_HPP_
#include <memory>
#include <vector>
#include <string>
#include "socket.hpp"
#include "address/ipaddress.hpp"
#include "scheduler/scheduler.hpp"
namespace zhao
{
    class TCPServer : public std::enable_shared_from_this<TCPServer>
    {
    public:
        typedef std::shared_ptr<TCPServer> ptr;
        TCPServer();
        virtual ~TCPServer();
        virtual bool bind(zhao::Address::ptr addr, bool ssl = false);
        virtual bool bind(const std::vector<Address::ptr> &addrs, std::vector<Address::ptr> &fails);
        virtual bool start();
        virtual void stop();
        virtual void setName(const std::string &v) { m_name = v; }
        std::string getName() const { return m_name; }

    protected:
        /**
         * @brief 处理新连接的Socket类
         */
        virtual void handleClient(Socket::ptr client);

        /**
         * @brief 开始接受连接
         */
        virtual void startAccept(Socket::ptr sock);
        /// 监听Socket数组
        std::vector<Socket::ptr> m_listen_socks;
        /// 新连接的Socket工作的调度器
        Scheduler *m_worker;
        /// 接收超时时间(毫秒)
        uint64_t m_recvTimeout;
        /// 服务器名称
        std::string m_name;
        /// 服务器类型
        std::string m_type = "tcp";
        /// 服务是否停止
        bool m_isStop = false;
    };

} // namespace zhao

#endif