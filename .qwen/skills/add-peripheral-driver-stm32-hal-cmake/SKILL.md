---
name: Add peripheral driver to STM32 HAL CMake project
description: 为不使用 CubeMX 的 STM32 裸机 CMake 项目添加新的外设驱动（UART、SPI、I2C、TIM 等）
source: auto-skill
extracted_at: '2026-06-22T07:41:57.054Z'
---

# 为 STM32 HAL CMake 项目添加外设驱动

该流程适用于**不使用 CubeMX**、手动管理 HAL 模块的 CMake 裸机项目。以本项目中添加 UART2（含 DMA + 中断 Echo）为例，总结出通用步骤。

## 1. 探索项目现有模式

在写任何代码之前，先理解项目的组织习惯：

```
Core/
├── main.c              — 初始化入口，调用各驱动 Init()
├── stm32f1xx_it.c      — 系统中断（SysTick 等），但外设 ISR 未必放这里
├── stm32f1xx_hal_conf.h — HAL 模块开关（手动管理！无 CubeMX）
├── system_stm32f1xx.c
└── hardware/            — 外设驱动放在这里
    ├── App_Uart2.c/h
    ├── App_Encoder.c/h     ← 参考现有驱动的风格
```

**关键观察点：**
- 中断处理函数放在哪里？参考项目——`App_Encoder.c` 中直接定义 `EXTI15_10_IRQHandler` 和 `EXTI3_IRQHandler`（覆盖 startup 的 weak 符号），而非放在 `stm32f1xx_it.c`。**保持风格一致**。
- 初始化函数如何拆分？`App_Encoder.c` 将 GPIOD 初始化、EXTI 配置、NVIC 配置拆为独立子函数，然后由顶层函数依次调用。
- `CMakeLists.txt` 中 `HAL_SOURCES` 和 `APP_SOURCES` 是手动维护的——加入新驱动和 HAL 模块时需要手动添加。
- `stm32f1xx_hal_conf.h` 中每个模块需要两处修改：`#define HAL_xxx_MODULE_ENABLED` 和对应的 `#include "stm32f1xx_hal_xxx.h"`。

## 2. 开启 HAL 模块

在 `stm32f1xx_hal_conf.h` 中：

```c
/* 加入 #define */
#define HAL_DMA_MODULE_ENABLED      // 添加 DMA
#define HAL_UART_MODULE_ENABLED     // 添加 UART

/* 加入 #include */
#include "stm32f1xx_hal_dma.h"
#include "stm32f1xx_hal_uart.h"
```

## 3. 添加 HAL 源码文件到 CMakeLists.txt

在 `HAL_SOURCES` 列表中追加对应 `.c` 文件：

```cmake
set(HAL_SOURCES
    ...
    ${HAL_SRC}/stm32f1xx_hal_dma.c      # 新加
    ${HAL_SRC}/stm32f1xx_hal_uart.c     # 新加
)
```

## 4. 添加驱动源文件到 CMakeLists.txt

在 `APP_SOURCES` 列表中追加：

```cmake
set(APP_SOURCES
    ...
    ${CMAKE_SOURCE_DIR}/Core/hardware/App_Uart2.c   # 新加
)
```

## 5. 实现驱动头文件

提供：
- 顶层 Init 函数声明（公共接口）
- `extern` handle（如 `extern UART_HandleTypeDef huart2`）——如果其他文件需要引用
- `extern` 共享变量（如 `extern volatile uint8_t uart2_rx_byte`）

## 6. 实现驱动 .c 文件

遵循项目的子函数拆分风格：

```c
// 1. 外设 GPIO 初始化（时钟 + 引脚模式）
static void Uart2_GPIO_Init(void) { ... }

// 2. 外设专用模块初始化（DMA 等）
static void Uart2_DMA_Init(void) {
    // 注意 __HAL_LINKDMA 必须在 HAL_UART_Init 之前调用
    ...
}

// 3. NVIC 优先级 + 使能
static void Uart2_NVIC_Init(void) { ... }

// 4. 公共接口：依次调用子函数 + HAL 外设 Init + 启动工作模式
void Uart2_init(void) {
    Uart2_GPIO_Init();
    Uart2_DMA_Init();   // 先于 HAL_UART_Init()
    HAL_UART_Init(&huart2);
    Uart2_NVIC_Init();
    HAL_UART_Receive_IT(&huart2, &byte, 1);  // 启动接收
}

// 5. ISR（覆盖 startup weak 符号，风格与 App_Encoder 一致）
void USART2_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart2);
}

// 6. HAL 回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        // Echo + 重新使能接收
        HAL_UART_Transmit_IT(huart, &uart2_rx_byte, 1);
        HAL_UART_Receive_IT(huart, &uart2_rx_byte, 1);
    }
}
```

