/*
 * timer.c
 *
 *  Created on: 19/11/2017
 *      Author: pvict
 */
#include "stdint.h"
#include "CMSIS.h"

void timerConfig(void){
    /*
     * Configuracion del timer
     * */
    SYSCTL->RCGCTIMER = 0x01;
    TIMER0->CTL = 0x00;
    TIMER0->CFG = 0x00;
    TIMER0->TAMR = (1<<1)|(1<<4);
    TIMER0->TAILR = 0xFFFFFFFF;
    TIMER0->CTL = (1<<0);
}
