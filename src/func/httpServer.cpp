#include "server/httpServer.hpp"
#include "http/httpsession.hpp"
#include "log/logger.hpp"
#define MODULE_NAME "http"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))
namespace zhao
{
    HttpServer::HttpServer(bool keepalive)
        : TCPServer(), m_isKeepalive(keepalive)
    {
        m_type = "http";
        m_name = "httpserver";
    }

    HttpServer::~HttpServer()
    {
    }

    

    void HttpServer::handleClient(Socket::ptr client)
    {
        HttpSession::ptr session(new HttpSession(client));
        do
        {
            HttpRequest::ptr req = session->recvRequest();
            if (!req)
            {
                 dbg << "recv http request fail, errno=" << errno
                    << " errstr=" << strerror(errno)
                    << " cliet:" << *client;
                break;
            }
             dbg << "request: " << req->toString();

            HttpResponse::ptr rsp(new HttpResponse(req->getVersion(), req->isClose() || !m_isKeepalive));
            rsp->setStatus(HttpStatus::OK);
            rsp->setBody("hello zhao http server");
            rsp->setHeader("Server", "zhao http server");
            session->sendResponse(rsp);
            if (req->isClose() || !m_isKeepalive)
            {
                break;
            }
        } while (true);
        session->close();
         dbg << "client:" << *client << " disconnect";
    }
} // namespace zhao