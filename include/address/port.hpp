#ifndef __ZHAO_PORT_HPP_
#define __ZHAO_PORT_HPP_
#include <string>
namespace zhao{
    typedef uint16_t port_t;
    class Port{
    public:
        Port(port_t port = 0);
        ~Port();
        /**
         * @brief 获取网络序端口
         * @return 网络序端口
         */
        port_t getNetSeqPort() const;
        /**
         * @brief 获取主机序端口
         * @return 主机序端口
         */
        port_t getHostSeqPort() const;
        /**
         * @brief 设置端口
         * @param port 主机序端口
         */
        void setPort(port_t port = 0);
        std::string toString() const;
    private:
        port_t m_port;
    };
}

#endif