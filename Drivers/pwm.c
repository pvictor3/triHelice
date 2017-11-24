#include "stdint.h"
#include "CMSIS.h"

void pwmInit(void){
    /*
     * Configuración para usar el PWM Modulo 1, Generador 3, salidas PWM6 y PWM7
     * */
    GPIOF->DIR |= (1 << 3) | (1 << 2);                  //PF2 y PF3 como salidas
    GPIOF->AFSEL |= (1 << 2) | (1 << 3);                //Alternate function para PF2 y PF3
    GPIOF->PCTL |= (5 << 8) | (5 << 12);                //Codigo(5) para usar como PWM
    GPIOF->DEN |= (1 << 3) | (1 << 2);       //Digital Enable

    SYSCTL->RCC |= (1 << 20);                           //Activa el divisor del system clock para el PWM
    SYSCTL->RCC &= ~(7 << 17);                          //Pone al divisor en 2, frecuencia de 20MHz

    PWM1->_3_CTL = 0x00;                                //PWM generator for countdown mode and immediate updates to the paramters
    PWM1->_3_GENA = 0x8C;                               //Cuando Count=Load la salida es alta, cuando Count=CMPA la salida es baja
    PWM1->_3_GENB = 0x80C;                              //Cuando Count=Load la salida es alta, cuando Count=CMPB la salida es baja
    PWM1->_3_LOAD = 0x9C3;                              //2499 para una frecuencia de 8KHz con un reloj de 20MHz

    PWM1->_3_CMPA = 0x4E1;                              //Duty Cycle al 50%
    PWM1->_3_CMPB = 0x752;                              //Duty Cycle al 25%

    PWM1->_3_CTL = (1 << 0);                            //PWM block enable
    PWM1->ENABLE = (1 << 7) | (1 << 6);                 //Activa la salida para PWM6 y PWM7

    /*
     * Configuración para usar el PWM Modulo 0, Generador 3, salida PWM6
     * */
    GPIOC->DIR |= (1<<4);
    GPIOC->AFSEL |= (1<<4);
    GPIOC->PCTL |= (4<<16);
    GPIOC->DEN |= (1<<4);

    PWM0->_3_CTL = 0x00;
    PWM0->_3_GENA = 0x8C;
    PWM0->_3_LOAD = 0x9C3;

    PWM0->_3_CMPA = 0x4E1;

    PWM0->_3_CTL = 0x01;
    PWM0->ENABLE = (1<<6);
}

void changeDuty(uint8_t salida, uint32_t valor){
    switch(salida){
    case 1:
        PWM0->_3_CMPA = valor;
        break;
    case 2:
        PWM1->_3_CMPA = valor;
        break;
    case 3:
        PWM1->_3_CMPB = valor;
        break;
    default:
        break;
    }
}
