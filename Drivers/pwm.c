#include "stdint.h"
#include "CMSIS.h"

void pwmInit(void){
    /*
     * Configuración para usar el PWM Modulo 0, Generador 3, salidas PWM6(PC4) y PWM7(PC5)
     * */
    GPIOC->DIR |= (1 << 4) | (1 << 5);                  //PC4 y PC5 como salidas
    GPIOC->AFSEL |= (1 << 4) | (1 << 5);                //Alternate function para PC4 y PC5
    GPIOC->PCTL |= (4 << 16) | (4 << 20);               //Codigo(4) para usar como PWM
    GPIOC->DEN |= (1 << 4) | (1 << 5);                  //Digital Enable

    SYSCTL->RCC |= (1 << 20);                           //Activa el divisor del system clock para el PWM
    SYSCTL->RCC &= ~(1 << 19);                          //Pone al divisor en 16, frecuencia de 2.5MHz

    PWM0->_3_CTL = 0x00;                                //PWM generator for countdown mode and immediate updates to the paramters
    PWM0->_3_GENA = 0x8C;                               //Cuando Count=Load la salida es alta, cuando Count=CMPA la salida es baja
    PWM0->_3_GENB = 0x80C;                              //Cuando Count=Load la salida es alta, cuando Count=CMPB la salida es baja
    PWM0->_3_LOAD = 0xC350;                              //50000 para una frecuencia de 50Hz con un reloj de 2.5MHz

    PWM0->_3_CMPA = 0xB4AA;                              //Duty Cycle al 50%
    PWM0->_3_CMPB = 0xAFC8;                              //Duty Cycle al 25%

    PWM0->_3_CTL = (1 << 0);                            //PWM block enable
    PWM0->ENABLE |= (1 << 7) | (1 << 6);                 //Activa la salida para PWM6 y PWM7

    /*
     * Configuración para usar el PWM Modulo 0, Generador 2, salida PWM5(PE5)
     * */
    GPIOE->DIR |= (1<<5);
    GPIOE->AFSEL |= (1<<5);
    GPIOE->PCTL |= (4<<20);
    GPIOE->DEN |= (1<<5);

    PWM0->_2_CTL = 0x00;
    PWM0->_2_GENB = 0x80C;
    PWM0->_2_LOAD = 0xC350;

    PWM0->_2_CMPB = 0xB98C;

    PWM0->_2_CTL = 0x01;
    PWM0->ENABLE |= (1<<5);
}

void changeDuty(uint8_t salida, uint32_t valor){
    switch(salida){
    case 1:
        PWM0->_3_CMPA = valor;
        break;
    case 2:
        PWM0->_3_CMPB = valor;
        break;
    case 3:
        PWM0->_2_CMPB = valor;
        break;
    default:
        break;
    }
}
