#include "mem/tcmalloc.hpp"
#include "thread/thread.hpp"
#include <iostream>
#include <vector>
#include <chrono>
using namespace zhao;
// 测试小对象 (8B)
static void test_small_objects_8B() {
    const size_t ALLOC_COUNT = 10000;
    std::vector<void*> pointers;
    pointers.reserve(ALLOC_COUNT);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 分配小对象
    for (size_t i = 0; i < ALLOC_COUNT; i++) {
        void* ptr = tcmalloc(8);
        pointers.push_back(ptr);
    }
    
    // 释放小对象
    for (size_t i = 0; i < ALLOC_COUNT; i++) {
        tcfree(pointers[i]);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Small objects (8B) - Alloc/Free " << ALLOC_COUNT 
              << " objects: " << duration.count() << " microseconds" << std::endl;
}

// 测试小对象 (256KB)
static void test_small_objects_256KB() {
    const size_t ALLOC_COUNT = 1000;
    const size_t SIZE = 256 * 1024; // 256KB
    std::vector<void*> pointers;
    pointers.reserve(ALLOC_COUNT);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 分配中等对象
    for (size_t i = 0; i < ALLOC_COUNT; i++) {
        void* ptr = tcmalloc(SIZE);
        pointers.push_back(ptr);
    }
    
    // 释放中等对象
    for (size_t i = 0; i < ALLOC_COUNT; i++) {
        tcfree(pointers[i]);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Small objects (256KB) - Alloc/Free " << ALLOC_COUNT 
              << " objects: " << duration.count() << " microseconds" << std::endl;
}

// 测试中对象 (257KB)
static void test_medium_objects_257KB() {
    const size_t ALLOC_COUNT = 500;
    const size_t SIZE = 257 * 1024; // 257KB
    std::vector<void*> pointers;
    pointers.reserve(ALLOC_COUNT);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 分配中等对象
    for (size_t i = 0; i < ALLOC_COUNT; i++) {
        void* ptr = tcmalloc(SIZE);
        pointers.push_back(ptr);
    }
    
    // 释放中等对象
    for (size_t i = 0; i < ALLOC_COUNT; i++) {
        tcfree(pointers[i]);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Medium objects (257KB) - Alloc/Free " << ALLOC_COUNT 
              << " objects: " << duration.count() << " microseconds" << std::endl;
}

// 测试大对象 (513KB)
static void test_large_objects_513KB() {
    const size_t ALLOC_COUNT = 100;
    const size_t SIZE = 513 * 1024; // 513KB
    std::vector<void*> pointers;
    pointers.reserve(ALLOC_COUNT);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 分配大对象
    for (size_t i = 0; i < ALLOC_COUNT; i++) {
        void* ptr = tcmalloc(SIZE);
        pointers.push_back(ptr);
    }
    
    // 释放大对象
    for (size_t i = 0; i < ALLOC_COUNT; i++) {
        tcfree(pointers[i]);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Large objects (513KB) - Alloc/Free " << ALLOC_COUNT 
              << " objects: " << duration.count() << " microseconds" << std::endl;
}

// 线程函数：测试混合大小对象
static void mixed_alloc_thread(const std::string& thread_name) {
    const size_t ITERATIONS = 100;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < ITERATIONS; i++) {
        // 分配不同大小的对象
        void* ptr1 = tcmalloc(8);           // 小对象
        void* ptr2 = tcmalloc(256 * 1024);  // 256KB
        void* ptr3 = tcmalloc(257 * 1024);  // 257KB
        void* ptr4 = tcmalloc(513 * 1024);  // 513KB
        
        // 释放对象
        tcfree(ptr1);
        tcfree(ptr2);
        tcfree(ptr3);
        tcfree(ptr4);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Thread " << thread_name << " - Mixed allocation: " 
              << duration.count() << " microseconds" << std::endl;
}

// 多线程测试函数
static void test_multithreaded_performance() {
    std::cout << "\n=== Multi-threaded Performance Test ===" << std::endl;
    
    // 创建多个线程进行混合分配测试
    zhao::Thread t1([&]() { mixed_alloc_thread("T1"); }, "Thread-1");
    zhao::Thread t2([&]() { mixed_alloc_thread("T2"); }, "Thread-2");
    zhao::Thread t3([&]() { mixed_alloc_thread("T3"); }, "Thread-3");
    zhao::Thread t4([&]() { mixed_alloc_thread("T4"); }, "Thread-4");
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
}

// 主测试函数
static void test01() {
    std::cout << "=== TCMalloc Comprehensive Test ===" << std::endl;
    
    // 测试不同大小的对象性能
    test_small_objects_8B();
    test_small_objects_256KB();
    test_medium_objects_257KB();
    test_large_objects_513KB();
    
    // 多线程测试
    test_multithreaded_performance();
    std::cout << "=== TCMalloc Comprehensive Test Complete ===" << std::endl;
}



// 新增的测试入口
void tcmalloc_test() {
    test01();
   
}