## 7. 在 main.c 中调用 Init

```c
int main(void) {
    HAL_Init();
    SystemClock_Config();
    ...
    Uart2_init();           // 新驱动
    ...
    while(1) {
        // main loop
    }
}
```

## 8. 验证编译

```bash
rm -rf build && mkdir build && cd build
cmake .. && make -j$(nproc)
```

检查是否有未定义的引用 → 通常意味着漏加了 `HAL_SOURCES` 中的 `.c` 文件。

### ⚠️ 常见陷阱：修改 CMakeLists.txt 后忘记重新 cmake

如果只是修改 `CMakeLists.txt`（如添加新 `.c` 文件到 `APP_SOURCES`）然后直接 `make`，**不会触发重新配置**，构建系统依然使用旧规则——新文件不会被编译和链接。

此时 linker 会报看似吓人的错误：

```
(.text.det_speed+0x2): undefined reference to `App_Speed_Get'
(.text.det_speed+0x2): dangerous relocation: unsupported relocation
(App_Speed_Get): Unknown destination type (ARM/Thumb) in ...main.c.o
```

**"dangerous relocation" + "Unknown destination type"** 是 GCC 15+ 工具链在遇到**未定义符号**时的表述方式——linker 无法判断目标函数的 ARM/Thumb 类型，因为符号根本不存在。初学者容易被"relocation"等术语误导去查 linker script 或编译选项，实际上问题很简单：**新加的 `.c` 文件没有被编译进项目**。

✅ **正确做法**：修改 `CMakeLists.txt` 后重新运行 cmake：

```bash
cd build
cmake ..            # 重新配置（生成新规则）
make -j$(nproc)     # 重新编译 + 链接
```

不需要每次 `rm -rf build`，除非你想彻底清理。`cmake ..` 会增量更新构建规则。

## 9. 从远程 Pull 后的编译失败诊断

當你從 GitHub 或其他遠端 pull 更新後，若出現以下典型編譯錯誤：

```
error: unknown type name 'TIM_HandleTypeDef'; did you mean 'DMA_HandleTypeDef'?
error: unknown type name 'UART_HandleTypeDef'
```

**最可能的原因**：遠端提交新增了外設源碼和 CMake 設定，卻**遺漏了更新本地的 `Core/stm32f1xx_hal_conf.h`**。

### 除錯步驟

```bash
# 1. 確認遠端提交改了哪些檔案
git log HEAD..origin/master --name-only

# 2. 根據新增的外設類型，檢查 hal_conf.h 是否有對應的 #define
grep HAL_TIM_MODULE_ENABLED Core/stm32f1xx_hal_conf.h   # TIM
grep HAL_SPI_MODULE_ENABLED Core/stm32f1xx_hal_conf.h   # SPI
grep HAL_I2C_MODULE_ENABLED Core/stm32f1xx_hal_conf.h   # I2C
grep HAL_ADC_MODULE_ENABLED Core/stm32f1xx_hal_conf.h   # ADC
```

### 修復

編輯 `Core/stm32f1xx_hal_conf.h`，需**同時**加入兩處：

1. 在 Module Selection 區域加入 `#define`：
```c
#define HAL_TIM_MODULE_ENABLED     // 新增（依外設類型調整）
```

2. 在 Module headers 區域加入對應的 `#include`：
```c
#include "stm32f1xx_hal_tim.h"    // 新增
```

### 為什麼會發生？

此專案的 `stm32f1xx_hal_conf.h` 是**手動管理**的精簡配置（只啟用用到的 HAL 模組），並非由 CubeMX 產生。遠端提交者可能在修改 `CMakeLists.txt` 時加入了 HAL 源碼，卻忘了同步更新 `hal_conf.h`。而 `stm32f1xx_hal.h` 使用條件式編譯（`#ifdef HAL_xxx_MODULE_ENABLED`），若無定義則不 include 對應的子頭檔，導致 `xxx_HandleTypeDef` 等型別無法被編譯器識別。

### 常見錯誤案例記錄

