#include "server/echoServer.hpp"
#include "log/logger.hpp"
#define MODULE_NAME "tcpserver"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))
namespace zhao
{
    void EchoServer::handleClient(zhao::Socket::ptr client)
    {
        char buf[4096];
        while (true) {
            int n = client->recv(buf, sizeof(buf));
            if (n <= 0) {
                break;
            }
            int sent = 0;
            while (sent < n) {
                int ret = client->send(buf + sent, n - sent);
                if (ret <= 0) {
                    break;
                }
                sent += ret;
            }
            if (sent < n) {
                break;
            }
        }
        client->close();
    }
}
