#include "address/address.hpp"
#include "address/ipaddress.hpp"
#include "address/unixaddress.hpp"
#include <cassert>
#include <vector>

using namespace zhao;

// 测试IPv4地址创建和基本功能
void testIPv4AddressBasic() {
    auto addr = IPv4Address::create("192.168.1.1", 8080);
    assert(addr != nullptr);  // 检查 addr 不为空
    assert(addr->toString() == "192.168.1.1:8080");  // 检查字符串表示是否正确
    assert(addr->getFamily() == AF_INET);  // 检查地址族是否正确
    assert(addr->getPort().getHostSeqPort() == 8080);  // 检查端口号
    std::cout << "testIPv4AddressBasic passed!" << std::endl;
}

// 测试IPv4子网掩码功能
void testIPv4SubnetMask() {
    auto addr = IPv4Address::create("192.168.1.1", 8080);
    auto mask = createSubnetMask(24);  // 255.255.255.0
    
    auto network = addr->networkAddress(mask);
    assert(network != nullptr);  // 检查网络地址不为空
    assert(network->toString() == "192.168.1.0:0");  // 检查网络地址字符串表示

    auto broadcast = addr->broadcastAddress(mask);
    assert(broadcast != nullptr);  // 检查广播地址不为空
    assert(broadcast->toString() == "192.168.1.255:0");  // 检查广播地址字符串表示
    std::cout << "testIPv4SubnetMask passed!" << std::endl;
}


// 测试地址查找功能
void testLookup() {
    std::vector<Address::ptr> addrs;
    bool result = Address::Lookup(addrs, "localhost");
    assert(result == true);  // 确保查找成功
    assert(!addrs.empty());  // 确保地址列表不为空
    
    auto any = Address::LookupAny("localhost");
    assert(any != nullptr);  // 确保查找返回一个有效的地址
    
    auto ip = Address::LookupAnyIPAddress("localhost");
    assert(ip != nullptr);  // 确保 IP 地址查找成功
    std::cout << "testLookup passed!" << std::endl;
}


// 测试网卡地址获取
void testInterfaceAddress() {
    std::vector<std::pair<Address::ptr, uint32_t>> results;
    bool result = Address::GetInterfaceAddresses(results, "lo");
    assert(result == true);  // 确保获取成功

    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> addrs;
    result = Address::GetInterfaceAddresses(addrs);
    assert(result == true);  // 确保获取成功
    assert(!addrs.empty());  // 确保获取到地址
    std::cout << "testInterfaceAddress passed!" << std::endl;
}


// 测试Unix域套接字地址
void testUnixAddress() {
    UnixAddress addr("/tmp/test.sock");
    assert(addr.getPath() == "/tmp/test.sock");  // 检查路径
    assert(addr.getFamily() == AF_UNIX);  // 检查地址族
    assert(addr.toString() == "unix:/tmp/test.sock");  // 检查字符串表示
    std::cout << "testUnixAddress passed!" << std::endl;
}




// 测试地址比较操作
void testAddressCompare() {
    auto addr1 = IPv4Address::create("192.168.1.1", 8080);
    auto addr2 = IPv4Address::create("192.168.1.1", 8080);
    auto addr3 = IPv4Address::create("192.168.1.2", 8080);

    assert(*addr1 == *addr2);  // 检查相等
    assert(*addr1 != *addr3);  // 检查不相等
    assert(*addr1 < *addr3);  // 检查地址比较
    std::cout << "testAddressCompare passed!" << std::endl;
}


static void test01() {
    testIPv4AddressBasic();
    testIPv4SubnetMask();
    testLookup();
    testInterfaceAddress();
    testUnixAddress();
    testAddressCompare();
    std::cout << "All tests passed!" << std::endl;
}
void address_test() {
    test01();
}