| 外設 | hal_conf.h 缺少的 define | 缺少的 include | 可能漏加的 HAL 源碼 |
|------|------------------------|----------------|---------------------|
| TIM (PWM/編碼器) | `HAL_TIM_MODULE_ENABLED` | `stm32f1xx_hal_tim.h` | `stm32f1xx_hal_tim.c` |
| TIM 主模式/TRGO | — | — | `stm32f1xx_hal_tim_ex.c`（`HAL_TIMEx_MasterConfigSynchronization`） |
| ADC | `HAL_ADC_MODULE_ENABLED` | `stm32f1xx_hal_adc.h` | `stm32f1xx_hal_adc.c` + `stm32f1xx_hal_adc_ex.c` |
| ADC 校準 | — | — | `stm32f1xx_hal_rcc_ex.c`（`HAL_RCCEx_GetPeriphCLKFreq`） |
| SPI | `HAL_SPI_MODULE_ENABLED` | `stm32f1xx_hal_spi.h` | `stm32f1xx_hal_spi.c` |
| I2C | `HAL_I2C_MODULE_ENABLED` | `stm32f1xx_hal_i2c.h` | `stm32f1xx_hal_i2c.c` |
| DMA | `HAL_DMA_MODULE_ENABLED` | `stm32f1xx_hal_dma.h` | `stm32f1xx_hal_dma.c` |

> 注意：STM32F1 HAL 的某些功能（如 TIM 主模式、RCC 輔助函數）放在 `_ex.c` 擴展文件中，而非主文件中。這些擴展函數**不會被 main HAL 源碼自動攜帶**，必須在 `CMakeLists.txt` 中**手動添加**。linker 出現 undefined reference 時，先在 HAL Src 目錄搜索對應的 `_ex.c` 文件。

## 注意事项

- **DMA 初始化顺序**：`__HAL_LINKDMA` 必须在 `HAL_UART_Init()` 之前调用，否则 UART Init 无法识别 DMA 通道。链接成功后，`HAL_UART_Init()` 会自动配置 CR3 暂存器的 DMAT/DMAR 位元，为日后 DMA 收发做好准备。
- **ISR 位置**：优先放在驱动 .c 文件中（匹配项目既有风格），除非项目惯例集中放在 `stm32f1xx_it.c`。驱动文件中的 ISR 定义会通过 linker 强符号覆盖 startup 的 weak 预设值——**用户不需要在别处额外声明或实现**。
- **全局变量**：回调中使用的 buffer（如 `uart2_rx_byte`）声明为 `volatile`，防止编译器优化。
- **回调重入**：Echo 场景在 `HAL_UART_RxCpltCallback` 中同时调用 Transmit + Receive_IT，两个操作都是中断驱动的，不会阻塞。
- **HAL 模块依赖**：如果发现缺少某个 HAL 模块，检查 `stm32f1xx_hal_conf.h` 中的定义和 `CMakeLists.txt` 中的源代码列表，修改两处即可。

### EXTI 中斷不觸發 — AFIO 時鐘的常見遺漏

使用 `HAL_EXTI_SetConfigLine()` 配置 EXTI 中斷時，**必須先啟用 AFIO 時鐘**，否則 `AFIO->EXTICR` 寫入無效：

```c
// ✅ 正確
__HAL_RCC_AFIO_CLK_ENABLE();     // ← 必須在 HAL_EXTI_SetConfigLine 之前
EXTI_ConfigTypeDef config = {0};
config.Line    = EXTI_LINE_14;
config.Mode    = EXTI_MODE_INTERRUPT;
config.Trigger = EXTI_TRIGGER_RISING_FALLING;
config.GPIOSel = EXTI_GPIOB;
HAL_EXTI_SetConfigLine(&hexti, &config);   // 內部寫 AFIO->EXTICR
```

**為什麼？** `HAL_EXTI_SetConfigLine()` 內部直接操作 `AFIO->EXTICR[]` 暫存器來將 EXTI 線路映射到指定 GPIO Port，但此操作**不會自動使能 AFIO 時鐘**。若 AFIO 時鐘未啟用，寫入無效 → EXTI 線路維持 reset 預設值（PAx）→ 你的引腳信號永遠進不了中斷控制器。

✅ 什麼時候**不需要** AFIO：
- USART2 在預設引腳（PA2/PA3）上
- 任何使用預設引腳（無需重映射）的外設

