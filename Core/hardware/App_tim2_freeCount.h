#ifndef __APP_TIM2_FREECOUNT_H
#define __APP_TIM2_FREECOUNT_H

#include <stdint.h>

// TIM2 自由計數器初始化（PSC=719 → 10 µs/tick, 32-bit free-running）
void App_Tim2_FreeCount_Init(void);

// 讀取 TIM2 當前計數值（每 tick = 10 µs）
uint32_t App_Tim2_GetTick(void);

#endif /* __APP_TIM2_FREECOUNT_H */
