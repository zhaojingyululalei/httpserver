#include "fiberscheduler.h"
#include "log.h"

#define dbg ZHAO_LOG_DEBUG(GET_ROOT_LOGGER())
static void a_fiber_func(void){
    static int i = 20;
    dbg<<"a_fiber_func"<<i<<"\r\n";
    sleep(1);
    if(--i>=0){
        zhao::FiberScheduler::getThis()->join(a_fiber_func);
    }
}
static void test02(void)
{
    zhao::FiberScheduler sched(10, true, "test02_sche");
    sched.start();
    sched.join(a_fiber_func);
    sleep(10);
    sched.stop();
}
static void test01(void)
{
    zhao::FiberScheduler sched(10,false,"test01_sche");
    sched.start();
    sched.join(a_fiber_func);
    sleep(15);
    sched.stop();
    dbg<<"test01_sche stop";
}
void fiberscheduler_test(void)
{
    test01();
}