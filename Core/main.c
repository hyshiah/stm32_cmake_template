/**
  * @file    main.c
  * @brief   STM32F103C8T6 (Blue Pill) LED Blink using HAL library
  *
  *          Clock: HSE (8 MHz) → PLL ×9 → SYSCLK 72 MHz
  *          LED:   PC13, active low
  *
  * @note    Built with arm-none-eabi-gcc + CMake on Linux
  */

#include "stm32f1xx_hal.h"

#define LED_PIN         GPIO_PIN_13
#define LED_PORT        GPIOC

static void SystemClock_Config(void);
static void GPIO_Init(void);
void Error_Handler(void);

int main(void)
{
    /* 1. HAL 库初始化：设定 SysTick 中断优先级等底层 */
    HAL_Init();

    /* 2. 设定系统时钟：HSE → PLL → 72 MHz */
    SystemClock_Config();

    /* 3. 初始化 LED 引脚 */
    GPIO_Init();

    /* 4. 主循环：每隔 500 ms 翻转 LED */
    while (1)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(500);
    }
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* ── 振荡器配置 ── */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* ── 系统时钟与总线分频配置 ── */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_HCLK   |
                                       RCC_CLOCKTYPE_PCLK1  |
                                       RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能 GPIOC 端口的时钟 */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* 设定 PC13 为推挽输出，无上下拉，速度 2 MHz */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* 初始状态：PC13 = HIGH → LED 熄灭 */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

void Error_Handler(void)
{
    /* 错误发生时停机 */
    while (1) { }
}
