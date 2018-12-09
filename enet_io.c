//*****************************************************************************
//
// enet_io.c - I/O control via a web server.
//
// Copyright (c) 2013-2017 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
// This is part of revision 2.1.4.178 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************
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
#include "cgifuncs.h"

// FreeRTOS includes
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

void ethernetTask(void *pvParameters);
void testTask(void *pvParameters);
void demoSerialTask(void *pvParameters);
void adcTask(void *pvParameters);
void pwmTask(void *pvParameters);
uint32_t getSpeed();

bool systemOnline = false;
bool automaticMode = true;
uint32_t automaticModeSpeed = 0;
uint32_t manualModeSpeed = 0;
uint32_t timerValue = 0;
uint32_t interruptValue = 0;
uint32_t timerEntries = 0;
bool firstPulse = true;
uint32_t firstTime = 0;
uint32_t secondTime = 0;
uint32_t measuredFrequency = 0;

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Ethernet-based I/O Control (enet_io)</h1>
//!
//! This example application demonstrates web-based I/O control using the
//! Tiva Ethernet controller and the lwIP TCP/IP Stack.  DHCP is used to
//! obtain an Ethernet address.  If DHCP times out without obtaining an
//! address, a static IP address will be chosen using AutoIP.  The address that
//! is selected will be shown on the UART allowing you to access the
//! internal web pages served by the application via your normal web browser.
//!
//! Two different methods of controlling board peripherals via web pages are
//! illustrated via pages labeled ``IO Control Demo 1'' and ``IO Control
//! Demo 2'' in the navigation menu on the left of the application's home page.
//! In both cases, the example allows you to toggle the state of the user LED
//! on the board and set the speed of a blinking LED.
//!
//! ``IO Control Demo 1'' uses JavaScript running in the web browser to send
//! HTTP requests for particular special URLs.  These special URLs are
//! intercepted in the file system support layer (io_fs.c) and used to
//! control the LED and animation LED.  Responses generated by the board are
//! returned to the browser and inserted into the page HTML dynamically by
//! more JavaScript code.
//!
//! ``IO Control Demo 2'' uses standard HTML forms to pass parameters to CGI
//! (Common Gateway Interface) handlers running on the board.  These handlers
//! process the form data and control the animation and LED as requested before
//! sending a response page (in this case, the original form) back to the
//! browser.  The application registers the names and handlers for each of its
//! CGIs with the HTTPD server during initialization and the server calls
//! these handlers after parsing URL parameters each time one of the CGI URLs
//! is requested.
//!
//! Information on the state of the various controls in the second demo is
//! inserted into the served HTML using SSI (Server Side Include) tags which
//! are parsed by the HTTPD server in the application.  As with the CGI
//! handlers, the application registers its list of SSI tags and a handler
//! function with the web server during initialization and this handler is
//! called whenever any registered tag is found in a .shtml, .ssi, .shtm or
//! .xml file being served to the browser.
//!
//! In addition to LED and animation speed control, the second example also
//! allows a line of text to be sent to the board for output to the UART.
//! This is included to illustrate the decoding of HTTP text strings.
//!
//! Note that the web server used by this example has been modified from the
//! example shipped with the basic lwIP package.  Additions include SSI and
//! CGI support along with the ability to have the server automatically insert
//! the HTTP headers rather than having these built in to the files in the
//! file system image.
//!
//! Source files for the internal file system image can be found in the ``fs''
//! directory.  If any of these files are changed, the file system image
//! (io_fsdata.h) should be rebuilt by running the following command from the
//! enet_io directory:
//!
//! ../../../../tools/bin/makefsfile -i fs -o io_fsdata.h -r -h -q
// C:\ti\TivaWare_C_Series-2.1.4.178\tools\bin\makefsfile -i fs -o io_fsdata.h -r -h -q
//!
//! For additional details on lwIP, refer to the lwIP web page at:
//! http://savannah.nongnu.org/projects/lwip/
//
//*****************************************************************************

