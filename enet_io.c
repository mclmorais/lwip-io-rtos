#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/pin_map.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"

#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/adc.h"
#include "driverlib/pwm.h"
#include "driverlib/gpio.h"
#include "driverlib/rom_map.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "httpserver_raw/httpd.h"
#include "drivers/pinout.h"
#include "io.h"
#include "./i2c.h"
#include "utils.h"

// FreeRTOS includes
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

void ethernetTask(void *pvParameters);
void testTask(void *pvParameters);
void demoSerialTask(void *pvParameters);
void adcTask(void *pvParameters);
void oledTask(void *pvParameters);
void pwmTask(void *pvParameters);
uint32_t getSpeed();

bool systemOnline = false;
bool automaticMode = false;
uint32_t automaticModeSpeed = 0;
uint32_t manualModeSpeed = 0;
uint32_t timerValue = 0;
uint32_t interruptValue = 0;
uint32_t timerEntries = 0;
bool firstPulse = true;
uint32_t firstTime = 0;
uint32_t secondTime = 0;
int32_t measuredFrequency = 0;

#define PERIOD_SAMPLES 1
uint32_t periodAverage[PERIOD_SAMPLES] = {0};
uint32_t periodIndex = 0;

int32_t a = 10;

#define PWM_CLOCKS 32000

//! tools/bin/makefsfile -i fs -o io_fsdata.h -r -h -q

#define MAX_FLOW 40
// Defines for setting up the system clock.
#define SYSTICKHZ 100
#define SYSTICKMS (1000 / SYSTICKHZ)

// Interrupt priority definitions.  The top 3 bits of these values are
// significant with lower values indicating higher priority interrupts.
#define SYSTICK_INT_PRIORITY 0x80
#define ETHERNET_INT_PRIORITY 0xC0

#define FLAG_TICK 0
static volatile unsigned long g_ulFlags;

extern void httpd_init(void);

#define SSI_INDEX_LEDSTATE 0
#define SSI_INDEX_FORMVARS 1
#define SSI_INDEX_SPEED 2

//*****************************************************************************
//
// The file sent back to the browser in cases where a parameter error is
// detected by one of the CGI handlers.  This should only happen if someone
// tries to access the CGI directly via the broswer command line and doesn't
// enter all the required parameters alongside the URI.
//
//*****************************************************************************
#define PARAM_ERROR_RESPONSE "/perror.htm"

#define JAVASCRIPT_HEADER \
  "<script type='text/javascript' language='JavaScript'><!--\n"
#define JAVASCRIPT_FOOTER \
  "//--></script>\n"

// Timeout for DHCP address request (in seconds).
#ifndef DHCP_EXPIRE_TIMER_SECS
#define DHCP_EXPIRE_TIMER_SECS 45
#endif

// The current IP address.
uint32_t g_ui32IPAddress;

// The system clock frequency.  Used by the SD card driver.**************************
uint32_t g_ui32SysClock;

// The error routine that is called if the driver library encounters an error.
#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// Display an lwIP type IP Address.
//
//*****************************************************************************
void DisplayIPAddress(uint32_t ui32Addr)
{
  char pcBuf[16];

  //
  // Convert the IP Address into a string.
  //
  usprintf(pcBuf, "%d.%d.%d.%d", ui32Addr & 0xff, (ui32Addr >> 8) & 0xff,
           (ui32Addr >> 16) & 0xff, (ui32Addr >> 24) & 0xff);

  //
  // Display the string.
  //
  UARTprintf(pcBuf);
}

//*****************************************************************************
//
// Required by lwIP library to support any host-related timer functions.
//
//*****************************************************************************
void lwIPHostTimerHandler(void)
{
  uint32_t ui32NewIPAddress;

  //
  // Get the current IP address.
  //
  ui32NewIPAddress = lwIPLocalIPAddrGet();

  //
  // See if the IP address has changed.
  //
  if (ui32NewIPAddress != g_ui32IPAddress)
  {
    //
    // See if there is an IP address assigned.
    //
    if (ui32NewIPAddress == 0xffffffff)
    {
      //
      // Indicate that there is no link.
      //
      UARTprintf("Aguardando por conex�o.\n");
    }
    else if (ui32NewIPAddress == 0)
    {
      //
      // There is no IP address, so indicate that the DHCP process is
      // running.
      //
      UARTprintf("Aguardando por endere�o IP.\n");
    }
    else
    {
      //
      // Display the new IP address.
      //
      UARTprintf("Endere�o IP: ");
      DisplayIPAddress(ui32NewIPAddress);
      UARTprintf("\n");
      UARTprintf("Abra um navegador e acesse o IP acima.\n");
    }

    //
    // Save the new IP address.
    //
    g_ui32IPAddress = ui32NewIPAddress;
  }

  //
  // If there is not an IP address.
  //
  if ((ui32NewIPAddress == 0) || (ui32NewIPAddress == 0xffffffff))
  {
    //
    // Do nothing and keep waiting.
    //
  }
}

