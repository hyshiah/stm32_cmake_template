#include "App_tim2_freeCount.h"
#include "stm32f1xx_hal.h"

static TIM_HandleTypeDef htim2;

/**
 * @brief TIM2 自由計數器初始化
 *        PSC = 719 → 72 MHz / 720 = 100 kHz → 每 tick = 10 µs
 *        Period = 0xFFFFFFFF → 32-bit free-running，不自動重載
 *        無需中斷，僅啟動計數器即可
 */
void App_Tim2_FreeCount_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 719;          // 72 MHz → 100 kHz
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 0xFFFFFFFF;   // 32-bit 最大值
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim2);
    HAL_TIM_Base_Start(&htim2);  // 開始計數
}

/**
 * @brief 讀取 TIM2 當前計數值
 * @return TIM2->CNT，每 tick = 10 µs
 * @note  可在中斷上下文中呼叫（僅讀取暫存器，無阻塞）
 */
uint32_t App_Tim2_GetTick(void)
{
    return TIM2->CNT;
}

/**
 * @brief 重置 TIM2 計數器為 0
 * @note  可在中斷上下文中呼叫
 */
void App_Tim2_Reset(void)
{
    TIM2->CNT = 0;
}
