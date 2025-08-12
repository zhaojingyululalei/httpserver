#include <iostream>
#include <memory>
static void test01(void)
{
    std::cout << "Testing malloc hook..." << std::endl;
    
    // 这应该会输出 "malloc"
    void* ptr = malloc(100);
    
    // 这应该会输出 "free"
    free(ptr);
    
    std::cout << "Test completed" << std::endl;
}

static void test02()
{
    std::cout << "Testing new and delete..." << std::endl;
    
    // 测试 new 和 delete
    int* ptr = new int(42);
    std::cout << "new int(42): " << *ptr << std::endl;
    delete ptr;
    
    // 测试 new[] 和 delete[]
    int* arr = new int[10];
    std::cout << "new int[10] allocated" << std::endl;
    delete[] arr;
    
    std::cout << "Testing shared_ptr..." << std::endl;
    
    // 测试 shared_ptr
    {
        std::shared_ptr<int> sp1 = std::make_shared<int>(100);
        std::cout << "shared_ptr created: " << *sp1 << std::endl;
        
        std::shared_ptr<int> sp2 = sp1;
        std::cout << "shared_ptr copy: " << *sp2 << std::endl;
        std::cout << "use_count: " << sp1.use_count() << std::endl;
    }
    // shared_ptr 离开作用域时会自动释放内存
    
    std::cout << "shared_ptr test completed" << std::endl;
}

// 设计一个简单的类
class TestClass {
public:
    TestClass(int value) : m_value(value) {
        std::cout << "TestClass constructed with value: " << m_value << std::endl;
    }
    
    ~TestClass() {
        std::cout << "TestClass destructed with value: " << m_value << std::endl;
    }
    
    void setValue(int value) {
        m_value = value;
    }
    
    int getValue() const {
        return m_value;
    }
    
    void printInfo() const {
        std::cout << "TestClass value: " << m_value << std::endl;
    }

private:
    int m_value;
};

static void test03(void)
{ 
    std::cout << "Testing class with new/delete and shared_ptr..." << std::endl;
    
    // 使用 new 和 delete
    TestClass* raw_ptr = new TestClass(42);
    raw_ptr->printInfo();
    raw_ptr->setValue(100);
    raw_ptr->printInfo();
    delete raw_ptr;
    
    // 使用 shared_ptr
    std::cout << "\nTesting shared_ptr:" << std::endl;
    std::shared_ptr<TestClass> shared_ptr1(new TestClass(200));
    shared_ptr1->printInfo();
    
    {
        // 创建 shared_ptr 的拷贝
        std::shared_ptr<TestClass> shared_ptr2 = shared_ptr1;
        std::cout << "use_count: " << shared_ptr1.use_count() << std::endl;
        shared_ptr2->setValue(300);
        shared_ptr2->printInfo();
    } // shared_ptr2 离开作用域，但对象不会被销毁，因为 shared_ptr1 仍然存在
    
    std::cout << "use_count: " << shared_ptr1.use_count() << std::endl;
    shared_ptr1->printInfo();
    
    // 使用 make_shared 创建 shared_ptr
    std::cout << "\nTesting make_shared:" << std::endl;
    auto shared_ptr3 = std::make_shared<TestClass>(400);
    shared_ptr3->printInfo();
    std::cout << "use_count: " << shared_ptr3.use_count() << std::endl;
    
    // 测试 shared_ptr 数组（C++11 不直接支持，需要自定义删除器）
    std::cout << "\nTesting shared_ptr with array:" << std::endl;
    std::shared_ptr<TestClass[]> shared_array(new TestClass[3]{TestClass(10), TestClass(20), TestClass(30)},
                                              [](TestClass* p) { delete[] p; });
    // 注意：对于数组，需要提供自定义删除器，否则会有未定义行为
} 
void hook_test(void)
{
    test03();
}