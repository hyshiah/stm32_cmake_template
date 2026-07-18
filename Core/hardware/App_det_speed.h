#ifndef __APP_DET_SPEED_H
#define __APP_DET_SPEED_H

#include <stdint.h>
#include <stdbool.h>

#define SPEED_RING_BUFFER_SIZE 3   /* 保留仅用于兼容（不再使用） */
#define ENCODER_PULSE_PER_REV 26.0f  /* 雙邊沿觸發 13 線 */

void App_Speed_Init(void);
void App_Speed_Set(void);
uint32_t App_Det_Ticks(void);
float App_Tick_to_speed(void);

#endif /* __APP_DET_SPEED_H */