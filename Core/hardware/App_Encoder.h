// 聲明 AB相編碼器
// generate encoder pin 
// a pin: pb14
// b pin: pb15
// motor encoder pin
// a pin: pb3
// b pin: pb4
#ifndef __APP_ENCODER_H
#define __APP_ENCODER_H
#include "stm32f1xx_hal.h"
void Gen_Encoder_Init(void);
void Motor_Encoder_init(void);
void EXTI_Init_gen(void);
void NVIC_Init_gen(void);
void Motor_EXTI_Init(void);
void Motor_NVIC_Init(void);

#endif
