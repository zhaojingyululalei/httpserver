#include "hook.h"

#include "log.h"
#include "ioscheduler.h"


#define dbg ZHAO_LOG_DEBUG(GET_ROOT_LOGGER())
static void test01()
{
    zhao::IOScheduler iosche;
    iosche.join([](){
        sleep(2);
        dbg << "sleep 2";
    });
    iosche.join([](){
        sleep(3);
        dbg << "sleep 3";
    });
    dbg<<"hook test end";
}

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
static void test_sock() { 
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);

    dbg << "begin connect";
    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    dbg << "connect rt=" << rt << " errno=" << errno;

    if(rt) {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    dbg << "send rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(sock, &buff[0], buff.size(), 0);
    dbg << "recv rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }

    buff.resize(rt);
    dbg << buff;
}
static void test02() {
    zhao::IOScheduler iom;
    iom.join(test_sock);
    
}
void hook_test()
{
    test02();
}