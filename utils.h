#include <stdint.h>
#include <stdbool.h>
#include <driverlib/sysctl.h>

void PLL_Init(void);
void SysTick_Init(void);

extern void SysTick_Wait1us(int time_in_us);
extern void SysTick_Wait1ms(int time_in_ms);