//*****************************************************************************
//
// Defines for setting up the system clock.
//
//*****************************************************************************
#define SYSTICKHZ 100
#define SYSTICKMS (1000 / SYSTICKHZ)

//*****************************************************************************
//
// Interrupt priority definitions.  The top 3 bits of these values are
// significant with lower values indicating higher priority interrupts.
//
//*****************************************************************************
#define SYSTICK_INT_PRIORITY 0x80
#define ETHERNET_INT_PRIORITY 0xC0

//*****************************************************************************
//
// A set of flags.  The flag bits are defined as follows:
//
//     0 -> An indicator that the animation timer interrupt has occurred.
//
//*****************************************************************************
#define FLAG_TICK 0
static volatile unsigned long g_ulFlags;

//*****************************************************************************
//
// External Application references.
//
//*****************************************************************************
extern void httpd_init(void);

//*****************************************************************************
//
// SSI tag indices for each entry in the g_pcSSITags array.
//
//*****************************************************************************
#define SSI_INDEX_LEDSTATE 0
#define SSI_INDEX_FORMVARS 1
#define SSI_INDEX_SPEED 2

//*****************************************************************************
//
// This array holds all the strings that are to be recognized as SSI tag
// names by the HTTPD server.  The server will call SSIHandler to request a
// replacement string whenever the pattern <!--#tagname--> (where tagname
// appears in the following array) is found in ".ssi", ".shtml" or ".shtm"
// files that it serves.
//
//*****************************************************************************
static const char *g_pcConfigSSITags[] =
    {
        "LEDtxt",   // SSI_INDEX_LEDSTATE
        "FormVars", // SSI_INDEX_FORMVARS
        "speed"     // SSI_INDEX_SPEED
};

//*****************************************************************************
//
// The number of individual SSI tags that the HTTPD server can expect to
// find in our configuration pages.
//
//*****************************************************************************
#define NUM_CONFIG_SSI_TAGS (sizeof(g_pcConfigSSITags) / sizeof(char *))

//*****************************************************************************
//
// Prototypes for the various CGI handler functions.
//
//*****************************************************************************
static char *ControlCGIHandler(int32_t iIndex, int32_t i32NumParams,
                               char *pcParam[], char *pcValue[]);
