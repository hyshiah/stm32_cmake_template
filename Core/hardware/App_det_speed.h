#ifndef __APP_DET_SPEED_H
#define __APP_DET_SPEED_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h> // Include this for the macros
#include <string.h>    // for memset
// 环形缓冲区大小
#define SPEED_RING_BUFFER_SIZE 3
#define ENCODER_PULSE_PER_REV 22.0f  // 每圈脉冲数（根据实际情况修改）
extern void float_to_string(float value, uint8_t decimals, char* output);
// 初始化速度检测模块
void App_Speed_Init(void);

// 外部中断调用：记录当前 Tick 到环形缓冲区
// 此函数应在中断上下文中调用，执行时间极短
void App_Speed_Set(void);

// 主循环调用：获取当前速度对应的时间差（单位：毫秒）
// 返回值：当前 Tick 减去缓冲区中最后一个有效记录
//         如果缓冲区无效，返回 0xFFFFFFFF
uint32_t App_Speed_Get(void);

// 获取缓冲区当前写入索引（调试用）
uint8_t App_Speed_GetIndex(void);

// 检查缓冲区是否已满（调试用）
bool App_Speed_IsBufferReady(void);

// 返回速度检测结果（徑度/每秒）
float App_Det_Speed(void);

// 打印速度检测结果（调试用）
void App_Print_Det_Speed(void);
#endif /* __APP_DET_SPEED_H */