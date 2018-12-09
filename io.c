//*****************************************************************************
//
// io.c - I/O routines for the enet_io example application.
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
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "utils/ustdlib.h"
#include "io.h"
extern bool systemOnline;
extern bool automaticMode;
extern uint32_t automaticModeSpeed;
extern uint32_t manualModeSpeed;
extern uint32_t getSpeed();

//*****************************************************************************
//
// Hardware connection for the user LED.
//
//*****************************************************************************
#define LED_PORT_BASE GPIO_PORTN_BASE
#define LED_PIN GPIO_PIN_0

//*****************************************************************************
//
// Hardware connection for the animation LED.
//
//*****************************************************************************
#define LED_ANIM_PORT_BASE GPIO_PORTN_BASE
#define LED_ANIM_PIN GPIO_PIN_1

//*****************************************************************************
//
// The system clock speed.
//
//*****************************************************************************
extern uint32_t g_ui32SysClock;

//*****************************************************************************
//
// The current speed of the on-screen animation expressed as a percentage.
//
//*****************************************************************************
volatile unsigned long g_ulAnimSpeed = 10;


//*****************************************************************************
//
// Set the status LED on or off.
//
//*****************************************************************************
void
io_set_led(bool bOn)
{
    //
    // Turn the LED on or off as requested.
    //
    ROM_GPIOPinWrite(LED_PORT_BASE, LED_PIN, bOn ? LED_PIN : 0);
}

//*****************************************************************************
//
// Return LED state
//
//*****************************************************************************
void
io_get_ledstate(char * pcBuf, int iBufLen)
{

    usnprintf(pcBuf, iBufLen, systemOnline? "ON" : "OFF");
    //
    // Get the state of the LED
    //
    if(ROM_GPIOPinRead(LED_PORT_BASE, LED_PIN))
    {
        usnprintf(pcBuf, iBufLen, "ON");
    }
    else
    {
        usnprintf(pcBuf, iBufLen, "OFF");
    }

}

//*****************************************************************************
//
// Return LED state as an integer, 1 on, 0 off.
//
//*****************************************************************************
int
io_is_led_on(void)
{
    //
    // Get the state of the LED
    //
    if(ROM_GPIOPinRead(LED_PORT_BASE, LED_PIN))
    {
        return(true);
    }
    else
    {
        return(0);
    }
}

//*****************************************************************************
//
// Set the speed of the animation shown on the display.  In this version, the
// speed is described as a decimal number encoded as an ASCII string.
//
//*****************************************************************************
void
io_set_animation_speed_string(char *pcBuf)
{
    unsigned long ulSpeed;

    //
    // Parse the passed parameter as a decimal number.
    //
    ulSpeed = 0;
    while((*pcBuf >= '0') && (*pcBuf <= '9'))
    {
        ulSpeed *= 10;
        ulSpeed += (*pcBuf - '0');
        pcBuf++;
    }

    //
    // If the number is valid, set the new speed.
    //
    if(ulSpeed <= 100)
    {
        g_ulAnimSpeed = ulSpeed;
        manualModeSpeed = ulSpeed;
    }
}

//*****************************************************************************
//
// Set the speed of the animation shown on the display.
//
//*****************************************************************************
void
io_set_animation_speed(unsigned long ulSpeed)
{
    //
    // If the number is valid, set the new speed.
    //
    if(ulSpeed <= 100)
    {
        g_ulAnimSpeed = ulSpeed;
    }
}

//*****************************************************************************
//
// Get the current animation speed as an ASCII string.
//
//*****************************************************************************
void
io_get_animation_speed_string(char *pcBuf, int iBufLen)
{
    usnprintf(pcBuf, iBufLen, "%d%%", (int)(getSpeed()));
}

//*****************************************************************************
//
// Get the current animation speed as a number.
//
//*****************************************************************************
unsigned long
io_get_animation_speed(void)
{
    return(g_ulAnimSpeed);
}