void configureEthernet()
{
  uint32_t ui32User0, ui32User1;
  uint8_t pui8MACArray[8];
  // Configure debug port for internal use.
  UARTStdioConfig(0, 115200, g_ui32SysClock);

  // Clear the terminal and print a banner.
  UARTprintf("Initializando Ethernet");

  // Configure SysTick for a periodic interrupt.
  MAP_SysTickPeriodSet(g_ui32SysClock / SYSTICKHZ);
  MAP_SysTickEnable();
  MAP_SysTickIntEnable();

  // Configure the hardware MAC address for Ethernet Controller filtering of
  // incoming packets.  The MAC address will be stored in the non-volatile
  // USER0 and USER1 registers.
  MAP_FlashUserGet(&ui32User0, &ui32User1);
  if ((ui32User0 == 0xffffffff) || (ui32User1 == 0xffffffff))
  {
    // Let the user know there is no MAC address
    UARTprintf("No MAC programmed!\n");

    while (1)
    {
    }
  }
  // Tell the user what we are doing just now.
  UARTprintf("Esperando por IP.\n");

  // Convert the 24/24 split MAC address from NV ram into a 32/16 split
  // MAC address needed to program the hardware registers, then program
  // the MAC address into the Ethernet Controller registers.
  pui8MACArray[0] = ((ui32User0 >> 0) & 0xff);
  pui8MACArray[1] = ((ui32User0 >> 8) & 0xff);
  pui8MACArray[2] = ((ui32User0 >> 16) & 0xff);
  pui8MACArray[3] = ((ui32User1 >> 0) & 0xff);
  pui8MACArray[4] = ((ui32User1 >> 8) & 0xff);
  pui8MACArray[5] = ((ui32User1 >> 16) & 0xff);

  // Initialze the lwIP library, using DHCP.
  lwIPInit(g_ui32SysClock, pui8MACArray, 0, 0, 0, IPADDR_USE_DHCP);

  // Setup the device locator service.
  LocatorInit();
  LocatorMACAddrSet(pui8MACArray);
  LocatorAppTitleSet("EK-TM4C1294XL Projeto SO-C2");

  //
  // Initialize a sample httpd server.
  //
  httpd_init();

  // Set the interrupt priorities.  We set the SysTick interrupt to a higher
  // priority than the Ethernet interrupt to ensure that the file system
  // tick is processed if SysTick occurs while the Ethernet handler is being
  // processed.  This is very likely since all the TCP/IP and HTTP work is
  // done in the context of the Ethernet interrupt.
  MAP_IntPrioritySet(INT_EMAC0, ETHERNET_INT_PRIORITY);
  MAP_IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);
}

void configureGPIOInterrupt(void)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

  while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA))
  {
  }

  IntEnable(INT_GPIOA);
  GPIOPinTypeGPIOInput(GPIO_PORTA_AHB_BASE, GPIO_PIN_2);
  GPIOIntTypeSet(GPIO_PORTA_AHB_BASE, GPIO_PIN_2, GPIO_FALLING_EDGE);
  GPIOIntEnable(GPIO_PORTA_AHB_BASE, GPIO_PIN_2);
}

void PortAIntHandler(void)
{
  GPIOIntClear(GPIO_PORTA_AHB_BASE, GPIO_PIN_2);

  if (firstPulse)
  {
    firstTime = TimerValueGet64(TIMER0_BASE);
    firstPulse = false;
  }
  else
  {
    secondTime = firstTime - TimerValueGet64(TIMER0_BASE);

    //UARTprintf("%i | ", secondTime);
    if (secondTime >= 2000000)
    {
      periodAverage[periodIndex] = firstTime - TimerValueGet64(TIMER0_BASE);
      if (++periodIndex >= PERIOD_SAMPLES)
        periodIndex = 0;
      TimerLoadSet64(TIMER0_BASE, g_ui32SysClock);
      firstPulse = true;
    }
  }
}

void configureTimer(void)
{
  // Enable the Timer0 peripheral
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

  // Wait for the Timer0 module to beg ready.
  while (!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0))
  {
  }

  IntMasterEnable();

  ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
  ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32SysClock);

  IntEnable(INT_TIMER0A);
  TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

  TimerEnable(TIMER0_BASE, TIMER_A);
}