static char *SetTextCGIHandler(int32_t iIndex, int32_t i32NumParams,
                               char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
// Prototype for the main handler used to process server-side-includes for the
// application's web-based configuration screens.
//
//*****************************************************************************
static int32_t SSIHandler(int32_t iIndex, char *pcInsert, int32_t iInsertLen);

//*****************************************************************************
//
// CGI URI indices for each entry in the g_psConfigCGIURIs array.
//
//*****************************************************************************
#define CGI_INDEX_CONTROL 0
#define CGI_INDEX_TEXT 1

//*****************************************************************************
//
// This array is passed to the HTTPD server to inform it of special URIs
// that are treated as common gateway interface (CGI) scripts.  Each URI name
// is defined along with a pointer to the function which is to be called to
// process it.
//
//*****************************************************************************
static const tCGI g_psConfigCGIURIs[] =
    {
        {"/iocontrol.cgi", (tCGIHandler)ControlCGIHandler}, // CGI_INDEX_CONTROL
        {"/settxt.cgi", (tCGIHandler)SetTextCGIHandler}     // CGI_INDEX_TEXT
};

//*****************************************************************************
//
// The number of individual CGI URIs that are configured for this system.
//
//*****************************************************************************
#define NUM_CONFIG_CGI_URIS (sizeof(g_psConfigCGIURIs) / sizeof(tCGI))

//*****************************************************************************
//
// The file sent back to the browser by default following completion of any
// of our CGI handlers.  Each individual handler returns the URI of the page
// to load in response to it being called.
//
//*****************************************************************************
#define DEFAULT_CGI_RESPONSE "/io_cgi.ssi"

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

//*****************************************************************************
//
// Timeout for DHCP address request (in seconds).
//
//*****************************************************************************
#ifndef DHCP_EXPIRE_TIMER_SECS
#define DHCP_EXPIRE_TIMER_SECS 45
#endif

//*****************************************************************************
//
// The current IP address.
//
//*****************************************************************************
uint32_t g_ui32IPAddress;

//*****************************************************************************
//
// The system clock frequency.  Used by the SD card driver.
//
//*****************************************************************************
uint32_t g_ui32SysClock;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// This CGI handler is called whenever the web browser requests iocontrol.cgi.
//
//*****************************************************************************
static char *
ControlCGIHandler(int32_t iIndex, int32_t i32NumParams, char *pcParam[],
                  char *pcValue[])
{
    int32_t i32LEDState, i32Speed;
    bool bParamError;

    //
    // We have not encountered any parameter errors yet.
    //
    bParamError = false;
    UARTprintf("Decisions!\n");

    //
    // Get each of the expected parameters.
    //
    i32LEDState = FindCGIParameter("LEDOn", pcParam, i32NumParams);
    i32Speed = GetCGIParam("speed_percent", pcParam, pcValue, i32NumParams,
                           &bParamError);

    //
    // Was there any error reported by the parameter parser?
    //
    if (bParamError || (i32Speed < 0) || (i32Speed > 100))
    {
        return (PARAM_ERROR_RESPONSE);
    }

    //
    // We got all the parameters and the values were within the expected ranges
    // so go ahead and make the changes.
    //
    io_set_led((i32LEDState == -1) ? false : true);
    io_set_animation_speed(i32Speed);

    //
    // Send back the default response page.
    //
    return (DEFAULT_CGI_RESPONSE);
}

//*****************************************************************************
//
// This CGI handler is called whenever the web browser requests settxt.cgi.
//
//*****************************************************************************
static char *
SetTextCGIHandler(int32_t i32Index, int32_t i32NumParams, char *pcParam[],
                  char *pcValue[])
{
    UARTprintf("Decisions!\n");
    long lStringParam;
    char pcDecodedString[48];

    //
    // Find the parameter that has the string we need to display.
    //
    lStringParam = FindCGIParameter("DispText", pcParam, i32NumParams);

    //
    // If the parameter was not found, show the error page.
    //
    if (lStringParam == -1)
    {
        return (PARAM_ERROR_RESPONSE);
    }

    //
    // The parameter is present. We need to decode the text for display.
    //
    DecodeFormString(pcValue[lStringParam], pcDecodedString, 48);

    //
    // Print sting over the UART
    //
    UARTprintf(pcDecodedString);
    UARTprintf("\n");

    //
    // Tell the HTTPD server which file to send back to the client.
    //
    return (DEFAULT_CGI_RESPONSE);
}

//*****************************************************************************
//
// This function is called by the HTTP server whenever it encounters an SSI
// tag in a web page.  The iIndex parameter provides the index of the tag in
// the g_pcConfigSSITags array. This function writes the substitution text
// into the pcInsert array, writing no more than iInsertLen characters.
//
//*****************************************************************************
static int32_t
SSIHandler(int32_t iIndex, char *pcInsert, int32_t iInsertLen)
{
    //
    // Which SSI tag have we been passed?
    //
    switch (iIndex)
    {
    case SSI_INDEX_LEDSTATE:
        io_get_ledstate(pcInsert, iInsertLen);
        break;

    case SSI_INDEX_FORMVARS:
        usnprintf(pcInsert, iInsertLen,
                  "%sls=%d;\nsp=%d;\n%s",
                  JAVASCRIPT_HEADER,
                  io_is_led_on(),
                  io_get_animation_speed(),
                  JAVASCRIPT_FOOTER);
        break;

    case SSI_INDEX_SPEED:
        io_get_animation_speed_string(pcInsert, iInsertLen);
        break;

    default:
        usnprintf(pcInsert, iInsertLen, "??");
        break;
    }

    //
    // Tell the server how many characters our insert string contains.
    //
    return (strlen(pcInsert));
}

//*****************************************************************************
//
// The interrupt handler for the SysTick interrupt.
//
//*****************************************************************************
void SysTickIntHandler(void)
{
    //
    // Call the lwIP timer handler.
    //
    lwIPTimer(SYSTICKMS);
}

//*****************************************************************************
//
// The interrupt handler for the timer used to pace the animation.
//
//*****************************************************************************
void AnimTimerIntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    MAP_TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);

    //
    // Indicate that a timer interrupt has occurred.
    //
    HWREGBITW(&g_ulFlags, FLAG_TICK) = 1;
}

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
            UARTprintf("Waiting for link.\n");
        }
        else if (ui32NewIPAddress == 0)
        {
            //
            // There is no IP address, so indicate that the DHCP process is
            // running.
            //
            UARTprintf("Waiting for IP address.\n");
        }
        else
        {
            //
            // Display the new IP address.
            //
            UARTprintf("IP Address: ");
            DisplayIPAddress(ui32NewIPAddress);
            UARTprintf("\n");
            UARTprintf("Open a browser and enter the IP address.\n");
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

    //
    // Configure debug port for internal use.
    //
    UARTStdioConfig(0, 115200, g_ui32SysClock);

    //
    // Clear the terminal and print a banner.
    //
    UARTprintf("\033[2J\033[H");
    UARTprintf("Ethernet IO Example\n\n");

    //
    // Configure SysTick for a periodic interrupt.
    //
    MAP_SysTickPeriodSet(g_ui32SysClock / SYSTICKHZ);
    MAP_SysTickEnable();
    MAP_SysTickIntEnable();

    //
    // Configure the hardware MAC address for Ethernet Controller filtering of
    // incoming packets.  The MAC address will be stored in the non-volatile
    // USER0 and USER1 registers.
    //
    MAP_FlashUserGet(&ui32User0, &ui32User1);
    if ((ui32User0 == 0xffffffff) || (ui32User1 == 0xffffffff))
    {
        //
        // Let the user know there is no MAC address
        //
        UARTprintf("No MAC programmed!\n");

        while (1)
        {
        }
    }

    //
    // Tell the user what we are doing just now.
    //
    UARTprintf("Waiting for IP.\n");

    //
    // Convert the 24/24 split MAC address from NV ram into a 32/16 split
    // MAC address needed to program the hardware registers, then program
    // the MAC address into the Ethernet Controller registers.
    //
    pui8MACArray[0] = ((ui32User0 >> 0) & 0xff);
    pui8MACArray[1] = ((ui32User0 >> 8) & 0xff);
    pui8MACArray[2] = ((ui32User0 >> 16) & 0xff);
    pui8MACArray[3] = ((ui32User1 >> 0) & 0xff);
    pui8MACArray[4] = ((ui32User1 >> 8) & 0xff);
    pui8MACArray[5] = ((ui32User1 >> 16) & 0xff);

    //
    // Initialze the lwIP library, using DHCP.
    //
    lwIPInit(g_ui32SysClock, pui8MACArray, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(pui8MACArray);
    LocatorAppTitleSet("EK-TM4C1294XL enet_io");

    //
    // Initialize a sample httpd server.
    //
    httpd_init();

    //
    // Set the interrupt priorities.  We set the SysTick interrupt to a higher
    // priority than the Ethernet interrupt to ensure that the file system
    // tick is processed if SysTick occurs while the Ethernet handler is being
    // processed.  This is very likely since all the TCP/IP and HTTP work is
    // done in the context of the Ethernet interrupt.
    //
    MAP_IntPrioritySet(INT_EMAC0, ETHERNET_INT_PRIORITY);
    MAP_IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);

    //
    // Pass our tag information to the HTTP server.
    //
    http_set_ssi_handler((tSSIHandler)SSIHandler, g_pcConfigSSITags,
                         NUM_CONFIG_SSI_TAGS);

    //
    // Pass our CGI handlers to the HTTP server.
    //
    http_set_cgi_handlers(g_psConfigCGIURIs, NUM_CONFIG_CGI_URIS);

    //
    // Initialize IO controls
    //
    io_init();

    //
    // Loop forever, processing the on-screen animation.  All other work is
    // done in the interrupt handlers.
    //
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
        //UARTprintf("\r\nFirst!!: %i.", (int)(firstTime));
            firstPulse = false;
    }
    else
    {
        measuredFrequency = firstTime - TimerValueGet64(TIMER0_BASE);
        //UARTprintf("\r\nSecond!!: %i.", (int)(measuredFrequency));

        if (measuredFrequency >= 2000000)
        {
            TimerLoadSet64(TIMER0_BASE, g_ui32SysClock);
            automaticModeSpeed = measuredFrequency;
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

    // Configure TimerA as a half-width one-shot timer, and TimerB as a
    // half-width edge capture counter.
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32SysClock);
    // Set the count time for the the one-shot timer (TimerA).
    //
    //TimerLoadSet64(TIMER0_BASE, 0);

    // TimerLoadSet(TIMER0_BASE, TIMER_A, 3000);
    //TimerLoadSet(TIMER0_BASE, TIMER_A, 3000);
    //
    // Configure the counter (TimerB) to count both edges.
    //
    // TimerControlEvent(TIMER0_BASE, TIMER_B, TIMER_EVENT_NEG_EDGE);

    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    //GPIOPinConfigure(GPIO_PD1_T0CCP1);

    //GPIOPinTypeTimer(GPIO_PORTD_AHB_BASE, GPIO_PIN_1);
    //GPIOIntTypeSet(GPIO_PORTD_AHB_BASE, GPIO_PIN_1, GPIO_FALLING_EDGE);

    //
    // Enable the timers.
    //
    TimerEnable(TIMER0_BASE, TIMER_A);
}

