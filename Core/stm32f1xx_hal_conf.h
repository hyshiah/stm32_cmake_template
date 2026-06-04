/**
  ******************************************************************************
  * @file    stm32f1xx_hal_conf.h
  * @brief   HAL configuration file for STM32F103C8T6 (Blue Pill) Blink project.
  ******************************************************************************
  */

#ifndef __STM32F1xx_HAL_CONF_H
#define __STM32F1xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* ===== CMSIS device header ===== */
#include "stm32f1xx.h"
#include "stm32f1xx_hal_def.h"

/* ===== Module Selection (only enable what's needed) ===== */
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED

/* ===== Oscillator Values ===== */
#define HSE_VALUE          8000000U   /* 8 MHz external crystal on Blue Pill */
#define HSE_STARTUP_TIMEOUT 100U
#define HSI_VALUE          8000000U
#define LSI_VALUE           40000U
#define LSE_VALUE           32768U
#define LSE_STARTUP_TIMEOUT 5000U

/* ===== System Clock ===== */
#define  VDD_VALUE          3300U
#define  TICK_INT_PRIORITY  0x0FU
#define  USE_RTOS           0U
#define  PREFETCH_ENABLE    1U

/* ===== Module headers (must come after module defines) ===== */
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_cortex.h"
#include "stm32f1xx_hal_flash.h"
#include "stm32f1xx_hal_exti.h"

/* ===== Assert macro ===== */
#if 0  /* set to 1 to enable full assertion checking */
  #define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t *file, uint32_t line);
#else
  #define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1xx_HAL_CONF_H */
