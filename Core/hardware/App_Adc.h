/**
 * ADC1 單通道注入組驅動
 * PB0 → ADC12_IN8
 * 由 TIM4 主模式 TRGO（Update @ 1 kHz）觸發注入組轉換
 * JEOC 中斷將 ADC 值寫入全域變數 pb0_voltage（raw 12-bit, 0–4095）
 */

#ifndef __APP_ADC_H
#define __APP_ADC_H

#include "stm32f1xx_hal.h"

extern volatile uint16_t pb0_voltage;

void App_Adc_Init(void);

#endif
