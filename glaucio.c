#include "utils.h"
void SysTick_Wait1us(int time_in_us)
{
  SysCtlDelay(time_in_us * 40);
}
void SysTick_Wait1ms(int time_in_ms)
{
    SysCtlDelay(time_in_ms * 40000);
}
