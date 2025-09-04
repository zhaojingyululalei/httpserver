#include "server/echoServer.hpp"
#include "log/logger.hpp"
#include "address/address.hpp"
#define MODULE_NAME "tcpserver"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))

static void run() {
    
    zhao::EchoServer::ptr es(new zhao::EchoServer);
    auto addr = zhao::Address::LookupAny("0.0.0.0:8020");
    while(!es->bind(addr)) {
        sleep(2);
    }
    //开始工作，不断接收新的client_sockfd  然后处理
    //start中调用了schedule，并且在bind中传入了this，EchoServer的share_ptr会+1
    //因此run执行结束后，EchoServer对象不会被释放
    es->start();
}//

void echoserver_test() {
    

    zhao::Scheduler iom(8);
    iom.schedule(run);
    //iom调用析构  析构中发现有m_rootFiber,会跳转到m_rootFiber去执行，m_rootFiber中没有其他任务又会进入idle协程
    //也就是说，析构函数执行了一半，没有执行完就卡在了idle协程中
    //因此，即便执行完return 0,iom对象并不会被释放
    //直到任务队列中没有cb也没有fiber，此时idle中的while循环结束，退出idle，然后回到析构函数接着回收其他线程
    //最后释放iom对象
    return ;
}

static void server_test(void)
{
    echoserver_test();
}