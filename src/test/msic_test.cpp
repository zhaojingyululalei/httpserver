

#include <iostream>
class Parent
{
public:
    Parent() = default;
    virtual ~Parent() = default;
    virtual void func() {}
    void func2()
    {
        func();
    }
};
class Son : public Parent
{
public:
    Son() = default;
    ~Son() = default;
    void func() override
    {
        std::cout << "Son func" << std::endl;
    }
};

void msic_test()
{
    Parent *p = new Son();
    p->func2(); // 调用 Son::func
    delete p;   // 释放内存
}