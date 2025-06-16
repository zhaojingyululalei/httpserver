
#include "ioscheduler.h"
#include "log.h"

#define dbg ZHAO_LOG_DEBUG(GET_ROOT_LOGGER())
static  zhao::Timer::ptr timer;
static void test01(void)
{
    zhao::IOScheduler sche(2);
    
    timer = sche.addTimer(1000, []() {
        static int i = 0;
        dbg << "timer callback i=" << i ;
        if(++i == 3)
        {
            timer->reset(2000);
            //timer->cancel();
        }
    },true);
}
void timerscheduler_test()
{
    test01();
}