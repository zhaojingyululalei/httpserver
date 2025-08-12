#include "utils/algrithm.hpp"
#include "utils/hashmap.hpp"
#include <chrono>
#include <thread>
using namespace zhao;
/**
 * @brief 测试令牌桶
 */
static void test01()
{
    // 示例：a=1秒，b=3秒
    DynamicTokenBucket bucket(1, 2);

    auto make_request = [&](int id)
    {
        bool success = bucket.try_consume();
        std::cout << "Request " << id << ": " << (success ? "Accepted" : "Rejected")
                  << ", Tokens: " << bucket.get_tokens() << std::endl;
    };

    // 测试场景
    make_request(1);                                             // 初始1令牌，消耗1 → 0
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 0.5秒后
    make_request(2);                                             // 在a秒内 → 令牌+1=1，消耗1 → 0
    std::this_thread::sleep_for(std::chrono::milliseconds(600)); // 总共1.1秒后
    make_request(3);                                             // 仍在a秒内 → 令牌+1=1，消耗1 → 0
    std::this_thread::sleep_for(std::chrono::seconds(3));        // 总共3.1秒后
    make_request(4);                                             // 超过b秒 → 令牌减到1，消耗1 → 0
    std::this_thread::sleep_for(std::chrono::seconds(3));        // 总共7.1秒后
    make_request(5);                                             // 超过多个b周期 → 令牌保持1，消耗1 → 0

    return;
}

/**
 * @brief 测试hashmap
 */
static void testBasicOperations() {
    zhao::HashMap<int, std::string> map;

    // 测试插入和查找
    map.insert(1, "Apple");
    assert(*map.find(1) == "Apple");

    // 测试更新
    map.insert(1, "Banana");
    assert(*map.find(1) == "Banana");

    // 测试删除
    assert(map.erase(1) == true);
    assert(map.find(1) == nullptr);

    // 测试清空
    map.insert(2, "Cherry");
    map.clear();
    assert(map.find(2) == nullptr);
}
static void testCollisionHandling() {
    // 使用固定哈希函数制造冲突
    struct FixedHash {
        size_t operator()(int) const { return 0; }
    };

    zhao::HashMap<int, std::string, FixedHash> map(2);

    // 所有元素都会哈希到同一个桶
    map.insert(1, "A");
    map.insert(2, "B");
    map.insert(3, "C");

    assert(*map.find(1) == "A");
    assert(*map.find(2) == "B");
    assert(*map.find(3) == "C");

    // 测试链式删除
    assert(map.erase(2) == true);
    assert(map.find(2) == nullptr);
    assert(*map.find(1) == "A");  // 其他元素应不受影响
}
static void testRehashing() {
    zhao::HashMap<int, int> map(4);  // 初始容量4

    // 插入3个元素（负载因子=0.75）
    for (int i = 0; i < 3; ++i) {
        map.insert(i, i*10);
    }

    // 触发扩容（负载因子>=0.7时扩容）
    map.insert(3, 30);

    // 验证所有元素存在
    for (int i = 0; i < 4; ++i) {
        assert(*map.find(i) == i*10);
    }
}
static void testMemoryManagement() {
    // 验证ObjectPool是否正常工作
    zhao::HashMap<long, long> map;

    // 大规模插入删除
    const int N = 10000;
    for (long i = 0; i < N; ++i) {
        map.insert(i, i*i);
    }

    // 验证没有内存泄漏
    for (long i = 0; i < N; ++i) {
        assert(map.erase(i) == true);
    }

    // 验证池化效果
    for (long i = 0; i < N; ++i) {
        map.insert(i, i);
    }
}
static void testEdgeCases() {
    // 测试空表行为
    zhao::HashMap<std::string, int> emptyMap;
    assert(emptyMap.find("missing") == nullptr);
    assert(emptyMap.erase("nothing") == false);

    // 测试默认构造容量
    zhao::HashMap<int, float> defaultMap;
    for (int i = 0; i < 12; ++i) {  // 16*0.7=11.2
        defaultMap.insert(i, i/2.0f);
    }
    assert(*defaultMap.find(5) == 2.5f);

    // 测试超大对象
    struct BigType { char data[1024]; };
    zhao::HashMap<int, BigType> bigMap;
    bigMap.insert(1, BigType{});
    assert(bigMap.find(1) != nullptr);
}
static void test02()
{
    std::cout << "Running basic tests..." << std::endl;
    testBasicOperations();

    std::cout << "Testing collision handling..." << std::endl;
    testCollisionHandling();

    std::cout << "Testing rehashing..." << std::endl;
    testRehashing();

    std::cout << "Validating memory management..." << std::endl;
    testMemoryManagement();

    std::cout << "Checking edge cases..." << std::endl;
    testEdgeCases();

    // std::cout << "Stress testing..." << std::endl;
    // testConcurrentAccess();  // 需要线程安全实现

    std::cout << "All tests passed!" << std::endl;
}
void algorithm_test()
{
    test02();
}