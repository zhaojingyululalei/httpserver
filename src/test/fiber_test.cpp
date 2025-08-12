#include "fiber/fiber.hpp"
#include "log/logger.hpp"

#define MODULE_NAME "fiber_test"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("root"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("root"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("root"))
static void run_in_fiber()
{
    dbg<< "run in fiber start";
    zhao::Fiber::getThis()->yieldToHold();
    dbg<<"run in fiber continue.....";
}

static void test_fiber()
{
    dbg << "start test";
    {
        zhao::Fiber::ptr fiber(new zhao::Fiber(run_in_fiber));
        fiber->swapIn();
        dbg<<"test fiber continue...";
        fiber->swapIn();
        dbg <<"test end";
    }
    dbg << "test end 2";
}
static void test01()
{
    test_fiber();
}
void fiber_test()
{
    test01();
}