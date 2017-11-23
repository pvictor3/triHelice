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
     * Configuracion del timer de 32bits para contar microsegundos
     * */
    WTIMER0->CTL = 0x00;                //Desactiva el timer
    WTIMER0->CFG = 0x04;                //Seleccionamos el timer de 32bits
    WTIMER0->TAMR = (0x02)|(1<<4);      //Timer Periodic and counting up
    WTIMER0->TAILR = 0xFFFFFFFF;        //Limite superior
    WTIMER0->TAPR = 40;                 //Preescaler para contar cada microsegundo
    WTIMER0->CTL = 0x01;                //Inicia el timer
}
