#include "mem/common.hpp"
using namespace zhao;
static void test02() {
    SpanList spanList;
    
    // 获取迭代器
    auto it = spanList.begin();
    
    if (it != spanList.end()) {
        Span &span1 = *it;  // 调用 operator*()
        std::cout << "通过*it访问: " << span1.page_id << std::endl;
        
        
        std::cout << "通过it->访问: " << it->page_id << std::endl;  // 调用 operator->()
        std::cout << "页数: " << it->n_pages << std::endl;
        
        // 修改成员
        it->out_count++;
        
        // 遍历时使用
        for (auto iter = spanList.begin(); iter != spanList.end(); ++iter) {
            std::cout << "Page ID: " << iter->page_id << std::endl;  // 使用 -> 运算符
            Span& span = *iter;  // 使用 * 运算符
            std::cout << "Pages: " << span.n_pages << std::endl;
        }
    }
}
static void test01()
{
    // 创建一个SpanList实例
    SpanList spanList;
    
    
    // 方法1：使用范围for循环（推荐）
    std::cout << "使用范围for循环遍历:" << std::endl;
    for (Span& span : spanList) {
        std::cout << "Span - page_id: " << span.page_id 
                  << ", n_pages: " << span.n_pages 
                  << ", out_count: " << span.out_count << std::endl;
    }
    
    // 方法2：使用传统迭代器方式
    std::cout << "\n使用传统迭代器遍历:" << std::endl;
    for (auto it = spanList.begin(); it != spanList.end(); ++it) {
        Span& span = *it;  // 解引用获取Span指针
        std::cout << "Span - page_id: " << span.page_id 
                  << ", n_pages: " << span.n_pages << std::endl;
    }
    
    // 方法3：使用while循环
    std::cout << "\n使用while循环遍历:" << std::endl;
    auto it = spanList.begin();
    while (it != spanList.end()) {
        Span& span = *it;
        std::cout << "Span - page_id: " << span.page_id << std::endl;
        ++it;
    }
    
    // 查找特定条件的Span
    std::cout << "\n查找n_pages大于10的Span:" << std::endl;
    for (Span& span : spanList) {
        if (span.n_pages > 10) {
            std::cout << "找到Span: page_id=" << span.page_id 
                      << ", n_pages=" << span.n_pages << std::endl;
        }
    }
    
    // 统计链表中Span的数量
    size_t count = 0;
    for (Span& span : spanList) {
        count++;
    }
    std::cout << "\n链表中Span总数: " << count << std::endl;
}
void spanlist_test() {
    test01();
}