Timer0BIntHandler(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    // timerEntries++;

    measuredFrequency = 0;
    //UARTprintf("\r\nSecond!!: %i.", (int)(measuredFrequency));

    TimerLoadSet64(TIMER0_BASE, g_ui32SysClock);
    //UARTprintf("\r\nTIMER!");

    // if(firstPulse)
    // {
    //     firstTime = TimerValueGet64(TIMER0_BASE);
    //     firstPulse = false;
    // }
    // else
    // {
    //     measuredFrequency = TimerValueGet64(TIMER0_BASE) - firstPulse;
    //     firstPulse = true;
    // }

    timerValue = interruptValue; ///TimerValueGet(TIMER0_BASE, TIMER_B);

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
    UARTprintf("\r\nHello, world from FreeRTOS 9.0!");

    for (;;)
    {
        // UARTprintf("\r\nTimer interrupts: %i.", (int)(timerEntries));
        // UARTprintf("\r\nTimer interrupts: %i.", (int)(timerValue));
        //    UARTprintf("\r\nGPIO Interrupts: %i.", (int)(interruptValue));
        UARTprintf("%i | ", ((int)(g_ui32SysClock / measuredFrequency)));
        // UARTprintf("\r\nVelocidade Atual de PWM: %i.", (int)(getSpeed() * 400.0 / 4096.0));
        vTaskDelay(100 / portTICK_PERIOD_MS);
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
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, 400);

    // Start the timers in generator 0.
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);

    // Enable the outputs.
    PWMOutputState(PWM0_BASE, (PWM_OUT_4_BIT), true);

    while (1)
    {
        uint32_t pwmValue = systemOnline ? (getSpeed() * 400.0 / 30.0) : 1;
        if (pwmValue >= 400)
            pwmValue = 399;
        else if (pwmValue <= 0)
            pwmValue = 1;
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (int)pwmValue);

        vTaskDelay(10 / portTICK_PERIOD_MS);
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

uint32_t getSpeed()
{
    return automaticMode ? ((int)(g_ui32SysClock / measuredFrequency)) : manualModeSpeed;
}
