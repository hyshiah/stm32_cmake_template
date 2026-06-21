// 編碼器初始化實現

#include "App_Encoder.h"
#include "stm32f1xx_hal.h"

extern volatile int32_t gen_counter; // 全局變量，記錄 Gen_Encoder 的計數
extern volatile int32_t motor_counter; // 全局變量，記錄 Motor_Encoder

void Gen_Encoder_Init(void) {
    // 初始化 AB 相編碼器
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 啟用 AFIO 時鐘（HAL_EXTI_SetConfigLine 寫 AFIO->EXTICR 需要）
    __HAL_RCC_AFIO_CLK_ENABLE();
    // 啟用 GPIOB 時鐘
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // 配置 PB14 和 PB15 為輸入模式，帶上拉電阻
    GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    EXTI_Init_gen();          // 配置 Gen_Encoder (PB14) EXTI
    NVIC_Init_gen();          // 啟用 EXTI15_10_IRQn
}

// 使用 HAL_EXTI_ConfigLine 配置 Gen_Encoder (PB14/PB15) 的 EXTI 中斷
void EXTI_Init_gen(void) {
    EXTI_HandleTypeDef hexti = {0};
    EXTI_ConfigTypeDef config = {0};

    // 配置 EXTI14 (PB14, Gen_Encoder A 相) — 雙邊沿中斷
    config.Line    = EXTI_LINE_14;
    config.Mode    = EXTI_MODE_INTERRUPT;
    config.Trigger = EXTI_TRIGGER_RISING_FALLING;
    config.GPIOSel = EXTI_GPIOB;   // HAL 內部會設定 AFIO_EXTICR
    HAL_EXTI_SetConfigLine(&hexti, &config);

    /* 只須A相中斷即可，B相用來判斷方向

    */
}

// 配置 Gen_Encoder NVIC 中斷
void NVIC_Init_gen(void) {
    // PB14 和 PB15 共用 EXTI15_10_IRQn
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/******************************************************************************/
/*            EXTI15_10 Interrupt Handler (overrides weak default)            */
/******************************************************************************/

void EXTI15_10_IRQHandler(void)
{
  EXTI_HandleTypeDef hexti = {0};

  /* PB14 — Gen_Encoder A 相 */
  hexti.Line = EXTI_LINE_14;
  if (HAL_EXTI_GetPending(&hexti, EXTI_TRIGGER_RISING_FALLING))
  {
    HAL_EXTI_ClearPending(&hexti, EXTI_TRIGGER_RISING_FALLING);
    /* TODO: 編碼器計數邏輯 */
    uint8_t a_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14); // 讀 A 相
    uint8_t b_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15); // 讀 B 相判斷方向

    if (a_state == b_state) {
        // 順時針
        gen_counter++; // 假設 counter 是全局變量
    } else {
        // 逆時針
        gen_counter--; // 假設 counter 是全局變量
    }
  }
}
void Motor_Encoder_init(void) {
    // 初始化 Motor 編碼器
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 關閉 JTAG 以釋放 PB3/PB4
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    // 啟用 GPIOB 時鐘
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // 配置 PB3 和 PB4 為輸入模式，帶上拉電阻
    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    Motor_EXTI_Init();  // 配置 Motor (PB3) EXTI
    Motor_NVIC_Init();  // 啟用 EXTI3_IRQn
}

// 配置 Motor_Encoder (PB3) 的 EXTI 中斷
void Motor_EXTI_Init(void) {
    EXTI_HandleTypeDef hexti = {0};
    EXTI_ConfigTypeDef config = {0};

    // 配置 EXTI3 (PB3, Motor A 相) — 雙邊沿中斷
    config.Line    = EXTI_LINE_3;
    config.Mode    = EXTI_MODE_INTERRUPT;
    config.Trigger = EXTI_TRIGGER_RISING_FALLING;
    config.GPIOSel = EXTI_GPIOB;
    HAL_EXTI_SetConfigLine(&hexti, &config);
}

// 配置 Motor_Encoder NVIC 中斷
void Motor_NVIC_Init(void) {
    HAL_NVIC_SetPriority(EXTI3_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);
}

/******************************************************************************/
/*            EXTI3 Interrupt Handler (overrides weak default)                */
/******************************************************************************/

void EXTI3_IRQHandler(void)
{
  EXTI_HandleTypeDef hexti = {0};

  /* PB3 — Motor_Encoder A 相 */
  hexti.Line = EXTI_LINE_3;
  if (HAL_EXTI_GetPending(&hexti, EXTI_TRIGGER_RISING_FALLING))
  {
    HAL_EXTI_ClearPending(&hexti, EXTI_TRIGGER_RISING_FALLING);
    /* TODO: 編碼器計數邏輯 (讀 PB4 判斷方向) */
    uint8_t a_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3); // 讀 A 相
    uint8_t b_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4); // 讀 B 相判斷方向
    if (a_state == b_state) {
        // 順時針
        motor_counter++; // 假設 counter 是全局變量
    } else {
        // 逆時針
        motor_counter--; // 假設 counter 是全局變量
    }
  }
}

