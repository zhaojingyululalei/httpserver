#include "fiber.h"
#include "log.h"
#define dbg ZHAO_LOG_DEBUG(GET_ROOT_LOGGER())
static void fiber_test_func(){
    dbg<<"fiber_one";
    zhao::Fiber::yieldToHold();//切到主协程
    dbg<<"fiber_two";
    zhao::Fiber::yieldToHold();//切到主协程

}
static void test01()
{
    zhao::Fiber::getThis();//创建主协程
    dbg<<"main begin";
    zhao::Fiber::Ptr fiber(new zhao::Fiber(fiber_test_func));//创建子协程
    fiber->swapIn();//切换到子协程
    dbg << "main after  swapIn__1";
    fiber->swapIn();
    dbg << "main after  swapIn__2";
    fiber->swapIn();
    dbg  << "main end";
}
void fiber_test()
{
    test01();
}