/*
    LED 閃爍 (PC13) — HAL 完整版
    時鐘樹：HSE 8MHz → PLL x9 → SYSCLK 72MHz
*/

#include "stm32f1xx_hal.h"
#include "App_Encoder.h"
#include "App_Uart2.h"
#include "App_Pwm.h"
#include "App_Adc.h"
#include "stm32f1xx_hal_conf.h"

#include <stdio.h>
#include <sys/stat.h>   // for _write retarget

volatile int32_t gen_counter = 0; // 全局變量，記錄 Gen_Encoder 的計數
volatile int32_t motor_counter = 0; // 全局變量，記錄 Motor_Encoder

// 要发送的3个通道数据：CH0(电压), CH1(角度), CH2(温度)
float data[3] = {3.14, 1.20, 25.60};

// 关键帧尾：0x00, 0x00, 0x80, 0x7f
uint8_t tail[4] = {0x00, 0x00, 0x80, 0x7f};

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

int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    Gen_Encoder_Init();   // 初始化 AB 相編碼器
    Motor_Encoder_init(); // 初始化 Motor 編碼器
    App_Uart2_init();         // 初始化 USART2（printf 輸出通道）
    App_Pwm_Init();           // 初始化 PWM（PA8=TIM1_CH1, PB6=TIM4_CH1, 1kHz）
    App_Adc_Init();           // 初始化 ADC1（PB0, TIM1 TRGO 觸發, EOC 中斷）

    /* 設定初始佔空比：PA8=50%, PB6=25% */
    App_Pwm1_SetDuty(500);
    App_Pwm4_SetDuty(250);

    while (1) {
        printf("hello world %d\r\n", pb0_voltage);
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
