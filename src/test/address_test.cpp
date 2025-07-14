#include"address.h"
#include "log.h"
#include <iostream>
#define dbg ZHAO_LOG_DEBUG(GET_ROOT_LOGGER())
static void test03(void)
{
    std::vector<zhao::Address::ptr> addrs;

    dbg << "begin";
    //bool v = zhao::Address::Lookup(addrs, "localhost:3080");
    bool v = zhao::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    //bool v = zhao::Address::Lookup(addrs, "www.sylar.top", AF_INET);
    dbg << "end";
    if(!v) {
       dbg << "lookup fail";
        return;
    }

    for(size_t i = 0; i < addrs.size(); ++i) {
        dbg << i << " - " << addrs[i]->toString();
    }

    auto addr = zhao::Address::LookupAny("localhost:4080");
    if(addr) {
        dbg << *addr;
    } else {
        dbg << "error";
    } 
}
static void test02(void)
{
    
    std::multimap<std::string, std::pair<zhao::Address::ptr, uint32_t> > results;
    bool v = zhao::Address::GetInterfaceAddresses(results);
    if(!v) {
        dbg << "GetInterfaceAddresses fail";
        return;
    }

    for(auto& i: results) {
        dbg << i.first << " - " << i.second.first->toString() << " - "
            << i.second.second;
    }
}
static void test01()
{
    auto addr = zhao::IPAddress::Create("127.0.0.8");
    if(addr) {
        dbg << addr->toString();
    }
}
void address_test(void)
{
    test03();
}