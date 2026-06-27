/*
    LED 閃爍 (PC13) — HAL 完整版
    時鐘樹：HSE 8MHz → PLL x9 → SYSCLK 72MHz
*/

#include "stm32f1xx_hal.h"
#include "App_Encoder.h"
#include "App_Uart2.h"
#include "App_Pwm.h"
#include "App_Adc.h"
#include "App_det_speed.h" // 包含速度检测模块的头文件
#include "App_tim2_freeCount.h"
#include "stm32f1xx_hal_conf.h"
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h> // Include this for the macros
#include <string.h>    // for memset

#include <sys/stat.h>   // for _write retarget
#define ENCODER_PULSE_PER_REV 22.0f  // 每圈脉冲数（根据实际情况修改）
volatile int32_t gen_counter = 0; // 全局變量，記錄 Gen_Encoder 的計數
volatile int32_t motor_counter = 0; // 全局變量，記錄 Motor_Encoder
volatile uint32_t time_diff = 0; // 全局變量，記錄速度對應的時間差

// 要发送的3个通道数据：CH0(电压), CH1(角度), CH2(温度)
float data[3] = {3.14, 1.20, 25.60};

// 关键帧尾：0x00, 0x00, 0x80, 0x7f
uint8_t tail[4] = {0x00, 0x00, 0x80, 0x7f};

/**
 * @brief 将浮点数转换为字符串（手动实现）
 * @param value 要转换的浮点数
 * @param decimals 小数位数
 * @param output 输出缓冲区（必须足够大）
 */
void float_to_string(float value, uint8_t decimals, char* output) {
    char temp[32];
    uint32_t int_part;
    uint32_t frac_part;
    uint32_t mult = 1;
    uint8_t is_negative = 0;
    int len = 0;
    
    // 处理负数
    if (value < 0) {
        is_negative = 1;
        value = -value;
    }
    
    // 计算乘数 (10^decimals)
    for (uint8_t i = 0; i < decimals; i++) {
        mult *= 10;
    }
    
    // 分离整数和小数部分（+0.5 实现四舍五入）
    int_part = (uint32_t)value;
    frac_part = (uint32_t)((value - int_part) * mult + 0.5f);
    
    // 处理小数部分进位（如 0.999 -> 1.000）
    if (frac_part >= mult) {
        int_part += 1;
        frac_part -= mult;
    }
    
    // 格式化整数部分
    if (is_negative) {
        len += sprintf(temp + len, "-");
    }
    len += sprintf(temp + len, "%lu", int_part);
    temp[len++] = '.';
    
    // 格式化小数部分（补零）
    char frac_str[16];
    sprintf(frac_str, "%0*lu", decimals, frac_part);
    strcpy(temp + len, frac_str);
    
    // 复制到输出缓冲区
    strcpy(output, temp);
}

static void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* HSE 8MHz + PLL x9 = 72MHz */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        /* HSE 起振失敗，停在這裡（Debug用） */
        while (1);
    }

    /* SYSCLK = PLL, HCLK = 72MHz, APB1 = 36MHz, APB2 = 72MHz */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void det_speed(void); // 前置声明速度检测函数

int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    Gen_Encoder_Init();   // 初始化 AB 相編碼器
    Motor_Encoder_init(); // 初始化 Motor 編碼器
    App_Uart2_init();         // 初始化 USART2（printf 輸出通道）
    App_Pwm_Init();           // 初始化 PWM（PA8=TIM1_CH1, PB6=TIM4_CH1, 1kHz）
    App_Adc_Init();           // 初始化 ADC1（PB0, TIM1 TRGO 觸發, EOC 中斷）
    App_Tim2_FreeCount_Init(); // 初始化 TIM2 自由計數器（10 µs/tick，供速度檢測用）
    App_Speed_Init();         // 初始化速度检测模块

    /* 設定初始佔空比：PA8=50%, PB6=25% */
    App_Pwm1_SetDuty(50);
    App_Pwm4_SetDuty(250);

    while (1) {
        //printf("hello world %d\r\n", pb0_voltage);
        printf("Generator count: %" PRId32 "\r\n", gen_counter);
        det_speed(); // 调用速度检测函数，输出时间差和频率
        //HAL_UART_Transmit(&huart2, (uint8_t*)"Hello", 5, 100);
        // 通过串口发送 (以HAL库为例 上位機用)
        //HAL_UART_Transmit(&huart2, (uint8_t*)data, sizeof(data), 0xFFFF);
        //HAL_UART_Transmit(&huart2, tail, 4, 0xFFFF);
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(500);
    }
}

/* 重定向 printf → USART2 */
int _write(int file, char *ptr, int len) {
    (void)file;
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

void det_speed(void) {
    char buffer[80];
    // 获取当前速度对应的时间差
    time_diff = App_Speed_Get();
    if (time_diff == 0xFFFFFFFF) {
            // 缓冲区未准备好
        printf("Speed module not ready yet!\r\n");
    } else if (time_diff == 0) {
        // 理论上不应该为 0，除非在同一个 Tick 内触发了两次中断
        printf("Time diff is 0! Too fast!\r\n");
    } else {
        // 計算頻率：100000 * 10µs / time_diff = Hz
        // 例如：time_diff = 10000（= 100ms），則頻率 = 10Hz
        float freq = 100000.0f / time_diff;               // 原始頻率 (Hz)
        float freq_corrected = freq / ENCODER_PULSE_PER_REV;  // 实际物理频率
        float rpm = freq_corrected * 60.0f;               // 转速 (RPM)
        // 格式化成字符串
        // 手动转换浮点数
        char freq_str[16];
        float_to_string(freq_corrected, 2, freq_str);  // 保留2位小数
        char rpm_str[16];
        float_to_string(rpm, 1, rpm_str);   // 保留1位小数
        snprintf(buffer, sizeof(buffer),
                 "dt=%" PRId32 " *10us  freq=%s Hz  RPM=%s\r\n",
                 time_diff, freq_str, rpm_str);
        // 发送到串口
        printf("%s", buffer);                 
    } 
}