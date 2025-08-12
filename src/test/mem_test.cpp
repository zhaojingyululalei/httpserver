#include "utils/utils.hpp"
#include <iostream>
#include <vector>
#include <string.h>
#include "mem/objectpool.hpp"
#include "mem/common.hpp"
using namespace zhao;

/**
 * @brief 测试getPageSize
 */
static void test01(void)
{
    
    size_t pageSize = Utils::Memory::getPageSize();
    std::cout << "page size: " << pageSize << std::endl;
}
static void test02(const int& a)
{
    std::cout << "a: " << a << std::endl;
}

// 测试用的简单类
class TestObject {
public:
    int id;
    char name[32];
    
    TestObject() : id(0) {
        strcpy(name, "default");
        std::cout << "TestObject default constructor called for obj at " << this << std::endl;
    }
    
    TestObject(int _id, const char* _name) : id(_id) {
        strncpy(name, _name, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        std::cout << "TestObject parameter constructor called for obj at " << this 
                  << " (id: " << id << ", name: " << name << ")" << std::endl;
    }
    
    ~TestObject() {
        std::cout << "TestObject destructor called for obj at " << this 
                  << " (id: " << id << ", name: " << name << ")" << std::endl;
    }
    
    void print() const {
        std::cout << "TestObject(id=" << id << ", name=" << name << ") at " << this << std::endl;
    }
};

static void test_phaddr_functionality() {
    std::cout << "=== Phaddr Functionality Test ===" << std::endl;
    
    // 分配一块内存用于模拟物理地址操作
    char buffer[1024];
    void* phaddr = buffer;
    
    // 测试 writeToPhysical 方法
    const char* test_data = "Hello Physical Address";
    size_t data_size = strlen(test_data) + 1;
    
    bool result1 = Phaddr<char>::writeToPhysical(phaddr, (void*)test_data, data_size);
    std::cout << "writeToPhysical result: " << (result1 ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "Written data: " << (char*)phaddr << std::endl;
    
    // 测试 wirteToPhysical 方法
    TestObject obj(123, "TestObject");
    void* obj_phaddr = buffer + 512; // 使用buffer的后半部分
    
    bool result2 = Phaddr<TestObject>::writeToPhysical(obj_phaddr, obj);
    std::cout << "wirteToPhysical result: " << (result2 ? "SUCCESS" : "FAILED") << std::endl;
    
    // 验证写入的数据
    TestObject* retrieved_obj = (TestObject*)obj_phaddr;
    std::cout << "Retrieved object: ";
    retrieved_obj->print();
}

static void test_object_pool_basic() {
    std::cout << "\n=== ObjectPool Basic Test ===" << std::endl;
    std::cout << "ObjectPool class is defined but not yet implemented." << std::endl;
    std::cout << "Page size: " << GET_PAGE_SIZE() << " bytes" << std::endl;
}
/**
 * @brief 测试Phaddr类
 */
static void test03(void)
{
    test_phaddr_functionality();
   // test_object_pool_basic();
}
/**
 * @brief 测试ObjectPool
 */
static void test04(void)
{
    std::cout << "=== ObjectPool Test ===" << std::endl;
    
    // 创建对象池
    ObjectPool<TestObject> pool;
    
    std::cout << "\n1. 分配新对象:" << std::endl;
    // 分配几个新对象
    TestObject* obj1 = pool._new();
    obj1->id = 1;
    strcpy(obj1->name, "First");
    
    TestObject* obj2 = pool._new();
    obj2->id = 2;
    strcpy(obj2->name, "Second");
    
    TestObject* obj3 = pool._new();
    obj3->id = 3;
    strcpy(obj3->name, "Third");
    
    obj1->print();
    obj2->print();
    obj3->print();
    
    std::cout << "\n2. 回收部分对象:" << std::endl;
    // 回收一些对象
    pool._delete(obj2);  // 回收obj2
    std::cout << "回收了 obj2" << std::endl;
    
    pool._delete(obj1);  // 回收obj1
    std::cout << "回收了 obj1" << std::endl;
    
    std::cout << "\n3. 重新分配对象（应复用回收的对象）:" << std::endl;
    // 重新分配，应该会复用之前回收的对象
    TestObject* obj4 = pool._new();
    obj4->id = 4;
    strcpy(obj4->name, "Fourth");
    std::cout << "新分配的 obj4 地址: " << obj4 << std::endl;
    obj4->print();
    
    TestObject* obj5 = pool._new();
    obj5->id = 5;
    strcpy(obj5->name, "Fifth");
    std::cout << "新分配的 obj5 地址: " << obj5 << std::endl;
    obj5->print();
    
    std::cout << "\n4. 再次回收所有对象:" << std::endl;
    pool._delete(obj3);
    pool._delete(obj4);
    pool._delete(obj5);
    
    std::cout << "\n5. 最后分配一批对象:" << std::endl;
    // 最后再分配一批对象
    std::vector<TestObject*> objects;
    for (int i = 10; i < 15; ++i) {
        TestObject* obj = pool._new();
        obj->id = i;
        sprintf(obj->name, "Object%d", i);
        objects.push_back(obj);
        obj->print();
    }
    
    // 清理
    for (auto obj : objects) {
        pool._delete(obj);
    }
    
    std::cout << "\n测试完成!" << std::endl;
}

#include "mem/tcmalloc.hpp"
#include "thread/thread.hpp"
static void alloc1(void)
{
    for (size_t i = 0; i < 512; i++)
    {
        void* ptr = tcmalloc(6);
        std::cout << "alloc1: " << ptr << std::endl;
    }
}
static void alloc2(void)
{
    for (size_t i = 0; i < 1024; i++)
    {
        void* ptr = tcmalloc(156);
        std::cout << "alloc2: " << ptr << std::endl;
    }
}
/**
 * @brief 测试tcmalloc
 */
static void test05()
{ 
    zhao::Thread t1(&alloc1,"t1");
    zhao::Thread t2(&alloc2,"t2");
    t1.join();
    t2.join();
    
}

void mem_test()
{
    //test01();
    //test02(10);
    //test03();

    //test04();
    test05();
}