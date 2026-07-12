#ifndef __APP_DET_SPEED_H
#define __APP_DET_SPEED_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h> // Include this for the macros
#include <string.h>    // for memset
// 环形缓冲区大小

typedef struct {
    uint8_t last_index;
    uint8_t second_last_index; // PID controller for speed
    uint32_t last_tick;
    uint32_t second_last_tick;
} debug_index_t;

#define SPEED_RING_BUFFER_SIZE 3
#define ENCODER_PULSE_PER_REV 26.0f  // 雙邊沿觸發 13 線
extern void float_to_string(float value, uint8_t decimals, char* output);
// 初始化速度检测模块
void App_Speed_Init(void);

// 外部中断调用：记录当前 Tick 到环形缓冲区
// 此函数应在中断上下文中调用，执行时间极短
void App_Speed_Set(void);


// 获取缓冲区当前写入索引（调试用）
uint8_t App_Speed_GetIndex(void);

// 检查缓冲区是否已满（调试用）
bool App_Speed_IsBufferReady(void);

// 返回检测结果（tick /per 22.5 degree）
uint32_t App_Det_Ticks(void);

//返回速度检测结果（rad/second）
float App_Tick_to_speed(void);

// 打印速度检测结果（调试用）
debug_index_t calculate_index(void);
#endif /* __APP_DET_SPEED_H */