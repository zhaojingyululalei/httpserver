#ifndef __ZHAO_ECHOSERVER_HPP_
#define __ZHAO_ECHOSERVER_HPP_
#include "server/tcpServer.hpp"
#include <memory>
namespace zhao
{
    class EchoServer : public TCPServer
    {
    public:
        typedef std::shared_ptr<EchoServer> ptr;
        EchoServer(){}
        void handleClient(zhao::Socket::ptr client);

    
    };
}

#endif