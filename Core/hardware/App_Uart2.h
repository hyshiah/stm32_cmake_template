/**
 * 始能 stm32f103c8t6 uart2
 * 使用 hal 庫
 * 异步通信模式
 * Baud Rate (波特率)：115200 Bits/s Word Length (数据位)：8 Bits Parity (校验位)：None Stop Bits (停止位)：1
 * 全雙工 tx rx 均開啟
 * 使能對應的 DMA
 *
 * 开启串口中断 使能對應的 串口異步接收中斷 rx_complete_callback()
 * 清除中斷標示 將接收位元 由 tx 發送 像 "echo"
 *
 */

#ifndef __APP_UART2_H
#define __APP_UART2_H
#include "stm32f1xx_hal.h"

/* UART2 handle (defined in App_Uart2.c) */
extern UART_HandleTypeDef huart2;

/* RX byte buffer for interrupt-based async receive */
extern volatile uint8_t uart2_rx_byte;

void App_Uart2_init(void);

#endif