/*
 * gpioAReceiver.c
 *
 *  Created on: 19/11/2017
 *      Author: pvict
 */
#include "stdint.h"
#include "CMSIS.h"

void gpioAConfig(void){
    /*
     * Configuracion para leer los 4 primeros canales en el GPIOA(PA2-PA5)
     * */
    GPIOA->DIR &= ~((1<<2)|(1<<3)|(1<<4)|(1<<5));           //Pines como entradas
    GPIOA->DEN |= (1<<2)|(1<<3)|(1<<4)|(1<<5);              //Activamos las entrada que queremos leer
    GPIOA->IM = 0x00;                                       //Desactiva las interrupciones
    GPIOA->IS = 0x00;                                       //Deteccion de flancos
    GPIOA->IBE = (1<<2)|(1<<3)|(1<<4)|(1<<5);               //Detección de ambos flancos
    GPIOA->ICR = (1<<2)|(1<<3)|(1<<4)|(1<<5);               //Limpiamos cualquier interrupcion
    GPIOA->IM |= (1<<2)|(1<<3)|(1<<4)|(1<<5);               //Activamos las entradas que queremos leer

    /*
     * Configuracion para leer el canal 5 en el GPIOF(PF4)
     * */
    GPIOF->DIR &= ~(1<<4);
    GPIOF->DEN |= (1<<4);
    GPIOF->IM = 0x00;
    GPIOF->IS = 0x00;
    GPIOF->IBE = (1<<4);
    GPIOF->ICR = (1<<4);
    GPIOF->IM |= (1<<4);
}

