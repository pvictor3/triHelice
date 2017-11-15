#include<stdint.h>
#include<stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#include "CMSIS.h"

#include"inc/tm4c123gh6pm.h"
#include"inc/hw_memmap.h"
#include"inc/hw_types.h"
#include"driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "driverlib/gpio.h"

#define TARGET_IS_BLIZZARD_RB1
#include "driverlib/rom.h"

void IntGPIOAHandler(void);
//*****************************************************************************
//
// Send a string to the UART.
//
//*****************************************************************************
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
        ROM_UARTCharPutNonBlocking(UART0_BASE, *s++);
    }
    va_end(args);
}

int main(void)
{
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN); //System clock = 40MHZ

	/*
	 * Configuración para usar el PWM Modulo 1, Generador 3, salidas PWM6 y PWM7
	 * */
	SYSCTL->RCGC2 |= (1<<5);    //Activa el reloj para GPIOF, PF2 y PF3 como M1PWM6 y M1PWM7
	SYSCTL->RCGCPWM = (1<<1);   //Activa pwm module 1

	GPIOF->DIR = (1<<3)|(1<<2); //PF2 y PF3 como salidas
	GPIOF->AFSEL |= (1<<2)|(1<<3);  //Alternate function para PF2 y PF3
	GPIOF->PCTL |= (5<<8)|(5<<12);  //Codigo para usar como PWM
	GPIOF->DEN = (1<<3)|(1<<2);     //Digital Enable
	SYSCTL->RCC |= (1<<20);         //Activa el divisor del system clock para el PWM
	SYSCTL->RCC &= ~(7<<17);         //Pone al divisor en 2

	PWM1->_3_CTL = 0x00;            //PWM generator for countdown mode and immediate updates to the paramters
	PWM1->_3_GENA = 0x8C;           //Cuando Count=Load la salida es alta, cuando Count=CMPA la salida es baja
	PWM1->_3_GENB = 0x80C;          //Cuando Count=Load la salida es alta, cuando Count=CMPB la salida es baja
	PWM1->_3_LOAD = 0x9C3;          //2499 para una frecuencia de 8KHz con un reloj de 20MHz

	PWM1->_3_CMPA = 0x4E1;          //Duty Cycle al 50%
	PWM1->_3_CMPB = 0x752;          //Duty Cycle al 25%

	PWM1->_3_CTL = (1<<0);             //PWM block enable
	PWM1->ENABLE = (1<<7)|(1<<6);   //Activa la salida para PWM6 y PWM7

	//
	// Enable processor interrupts.
	//
	ROM_IntMasterEnable();


	/*
	 * Configuracion para leer el receiver de 5 Canales
	 * */
	SYSCTL->RCGC2 |= (1<<0);        //Activa el reloj para GPIOA
	GPIOA->DIR &= ~((1<<2)|(1<<3)|(1<<4)|(1<<5));   //Pines como entradas
	GPIOA->DEN = (1<<2);            //Activamos la entrada que queremos leer
	GPIOA->IM = 0x00;               //Desactiva las interrupciones
	GPIOA->IS = 0x00;               //Deteccion de flancos
	GPIOA->IBE = (1<<2)|(1<<3)|(1<<4)|(1<<5);   //Detección de ambos flancos
	GPIOA->ICR = (1<<2)|(1<<3)|(1<<4)|(1<<5);   //Limpiamos cualquier interrupcion
	GPIOA->IM = (1<<2);             //Activamos la entrada que queremos leer

    //
    // Enable the GPIOA interrupt.
    //
    ROM_IntEnable(INT_GPIOA);

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

    /*
     * Configuracion del timer
     * */
    SYSCTL->RCGCTIMER = 0x01;
    TIMER0->CTL = 0x00;
    TIMER0->CFG = 0x00;
    TIMER0->TAMR = (1<<1)|(1<<4);
    TIMER0->TAILR = 0xFFFFFFFF;
    TIMER0->CTL = (1<<0);

	while(1);
}

void IntGPIOAHandler(void){
    uint32_t time = 0;
    if( (GPIOA->DATA & (1<<2) ) ){
        TIMER0->TAV = 0x00;
    }
    else{
        time = TIMER0->TAR;
        UARTSend("Tiempo = %d",time);
    }
    GPIOA->ICR = (1<<2);
}
