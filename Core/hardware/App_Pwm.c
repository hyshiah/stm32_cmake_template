// PWM 輸出驅動 — TIM1_CH1 (PA8), TIM4_CH1 (PB6)

#include "App_Pwm.h"

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim4;

/* ── TIM1_CH1 GPIO 初始化 (PA8) ── */
static void App_Pwm1_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();

    GPIO_InitStruct.Pin   = GPIO_PIN_8;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* ── TIM4_CH1 GPIO 初始化 (PB6) ── */
static void App_Pwm4_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();

    GPIO_InitStruct.Pin   = GPIO_PIN_6;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* ── PWM 初始化（公開接口） ── */
/**
 * @brief 使用 Tim1 Tim4 生成 pwm ，tim4 供 ADC1 注入組觸發使用
 * @note  TIM1_CH1 (PA8) 用於發電機控制， TIM4_CH1 (PB6) 用於電機控制
 * @note  TIM4 主模式 TRGO 設定為 Update 事件，頻率 1 kHz，供 ADC1 注入組觸發使用
 */
void App_Pwm_Init(void)
{
    /* ===== TIM1_CH1 (PA8) ===== */
    App_Pwm1_GPIO_Init();

    htim1.Instance               = TIM1;
    htim1.Init.Prescaler         = 71;        // 72 MHz / 72 = 1 MHz
    htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim1.Init.Period            = 999;       // 1 MHz / 1000 = 1 kHz
    htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim1);

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode       = TIM_OCMODE_PWM1;
    sConfigOC.Pulse        = 0;              // 初始 0% 佔空比
    sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1);

    /* ===== TIM4_CH1 (PB6) ===== */
    App_Pwm4_GPIO_Init();

    htim4.Instance               = TIM4;
    htim4.Init.Prescaler         = 71;        // 72 MHz / 72 = 1 MHz
    htim4.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim4.Init.Period            = 999;       // 1 MHz / 1000 = 1 kHz
    htim4.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim4);

    HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1);

    /* 啟動 PWM 輸出 */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);

    /* TIM4 主模式 TRGO：Update → 1 kHz（供 ADC1 注入組觸發使用） */
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode    = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig);
}

/* ── 設定 TIM1_CH1 佔空比 (0–1000) ── */
void App_Pwm1_SetDuty(uint16_t duty)
{
    if (duty > 1000) duty = 1000;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty);
}

/* ── 設定 TIM4_CH1 佔空比 (0–1000) ── */
void App_Pwm4_SetDuty(uint16_t duty)
{
    if (duty > 1000) duty = 1000;
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, duty);
}
