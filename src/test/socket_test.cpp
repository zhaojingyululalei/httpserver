#include "socket.hpp"
#include "address/ipaddress.hpp"
#include "log/logger.hpp"
#include "scheduler/scheduler.hpp"
#include "hook.hpp"
#include <string>
#include "stream/socketstream.hpp"

using namespace zhao;
#define MODULE_NAME "socket"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))
// 测试TCP服务器基本功能
static void test_tcp_server() {
    dbg << "====== test_tcp_server =====";
    // 创建IPv4地址
    auto addr = IPv4Address::create("127.0.0.1", 5000);
    
    // 创建TCP Socket
    auto sock = Socket::CreateTCP(addr);
    
    // 测试bind + listen
    if(!sock->bind(addr)) {
        error << "bind error";
        return;
    }
    
    if(!sock->listen()) {
        error << "listen error";
        return;
    }
    
    dbg << "TCP server waiting for connection at: " << addr->toString();
    
    // 等待并接受客户端连接
    auto client = sock->accept();
    if(client) {
        dbg << "New client connected from: " << client->getRemoteAddress()->toString();
        // 这里可以添加数据收发测试
        const char* msg = "Hello from server!";
        client->send(msg, strlen(msg));
    }
}

// 测试TCP客户端基本功能 
static void test_tcp_client() {
    dbg << "====== test_tcp_client =====";
    sleep(1);
    auto addr = IPv4Address::create("127.0.0.1", 5000);
    auto sock = Socket::CreateTCP(addr);
    
    // 测试connect
    if(!sock->connect(addr,2)) {
        error << "connect error";
        return;
    }
    
    dbg << "connect success, remote address: " << sock->getRemoteAddress()->toString();
    dbg << "local address: " << sock->getLocalAddress()->toString();
}

// 测试UDP功能
static void test_udp() {
    dbg << "====== test_udp =====";
    auto addr = IPv4Address::create("127.0.0.1", 8889);
    auto sock = Socket::CreateUDP(addr);
    
    if(!sock->bind(addr)) {
        error << "bind error";
        return;
    }
    
    dbg << "UDP server start at: " << addr->toString();
}

static void test01()
{
    
    //test_tcp_server();
    zhao::Scheduler::getThis()->schedule([](){
        test_tcp_server();
    });

    
    //test_tcp_client();
    zhao::Scheduler::getThis()->schedule([](){
        test_tcp_client();
    });
    
    
    test_udp();
}
static void test02() {
    zhao::Scheduler::getThis()->schedule([](){
        // 服务端
        auto addr = IPv4Address::create("127.0.0.1", 5000);
        auto sock = Socket::CreateTCP(addr);
        
        if(!sock->bind(addr)) {
            error << "bind error";
            return;
        }
        
        if(!sock->listen()) {
            error << "listen error";
            return;
        }
        
        dbg << "TCP server waiting for connection at: " << addr->toString();
        
        auto client = sock->accept();
        if(client) {
            dbg << "New client connected from: " << client->getRemoteAddress()->toString();
            SocketStream ss(client);
            
            // 测试接收不同类型的数据
            int i;
            float f;
            std::string str;
            char buf[128];
            
            ss >> i >> f >> str >> buf;
            dbg << "Server received: " << i << " " << f << " " << str << " " << buf;
            
            // 发送响应
            ss << "Response: " << i * 2 << " " << f * 2.0f << " " << str + "_echo" << "\n";
        }
    });

    zhao::Scheduler::getThis()->schedule([](){
        sleep(1); // 等待服务器启动
        auto addr = IPv4Address::create("127.0.0.1", 5000);
        auto sock = Socket::CreateTCP(addr);
        
        if(!sock->connect(addr,2)) {
            error << "connect error";
            return;
        }
        
        dbg << "Client connected to: " << sock->getRemoteAddress()->toString();
        
        SocketStream ss(sock);
        
        // 测试发送不同类型的数据
        int i = 42;
        float f = 3.14f;
        std::string str = "hello";
        const char* cstr = "world";
        
        ss << i << f << str << cstr;
        
        // 接收服务器响应
        std::string response;
        ss >> response;
        dbg << "Client received: " << response;
    });
}

void socket_test() {
    test02();
}