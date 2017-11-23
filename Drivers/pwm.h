/*
 * pwm.h
 *
 *  Created on: 19/11/2017
 *      Author: pvict
 */


#ifndef DRIVERS_PWM_H_
#define DRIVERS_PWM_H_

void pwmInit(void);

void changeDuty(uint8_t salida, uint32_t valor);



#endif /* DRIVERS_PWM_H_ */
