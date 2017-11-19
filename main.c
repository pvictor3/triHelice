#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#include "CMSIS.h"
#include "drivers/pwm.h"
#include "drivers/gpioAReceiver.h"
#include "drivers/timer.h"
#include "utils/uartPrint.h"

#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "driverlib/gpio.h"
#include "driverlib/fpu.h"

#define TARGET_IS_BLIZZARD_RB1
#include "driverlib/rom.h"

uint32_t chanel = 2;
float time[4];

void IntGPIOAHandler(void){
    if( (GPIOA->DATA & (1<<chanel) ) ){
        TIMER0->TAV = 0x00;
        GPIOF->DATA |= (1<<1);
    }
    else{
        time[chanel-2] = (float)TIMER0->TAR / 40;
        if(chanel == 5){
                    chanel = 1;
                }
        chanel++;
        GPIOF->DATA &= ~(1<<1);
    }
    GPIOA->ICR = (1<<2)|(1<<3)|(1<<4)|(1<<5);
    GPIOA->IM = (1<<chanel);
    ROM_IntPendClear(INT_GPIOA);
}

int main(void)
{
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN); //System clock = 40MHZ

	//
	// Inicializa 3 salidas PWM
	//
	pwmInit();

	//
	//Inicializa el GPIOA para generar una interrupcion en ambos flancos
	//
	gpioAConfig();

    //
    // Enable the GPIOA interrupt.
    //
    ROM_IntEnable(INT_GPIOA);

    //
    //Inicializa el timer para contar microsegundos
    //
    timerConfig();

    //
    // Enable the peripherals used by UART0.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Set GPIO A0 and A1 as UART pins.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    //
    // Prompt for text to be entered.
    //
    UARTSend("Flight Controller");

    //
    // Enable processor interrupts.
    //
    ROM_IntMasterEnable();

	while(1){
	    UARTSend("Canal 2 = %f \n", time[0]);
	    UARTSend("Canal 3 = %f \n", time[1]);
	    UARTSend("Canal 4 = %f \n", time[2]);
	    UARTSend("Canal 5 = %f \n", time[3]);
	    ROM_SysCtlDelay(5000000);
	}
}
