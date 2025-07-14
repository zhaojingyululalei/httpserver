
#include "socket.h"
#include "ioscheduler.h"
#include "log.h"
#include "utils.h"
#define dbg ZHAO_LOG_DEBUG(GET_ROOT_LOGGER())

static void test02() {
    
    zhao::IPAddress::ptr addr = zhao::Address::LookupAnyIPAddress("www.baidu.com");
    if(addr) {
        dbg << "get address: " << addr->toString();
    } else {
        dbg << "get address fail";
        return;
    }

    zhao::Socket::ptr sock = zhao::Socket::createTCP(addr);
    addr->setPort(80);
    dbg << "addr=" << addr->toString();
    if(!sock->connect(addr)) {
        dbg << "connect " << addr->toString() << " fail";
        return;
    } else {
        dbg << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        dbg << "send fail rt=" << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if(rt <= 0) {
        dbg << "recv fail rt=" << rt;
        return;
    }

    buffs.resize(rt);
    dbg << buffs;
}

static void test01() {
    zhao::IPAddress::ptr addr = zhao::Address::LookupAnyIPAddress("www.baidu.com:80");
    if(addr) {
        dbg << "get address: " << addr->toString();
    } else {
        dbg << "get address fail";
        return;
    }

    zhao::Socket::ptr sock = zhao::Socket::createTCP(addr);
    if(!sock->connect(addr)) {
        dbg << "connect " << addr->toString() << " fail";
        return;
    } else {
        dbg << "connect " << addr->toString() << " connected";
    }

    uint64_t ts = zhao::getCurrentUS();
    for(size_t i = 0; i < 10000000000ul; ++i) {
        if(int err = sock->getError()) {
            dbg << "err=" << err << " errstr=" << strerror(err);
            break;
        }

        //struct tcp_info tcp_info;
        //if(!sock->getOption(IPPROTO_TCP, TCP_INFO, tcp_info)) {
        //    dbg << "err";
        //    break;
        //}
        //if(tcp_info.tcpi_state != TCP_ESTABLISHED) {
        //    dbg
        //            << " state=" << (int)tcp_info.tcpi_state;
        //    break;
        //}
        static int batch = 10000000;
        if(i && (i % batch) == 0) {
            uint64_t ts2 = zhao::getCurrentUS();
            dbg << "i=" << i << " used: " << ((ts2 - ts) * 1.0 / batch) << " us";
            ts = ts2;
        }
    }
}

void socket_test(void) {
    zhao::IOScheduler iom;
    //iom.join(&test01);
    iom.join(&test02);
    return ;
}