⚠️ 什麼時候**一定需要** AFIO：
- **所有 EXTI 配置**（`AFIO->EXTICR`）
- 外設重映射（如把 USART1 從 PA9/PA10 改成 PB6/PB7）
- 關閉 JTAG 釋放 PB3/PB4（`__HAL_AFIO_REMAP_SWJ_NOJTAG()`）

### 編碼器（Quadrature Encoder）方向邏輯陷阱

當使用 **雙邊沿中斷**（rising + falling）讀取正交編碼器 A/B 相時，方向判斷不能簡單地 `if (A == B)`：

```c
// ❌ 錯誤 — 雙邊沿下方向完全相反
if (A == B) { gen_counter++; } else { gen_counter--; }

// ✅ 正確 — 使用 XOR
if (A != B) { gen_counter++; } else { gen_counter--; }
```

**原因**：在 A 的上升沿和下降沿，A 與 B 的比較關係是互補的：
- 上升沿：A 領先 B → `A != B` → 正轉 ✓
- 下降沿：A 領先 B → `A == B` → 正轉…但 `A != B` 才正確

用 `A != B`（XOR）在兩個邊沿都能正確判斷方向。

### USART2 / STM32F1 特有细节

- **引脚复用**：`USART2_TX(PA2)` 用 `GPIO_MODE_AF_PP`，`USART2_RX(PA3)` 用 `GPIO_MODE_INPUT` + `GPIO_PULLUP`。
- **AFIO 时钟**：USART2 在默认引脚（PA2/PA3）上，**不需要**使能 AFIO 时钟。只有需要重映射外设到非默认引脚时才需要 AFIO。
- **时钟使能**：只需 `__HAL_RCC_USART2_CLK_ENABLE()` + `__HAL_RCC_GPIOA_CLK_ENABLE()`。

### ADC + TIM TRGO 觸發（STM32F1 特有）

#### 選擇哪個 Timer 做觸發源

STM32F1 的 ADC 規則組外部觸發選項有限，**不要想當然地認為所有 timer 的 TRGO 都能連到 ADC**。查詢參考手冊 RM0008 確認：

| EXTSEL | ADC1/ADC2 觸發源 | 適合我們的情境 |
|--------|-----------------|--------------|
| 000    | TIM1_CC1        | TIM1 PWM 正在用 |
| 001    | TIM1_CC2        | — |
| 010    | TIM1_CC3        | — |
| 011    | TIM2_CC2        | 需要 TIM2 |
| 100    | TIM3_TRGO       | 需要 TIM3 |
| 101    | TIM4_CC4        | TIM4 但只能用 CC4 非 TRGO |
| **110** | **EXTI line 11** | **STM32F103 中密度 = TIM1_TRGO** |
| 111    | SWSTART         | 軟體觸發 |

**結論**：若你已在用 TIM1 (PWM 等)，選 TIM1 做主模式 TRGO 最直接——EXTSEL=110 (`ADC_EXTERNALTRIGCONV_EXT_IT11`)。

#### STM32F1 `ADC_InitTypeDef` 與其他系列不同

STM32F1 的 `ADC_InitTypeDef` **非常精簡**，沒有以下在其他 STM32 系列常見的欄位：

```c
// ❌ 不存在於 STM32F1 HAL 的欄位
hadc1.Init.ClockPrescaler     = ...;  // ❌ ADC 時鐘分頻通過 RCC 寄存器配置
hadc1.Init.Resolution          = ...;  // ❌ STM32F1 固定 12-bit
hadc1.Init.ExternalTrigConvEdge = ...; // ❌ STM32F1 固定上升緣
hadc1.Init.DMAContinuousRequests = ...; // ❌ 不存在
hadc1.Init.EOCSelection        = ...;  // ❌ 不存在
```

**STM32F1 實際可用的欄位**：
```c
hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;          // 單通道
hadc1.Init.ContinuousConvMode    = DISABLE;                   // 由外部觸發
hadc1.Init.DiscontinuousConvMode = DISABLE;
hadc1.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_EXT_IT11;  // TIM1_TRGO
hadc1.Init.NbrOfConversion       = 1;
```

#### ADC 時鐘分頻 via RCC

因為 ADC 結構中沒有 `ClockPrescaler`，需通過 RCC 寄存器直接配置：

```c
__HAL_RCC_ADC_CONFIG(RCC_CFGR_ADCPRE_DIV6);   // PCLK2/6 = 12 MHz (≤14 MHz max)
```

