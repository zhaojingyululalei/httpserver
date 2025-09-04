#ifndef __ZHAO_HTTPSESSION_HPP_
#define __ZHAO_HTTPSESSION_HPP_
#include "stream/socketstream.hpp"
#include "httpRequest.hpp"
#include "httpResponse.hpp"
#include <memory>
namespace zhao
{
    /**
     * @brief HTTPSession封装
     * @note clientfd是一个session
     */
    class HttpSession
    {
    public:
        /// 智能指针类型定义
        typedef std::shared_ptr<HttpSession> ptr;

        /**
         * @brief 构造函数
         * @param[in] sock Socket类型
         */
        HttpSession(Socket::ptr sock);

        /**
         * @brief 接收HTTP请求
         */
        HttpRequest::ptr recvRequest();
        /**
         * @brief 发送HTTP响应
         */
        int sendResponse(HttpResponse::ptr rsp);
        /**
         * @brief 关闭会话
         */
        void close() { m_socket->close(); }

    private:
        SocketStream m_stream;
        Socket::ptr m_socket;
    };
}

#endif