Timer0BIntHandler(void)
{
  TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

  int i = 0;
  for (i = 0; i < PERIOD_SAMPLES; i++)
  {
    periodAverage[i] = 0;
  }

  TimerLoadSet64(TIMER0_BASE, g_ui32SysClock);

  timerValue = interruptValue;

  interruptValue = 0;
}

void configureController(void)
{
  // Make sure the main oscillator is enabled because this is required by
  // the PHY.  The system must have a 25MHz crystal attached to the OSC
  // pins.  The SYSCTL_MOSC_HIGHFREQ parameter is used when the crystal
  // frequency is 10MHz or higher.
  //
  SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);

  //
  // Run from the PLL at 120 MHz.
  //
  g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN |
                                           SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480),
                                          120000000);

  //
  // Configure the device pins.
  //
  PinoutSet(true, false);
}

//*****************************************************************************
//
// This example demonstrates the use of the Ethernet Controller and lwIP
// TCP/IP stack to control various peripherals on the board via a web
// browser.
//
//*****************************************************************************
int main(void)
{

  configureController();

  configureEthernet();

  xTaskCreate(ethernetTask, (const portCHAR *)"Ethernet Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

  xTaskCreate(demoSerialTask, (const portCHAR *)"Serial Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

  //  xTaskCreate(adcTask, (const portCHAR *)"ADC Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

  xTaskCreate(pwmTask, (const portCHAR *)"PWM Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

  xTaskCreate(oledTask, (const portCHAR *)"OLED Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

  configureTimer();

  configureGPIOInterrupt();

  vTaskStartScheduler();
  while (1)
  {
    //
    // Wait for a new tick to occur.
    //
    while (!g_ulFlags)
    {
      //
      // Do nothing.
      //
    }

    //
    // Clear the flag now that we have seen it.
    //
    HWREGBITW(&g_ulFlags, FLAG_TICK) = 0;

    //
    // Toggle the GPIO
    //
    MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,
                     (MAP_GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_1) ^
                      GPIO_PIN_1));
  }
}

void ethernetTask(void *pvParameters)
{
  UARTprintf("First time ethernet task!!\n");

  while (1)
  {
    // Call the lwIP timer handler.
    //
    lwIPTimer(SYSTICKMS);
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// Write text over the Stellaris debug interface UART port
void demoSerialTask(void *pvParameters)
{
  // Set up the UART which is connected to the virtual COM port
  UARTprintf("\r\nTask Serial Inicializada!");

  for (;;)
  {
   // UARTprintf("%i | ", measuredFrequency );//(getSpeed() * PWM_CLOCKS/100));
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    //I2C_OLED_Move_Cursor(0, 0);
    //I2C_OLED_Print(itoa(getSpeed()*2));
  }
}

void configurePwm(void)
{

  ROM_GPIOPinConfigure(GPIO_PG0_M0PWM4);

  ROM_GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0);

  //
  // Enable the PWM0 peripheral
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

  //
  // Wait for the PWM0 module to be ready.
  //
  while (!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0))
  {
  }

  //
  // Configure the PWM generator for count down mode with immediate updates
  // to the parameters.
  //
  PWMGenConfigure(PWM0_BASE, PWM_GEN_2,
                  PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

  //
  // Set the period. For a 50 KHz frequency, the period = 1/50,000, or 20
  // microseconds. For a 20 MHz clock, this translates to 400 clock ticks.
  // Use this value to set the period.
  //
  PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, 1600);
  //
  // Set the pulse width of PWM0 for a 25% duty cycle.
  //
  PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 1600);
  //
  // Start the timers in generator 0.
  //
  PWMGenEnable(PWM0_BASE, PWM_GEN_2);
  //
  // Enable the outputs.
  //
  PWMOutputState(PWM0_BASE, (PWM_OUT_4_BIT), true);
}

void configureOLED(void)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

  while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB))
  {
  }

  GPIOPinTypeGPIOOutput(GPIO_PORTA_AHB_BASE, GPIO_PIN_2);
  GPIOPinTypeGPIOOutputOD(GPIO_PORTA_AHB_BASE, GPIO_PIN_3);
  GPIOPinTypeI2CSCL(GPIO_PORTA_AHB_BASE, GPIO_PIN_2);
  GPIOPinTypeI2C(GPIO_PORTA_AHB_BASE, GPIO_PIN_3);

  // 3. Limpar o AMSEL para desabilitar a analógica
  GPIO_PORTB_AHB_AMSEL_R = 0x00;
  // 4. Limpar PCTL para selecionar o GPIO
  GPIO_PORTB_AHB_PCTL_R = 0x2200;
  // 6. Limpar os bits AFSEL para 0 para selecionar GPIO sem fun��o alternativa
  GPIO_PORTB_AHB_AFSEL_R = 0x0C;
  // 7. Setar os bits de DEN para habilitar I/O digital
  GPIO_PORTB_AHB_DEN_R = 0x0C;
  // 8. Habilitar resistor de pull-up interno, setar PUR para 1
  GPIO_PORTD_AHB_PUR_R = 0x0F;
  // 9. Registrador de Dreno Aberto
  GPIO_PORTB_AHB_ODR_R = 0x08;
  // 4. Limpar PCTL para selecionar o GPIO
  GPIO_PORTB_AHB_PCTL_R = 0x2200;
  I2C_Init();
  I2C_OLED_Init();
  I2C_OLED_Sequence_Init();
}

