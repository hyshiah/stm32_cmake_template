# STM32F103C8T6 CMake 模板專案(for Raspberry) — 編碼器驅動

## 概述

以 **CMake** 建置的 STM32F103C8T6 (Blue Pill) 裸機專案，使用 STM32CubeF1 HAL 庫。展示雙正交編碼器（Generator / Motor）的中斷驅動實作，包含 LED 閃爍作為基礎驗證。

## 硬體平台

| 項目 | 內容 |
|---|---|
| MCU | STM32F103C8T6 (Cortex-M3, 72 MHz) |
| 開發板 | Blue Pill |
| 外部晶振 | 8 MHz (HSE) → PLL ×9 → SYSCLK 72 MHz |
| LED | PC13（開發板內建） |
| 除錯 | SWD（PA13 SWDIO, PA14 SWCLK） |
| 燒錄器 | ST-Link (OpenOCD) |
| 工具鏈 | `arm-none-eabi-gcc` |

## 腳位配置

| 功能 | 腳位 | 說明 |
|---|---|---|
| Gen_Encoder A 相 | PB14 | EXTI14 中斷，雙邊沿觸發 |
| Gen_Encoder B 相 | PB15 | 同步讀取判斷方向，無中斷 |
| Motor_Encoder A 相 | PB3 | EXTI3 中斷，雙邊沿觸發（需關閉 JTAG） |
| Motor_Encoder B 相 | PB4 | 同步讀取判斷方向，無中斷 |
| LED | PC13 | 輸出推挽，500 ms 翻轉 |

> PB3/PB4 預設為 JTAG 功能（JTDO / JTRST），初始化時透過 `__HAL_AFIO_REMAP_SWJ_NOJTAG()` 釋放為 GPIO。

## 專案結構

```
.
├── CMakeLists.txt              # 主建置腳本
├── Core/
│   ├── main.c                  # 主程式：時鐘、GPIO、編碼器初始化
│   ├── stm32f1xx_it.c          # 系統中斷 (SysTick)
│   ├── stm32f1xx_hal_conf.h    # HAL 模組配置
│   ├── system_stm32f1xx.c      # 系統初始化
│   └── hardware/
│       ├── App_Encoder.h        # 編碼器驅動標頭
│       └── App_Encoder.c        # 編碼器實作 + EXTI ISR
├── startup/
│   └── startup_stm32f103xb.s   # 啟動向量表
├── linkers/
│   └── STM32F103XB_FLASH.ld    # 鏈結腳本 (64 KB Flash)
├── build/                      # 建置輸出
└── README.md
```

## 軟體架構

### 編碼器驅動 (`App_Encoder.c`)

兩個編碼器共用相同的正交解碼模式：

- **僅 A 相觸發中斷**：減少中斷負擔
- **B 相同步取樣**：ISR 中同時讀取 A、B 相位，比較後判斷旋轉方向

判斷邏輯：

| A 相（中斷觸發後） | B 相 | 方向 |
|---|---|---|
| High | High | 順時針 (+) |
| Low | Low | 順時針 (+) |
| High | Low | 逆時針 (-) |
| Low | High | 逆時針 (-) |

方向計數值暫存在 `main.c` 定義的全域變數 `gen_counter` / `motor_counter` 中。

### 中斷向量

| IRQ | Handler | 用途 |
|---|---|---|
| EXTI3_IRQn | `EXTI3_IRQHandler` | Motor_Encoder PB3 中斷 |
| EXTI15_10_IRQn | `EXTI15_10_IRQHandler` | Gen_Encoder PB14 中斷 |

ISR 實作在 `App_Encoder.c` 中，透過 linker 強定義覆蓋 startup 的 weak 預設值。

## 建置需求

### 工具鏈

```bash
# ARM GNU Toolchain (arm-none-eabi-gcc)
# 下載：https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads

# 確認工具鏈可用
arm-none-eabi-gcc --version
```

### STM32CubeF1 HAL

HAL 庫路徑在 `CMakeLists.txt` 中設定：

```cmake
set(CUBE_DIR "/home/rock/stm32/STM32CubeF1")
```

請根據實際安裝位置修改。

### OpenOCD（可選，用於燒錄）

```bash
# Ubuntu / Debian
sudo apt install openocd

# 確認可用
openocd --version
```

## 建置與燒錄

```bash
# 進入專案目錄
cd /home/rock/stm32/stm32_cmake_template

# 配置 CMake
cmake -S . -B build

# 編譯
cmake --build build

# 燒錄（需 OpenOCD + ST-Link）
cmake --build build --target flash
```

### 可用 make target

| Target | 說明 |
|---|---|
| `main.elf` | 編譯連結生成 ELF |
| `flash` | 透過 OpenOCD 燒錄 ELF |
| `flash-bin` | 透過 OpenOCD 燒錄 BIN |
| `openocd` | 啟動 OpenOCD GDB server |

## 開發備註

### 已啟用的 HAL 模組

`Core/stm32f1xx_hal_conf.h` 中僅啟用所需模組以節省 Flash：

- HAL (核心)
- RCC（時鐘）
- GPIO
- CORTEX (NVIC/SysTick)
- FLASH
- EXTI

### 鏈結腳本

`linkers/STM32F103XB_FLASH.ld` 對應 64 KB Flash / 20 KB SRAM。若使用其他型號請更換對應的 `.ld` 檔案。

### 除錯

每次建置會輸出 `compile_commands.json`，可供 clangd 或 IDE 使用。

```bash
# 手動啟動 OpenOCD GDB server
cmake --build build --target openocd

# 另開終端，用 GDB 連線
arm-none-eabi-gdb -ex "target extended-remote :3333" build/main.elf
```
