#include "tcpServer.hpp"
#include <memory>
namespace zhao
{
    /**
     * @brief HTTP服务器类
     */
    class HttpServer : public TCPServer
    {
    public:
        /// 智能指针类型
        typedef std::shared_ptr<HttpServer> ptr;

        /**
         * @brief 构造函数
         * @param[in] keepalive 是否长连接
         * @param[in] worker 工作调度器
         * @param[in] accept_worker 接收连接调度器
         */
        HttpServer(bool keepalive = false);

        ~HttpServer();

    protected:
        virtual void handleClient(Socket::ptr client) override;

    private:
        /// 是否支持长连接
        bool m_isKeepalive;
    };
}