int32_t erroAnterior = 0;
int32_t rawPwmAnterior = 0;

void pwmTask(void *pvParameters)
{
  ROM_GPIOPinConfigure(GPIO_PG0_M0PWM4);

  ROM_GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0);

  // Enable the PWM0 peripheral
  SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

  // Wait for the PWM0 module to be ready.
  while (!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0))
  {
  }

  // Configure the PWM generator for count down mode with immediate updates
  // to the parameters.
  PWMGenConfigure(PWM0_BASE, PWM_GEN_2,
                  PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

  // Set the period. For a 50 KHz frequency, the period = 1/50,000, or 20
  // microseconds. For a 20 MHz clock, this translates to 400 clock ticks.
  // Use this value to set the period.
  PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PWM_CLOCKS);

  // Start the timers in generator 0.
  PWMGenEnable(PWM0_BASE, PWM_GEN_2);

  // Enable the outputs.
  PWMOutputState(PWM0_BASE, (PWM_OUT_4_BIT), true);

  while (1)
  {
    int i = 0;
    uint64_t sum = 0;
    for (i = 0; i < PERIOD_SAMPLES; i++)
    {
      sum += periodAverage[i];
    }
    sum = sum / PERIOD_SAMPLES;

    measuredFrequency = ((int)(g_ui32SysClock / sum));

    if (measuredFrequency < 0)
      measuredFrequency = 0;


    uint32_t percentFrequency = measuredFrequency * 100 / 23;

    if(percentFrequency > 100) percentFrequency = 100;

    int32_t k = 8;
    //double a = 0.1;

    //uint32_t rawPwm = manualModeSpeed + k*(manualModeSpeed - percentFrequency);
    int32_t erroAtual = ((manualModeSpeed - percentFrequency));
    int32_t rawPwm = ((int32_t)(k*erroAtual-(k*erroAnterior/a)+rawPwmAnterior));

    if(rawPwm <= 0) rawPwm = 0;
    if(rawPwm > 100) rawPwm = 100;

    erroAnterior = erroAtual;
    rawPwmAnterior = rawPwm;

    UARTprintf("%i,%i,%i,%i\n", percentFrequency, manualModeSpeed, rawPwm, erroAtual);

      uint32_t pwmValue = systemOnline ? ((int)(rawPwm * PWM_CLOCKS/100)) : 1;
      if (pwmValue >= PWM_CLOCKS)
        pwmValue = PWM_CLOCKS-1;
      else if (pwmValue <= 0)
        pwmValue = 1;
      //PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (int)pwmValue);
      PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, pwmValue);



    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void adcTask(void *pvParameters)
{
  // Enable the ADC0 module.
  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

  // Wait for the ADC0 module to be ready.
  while (!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0))
  {
  };

  // Enable the first sample sequencer to capture the value of channel 0 when
  // the processor trigger occurs.
  ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 0,
                           ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH0);
  ADCSequenceEnable(ADC0_BASE, 0);

  while (1)
  {
    // Trigger the sample sequence.
    ADCProcessorTrigger(ADC0_BASE, 0);

    // Wait until the sample sequence has completed.
    while (!ADCIntStatus(ADC0_BASE, 0, false))
    {
    };

    // Read the value from the ADC.
    ADCSequenceDataGet(ADC0_BASE, 0, &automaticModeSpeed);
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void oledTask(void *pvParameters)
{
  configureOLED();

  while (1)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

uint32_t getSpeed()
{
  return automaticMode ? measuredFrequency : (manualModeSpeed);
}
