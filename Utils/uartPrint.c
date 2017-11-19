/*
 * UARTSend.c
 *
 *  Created on: 19/11/2017
 *      Author: pvict
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/uart.h"
#define TARGET_IS_BLIZZARD_RB1
#include "driverlib/rom.h"


void
UARTSend(char *format, ...)
{
    char str[80];
    char *s;

    //Extract the argument list using VA apis
    va_list args;
    va_start(args, format);
    vsprintf(str, format, args);
    s = str;

    //
    // Loop while there are more characters to send.
    //
    while(*s)
    {
        //
        // Write the next character to the UART.
        //
        ROM_UARTCharPut(UART0_BASE, *s++);
    }
    va_end(args);
}