RCC 分頻選項：`DIV2`、`DIV4`、`DIV6`、`DIV8`。

#### TIM 主模式 TRGO 配置

使用 `HAL_TIMEx_MasterConfigSynchronization`（在 `stm32f1xx_hal_tim_ex.c` 中）：

```c
TIM_MasterConfigTypeDef sMasterConfig = {0};
sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
sMasterConfig.MasterSlaveMode    = TIM_MASTERSLAVEMODE_DISABLE;
HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig);
```

可在 timer 運行中（已 Start）動態修改 TRGO，不需停止 timer。

#### 觸發模式下的中斷重武裝

在外部觸發模式 (`ContinuousConvMode = DISABLE`) 下，ADC 完成一次轉換後需要**在回調中重新呼叫 `HAL_ADC_Start_IT()`**，否則後續觸發不會產生中斷：

```c
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        pb0_voltage = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Start_IT(&hadc1);   // 重新武裝，等待下次觸發
    }
}
```

原因：HAL 狀態機在轉換完成後從 `HAL_ADC_STATE_BUSY_REG` 回到 `HAL_ADC_STATE_READY`，此時 EOCIE 雖仍為 1，但 HAL ISR 因 state 不對而不會執行回調。

#### 需要添加的 HAL 源碼文件

ADC + TIM TRGO 比基本 PWM 多了兩個依賴：

```cmake
set(HAL_SOURCES
    ...
    ${HAL_SRC}/stm32f1xx_hal_adc.c       # ADC 核心
    ${HAL_SRC}/stm32f1xx_hal_adc_ex.c    # ADC 擴展（校準等）
    ${HAL_SRC}/stm32f1xx_hal_tim_ex.c    # TIM 擴展（MasterConfigSynchronization）
    ${HAL_SRC}/stm32f1xx_hal_rcc_ex.c    # RCC 擴展（GetPeriphCLKFreq，被 ADC 校準依賴）
    ...
)
```

漏掉其中任何一個都會導致 link error：

```
undefined reference to 'HAL_TIMEx_MasterConfigSynchronization'  → 缺 stm32f1xx_hal_tim_ex.c
undefined reference to 'HAL_RCCEx_GetPeriphCLKFreq'             → 缺 stm32f1xx_hal_rcc_ex.c
```

#### ADC 啟動順序（規則組 Regular Group）

1. 配置 GPIO（PB0 = `GPIO_MODE_ANALOG`）
2. 配置 ADC 時鐘分頻（`__HAL_RCC_ADC_CONFIG`）
3. 配置 TIM 主模式 TRGO（可在 timer 運行後做）
4. `HAL_ADC_Init()` → ADC 基礎設定
5. `HAL_ADC_ConfigChannel()` → 通道 + 採樣時間
6. `HAL_ADCEx_Calibration_Start()` → 校準
7. NVIC 使能（`ADC1_2_IRQn`）
8. `HAL_ADC_Start_IT()` → 開始等待觸發

#### 注入組 (Injected Group) 配置

注入組與規則組是獨立的轉換序列，可**使用不同的外部觸發源**。STM32F1 的 JEXTSEL 映射與規則組 EXTSEL **不同**：

| JEXTSEL | ADC1 注入組觸發源 |
|---------|-----------------|
| 000     | TIM1_TRGO       |
| **101** | **TIM4_TRGO**   |

關鍵 API 差異：

| 功能 | 規則組 | 注入組 |
|------|--------|--------|
| 通道配置函數 | `HAL_ADC_ConfigChannel()` | `HAL_ADCEx_InjectedConfigChannel()` |
| 通道結構 | `ADC_ChannelConfTypeDef` | **`ADC_InjectionConfTypeDef`** |
| 啟動中斷模式 | `HAL_ADC_Start_IT()` | `HAL_ADCEx_InjectedStart_IT()` |
| 讀取結果 | `HAL_ADC_GetValue()` | `HAL_ADCEx_InjectedGetValue()` |
| 轉換完成回調 | `HAL_ADC_ConvCpltCallback` | **`HAL_ADCEx_InjectedConvCpltCallback`** |
| 外部觸發常數 | `ADC_EXTERNALTRIGCONV_EXT_IT11` | **`ADC_EXTERNALTRIGINJECCONV_T4_TRGO`** |

##### ⚠️ `ADC_InjectionConfTypeDef` 欄位名稱陷阱

