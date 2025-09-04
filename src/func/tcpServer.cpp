#include "server/tcpServer.hpp"
#include "config/configvar.hpp"
#include "log/logger.hpp"
#define MODULE_NAME "tcpserver"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))
namespace zhao {
static  ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
     ConfigMgr::getInstance()->add<uint64_t>("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2),
            "tcp server read timeout");
TCPServer::TCPServer() {
    m_worker = Scheduler::getThis();
    m_recvTimeout = g_tcp_server_read_timeout->getValue();
    m_name = "tcpserver";

}

TCPServer::~TCPServer() {
    for(auto& i :  m_listen_socks) {
        i->close();
    }
     m_listen_socks.clear();
}

bool TCPServer::bind(zhao::Address::ptr addr, bool ssl) {
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}

bool TCPServer::bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails) {
    for(auto& addr : addrs) {
        Socket::ptr sock = Socket::CreateTCP(addr);//创建socket
        if(!sock->bind(addr)) { //绑定地址
             dbg << "bind fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        if(!sock->listen()) {//监听地址
             dbg << "listen fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
         m_listen_socks.push_back(sock);//所有要监听的地址放在一起
    }

    if(!fails.empty()) {
         m_listen_socks.clear();
        return false;
    }

    for(auto& i :  m_listen_socks) {
         dbg << "type=" << m_type
            << " name=" << m_name
            << " server bind success: " << *i;
    }
    return true;
}

bool TCPServer::start() {
    if(!m_isStop) {
        return true;
    }
    m_isStop = false;
    for(auto& sock :  m_listen_socks) {
        m_worker-> schedule(std::bind(&TCPServer::startAccept,
                    shared_from_this(), sock));//分别去执行accept
    }
    return true;
}

void TCPServer::stop() {
    m_isStop = true;
    auto self = shared_from_this();
    m_worker-> schedule([this, self]() {
        for(auto& sock :  m_listen_socks) {
            sock->cancelAll();
            sock->close();
        }
         m_listen_socks.clear();
    });
}

void TCPServer::handleClient(Socket::ptr client) {
    dbg << "handleClient: " << *client;
}

void TCPServer::startAccept(Socket::ptr sock) {
    while(!m_isStop) {
        Socket::ptr client = sock->accept();
        if(client) {
            client->setRecvTimeout(m_recvTimeout);
            m_worker->schedule(std::bind(&TCPServer::handleClient,
                        shared_from_this(), client));//收到client，交给io调度
        } else {
             dbg << "accept errno=" << errno
                << " errstr=" << strerror(errno);
        }
    }
}

} // namespace zhao

