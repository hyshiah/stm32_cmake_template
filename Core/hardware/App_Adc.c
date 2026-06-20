// ADC1 單通道注入組驅動 — PB0(ADC12_IN8) TIM4 TRGO 觸發 + JEOC 中斷

#include "App_Adc.h"

ADC_HandleTypeDef hadc1;
volatile uint16_t pb0_voltage;

/* ── PB0 GPIO 初始化（類比模式） ── */
static void App_Adc_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_ADC_CONFIG(RCC_CFGR_ADCPRE_DIV6);     // PCLK2 / 6 = 12 MHz (≤14 MHz max)

    GPIO_InitStruct.Pin  = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* ── ADC1 NVIC 初始化 ── */
static void App_Adc_NVIC_Init(void)
{
    HAL_NVIC_SetPriority(ADC1_2_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
}

/* ── ADC1 初始化（公開接口） ──
 *
 * 必須在 App_Pwm_Init() 之後呼叫，確保 TIM4 已啟動且已配置 TRGO。
 */
void App_Adc_Init(void)
{
    App_Adc_GPIO_Init();

    /* ADC1 基礎配置 */
    hadc1.Instance                   = ADC1;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;    // 規則組不使用
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    HAL_ADC_Init(&hadc1);

    /* 注入組通道 8 (PB0) 配置 — TIM4_TRGO 觸發 */
    ADC_InjectionConfTypeDef sConfigInjected = {0};
    sConfigInjected.InjectedChannel           = ADC_CHANNEL_8;
    sConfigInjected.InjectedRank              = ADC_INJECTED_RANK_1;
    sConfigInjected.InjectedNbrOfConversion   = 1;
    sConfigInjected.InjectedSamplingTime      = ADC_SAMPLETIME_55CYCLES_5;  // ≈ 4.6 µs @ 12 MHz
    sConfigInjected.ExternalTrigInjecConv     = ADC_EXTERNALTRIGINJECCONV_T4_TRGO;
    sConfigInjected.AutoInjectedConv          = DISABLE;
    sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;
    sConfigInjected.InjectedOffset            = 0;
    HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected);

    /* ADC 校準 */
    HAL_ADCEx_Calibration_Start(&hadc1);

    App_Adc_NVIC_Init();

    /* 啟動注入組中斷模式，等待 TIM4 TRGO */
    HAL_ADCEx_InjectedStart_IT(&hadc1);
}

/******************************************************************************/
/*            ADC1_2 Interrupt Handler (overrides weak default)               */
/******************************************************************************/
void ADC1_2_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&hadc1);
}

/******************************************************************************/
/*            Injected Conversion Complete Callback (JEOC)                    */
/******************************************************************************/
void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        pb0_voltage = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
        /* 注入組外部觸發模式下，ADC 完成後會自動等待下一次 TRGO，
         * 但呼叫 re-arm 確保 JEOC 中斷保持啟用 */
        HAL_ADCEx_InjectedStart_IT(&hadc1);
    }
}
