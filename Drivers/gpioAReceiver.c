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
     * Configuracion para leer el receiver de 5 Canales
     * */
    SYSCTL->RCGC2 |= (1<<0);        //Activa el reloj para GPIOA
    GPIOA->DIR &= ~((1<<2)|(1<<3)|(1<<4)|(1<<5));   //Pines como entradas
    GPIOA->DEN = (1<<2)|(1<<3)|(1<<4)|(1<<5);            //Activamos la entrada que queremos leer
    GPIOA->IM = 0x00;               //Desactiva las interrupciones
    GPIOA->IS = 0x00;               //Deteccion de flancos
    GPIOA->IBE = (1<<2)|(1<<3)|(1<<4)|(1<<5);   //Detección de ambos flancos
    GPIOA->ICR = (1<<2)|(1<<3)|(1<<4)|(1<<5);   //Limpiamos cualquier interrupcion
    GPIOA->IM = (1<<2);             //Activamos la entrada que queremos leer
}

