#include "ioscheduler.h"
#include "log.h"

#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#define dbg ZHAO_LOG_DEBUG(GET_ROOT_LOGGER())

static void fiber_test(void)
{
    dbg<<"fiber_test";
}
static void test01(void)
{
    //构造函数调用start，析构函数调用stop
    zhao::IOScheduler iosche;
    iosche.join(&fiber_test);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        dbg<<"socket error";
        return;
    }
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8080);
    inet_pton(AF_INET, "39.156.70.46", &servaddr.sin_addr.s_addr);

    iosche.addEvent(sockfd, zhao::IOScheduler::EventType::WRITE, []()
    { 
        dbg<<"connect success";
    });
    
    int ret = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (ret == -1 && errno != EINPROGRESS)
    {
        dbg<<"connect error";
        return;
    }
    //sleep(3);
}
void ioscheduler_test()
{
    test01();

}