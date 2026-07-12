/**
 * PWM 輸出驅動
 * TIM1_CH1 → PA8：10 kHz，分辨率 1200（ARR=1199, PSC=5）
 * TIM4_CH1 → PB6：1 kHz，分辨率 1000（ARR=999, PSC=71）
 */

#ifndef __APP_PWM_H
#define __APP_PWM_H

#include "stm32f1xx_hal.h"


extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim4;

void App_Pwm_Init(void);
void App_Pwm1_SetDuty(uint16_t duty);  // 0–1200
void App_Pwm4_SetDuty(uint16_t duty);  // 0–1000

#endif