STM32F1 HAL 的注入組配置結構體**所有欄位都帶 `Injected` 前綴**，新手易誤用無前綴的欄位名：

```c
// ❌ 錯誤 — 編譯器報 'no member named "Channel"'
ADC_InjectionConfTypeDef s = {0};
s.Channel = ADC_CHANNEL_8;      // 不存在！
s.Rank    = ADC_INJECTED_RANK_1; // 不存在！

// ✅ 正確 — 所有欄位帶 Injected 前綴
ADC_InjectionConfTypeDef s = {0};
s.InjectedChannel         = ADC_CHANNEL_8;
s.InjectedRank            = ADC_INJECTED_RANK_1;
s.InjectedNbrOfConversion = 1;
s.InjectedSamplingTime    = ADC_SAMPLETIME_55CYCLES_5;
s.ExternalTrigInjecConv   = ADC_EXTERNALTRIGINJECCONV_T4_TRGO;
s.AutoInjectedConv        = DISABLE;
s.InjectedDiscontinuousConvMode = DISABLE;
s.InjectedOffset          = 0;
HAL_ADCEx_InjectedConfigChannel(&hadc1, &s);
```

##### 觸發源選擇：TIM4_TRGO

TIM4_TRGO 是 STM32F1 中可觸發 ADC1 注入組的選項之一。若要使用 TIM4 的 TRGO：

1. 在 TIM4 初始化（含 `HAL_TIM_PWM_Start`）**之後**，增加主模式配置：

```c
TIM_MasterConfigTypeDef sMasterConfig = {0};
sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;  // 每次溢位送脈衝
sMasterConfig.MasterSlaveMode    = TIM_MASTERSLAVEMODE_DISABLE;
HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig);
```

2. ADC 初始化使用注入組 API，不需要改 TIM4 的時基參數（PSC/ARR）。

##### 注入組啟動順序

1. `App_Pwm_Init()` — 先初始化 TIM4（含 TRGO 配置）
2. 配置 GPIO（`GPIO_MODE_ANALOG`）
3. `HAL_ADC_Init()` — 只需基礎配置（規則組相關欄位不會影響注入組）
4. `HAL_ADCEx_InjectedConfigChannel()` — 注入組通道 + 觸發源
5. `HAL_ADCEx_Calibration_Start()` — 校準
6. NVIC 使能（`ADC1_2_IRQn`）
7. `HAL_ADCEx_InjectedStart_IT()` — 開始等待 TIM4 TRGO

##### 注入組 JEOC 中斷回調

```c
void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        pb0_voltage = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
        // 注入組外部觸發模式下，建議在回調中重新呼叫 Start_IT 確保中斷保持活躍
        HAL_ADCEx_InjectedStart_IT(&hadc1);
    }
}
```

## 9. （可选）printf 重定向到 UART

初始化 UART 后，可通过 `_write()` 将 `printf()` 输出导向该串口，实现调试打印。

### 步骤

#### a) main.c 中引入头文件

```c
#include <stdio.h>
#include <sys/stat.h>   // _write 的 file 参数类型
```

#### b) 先调用 UART2 初始化再使用 printf

```c
int main(void) {
    HAL_Init();
    ...
    Uart2_init();           // 必须在 printf 之前
    ...
    while(1) {
        printf("hello world\r\n");
        HAL_Delay(500);
    }
}
```

#### c) 实现 _write() 重定向

```c
int _write(int file, char *ptr, int len)
{
    (void)file;
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}
```

> ARM 工具链的 `nosys.specs` 已提供 `_write` 的 stub（返回 -1），所以不会 crash 但也不会有输出。用户实现的 `_write` 是**强符号**，会覆盖 stub 版本。

### 常见错误

- ❌ **`multiple definition of '_write'`** — `_write` 只能放在**一个** `.c` 文件中。如果同时在 `main.c` 和外设驱动文件（如 `App_Uart2.c`）中定义，链接器报重复定义。统一放在 `main.c` 即可。
- ❌ **`multiple definition of 'huart2'`** — header 中 `UART_HandleTypeDef huart2;` 是**定义**而非声明。被两个 `.c` 文件包含后会产生两个同名变量。修正：header 中写 `extern UART_HandleTypeDef huart2;`，定义只放在对应的 `.c` 文件里。
- ❌ **printf 无输出** — 检查 UART 是否已初始化、`_write()` 中的 handle 是否正确（名称、Instance）
