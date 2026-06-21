cmake_minimum_required(VERSION 3.10)

# ═══ Must be BEFORE project() ═══
# Bare-metal compiler cannot link without linker script + startup file.
# This tells CMake to only compile (not link) during compiler detection.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(stm32-blink C ASM)

# ── Export compile_commands.json for clangd / IDE ──
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Toolchain ──
set(CMAKE_C_COMPILER   arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

# ── Target CPU ──
set(MCU_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft")
set(CMAKE_C_FLAGS "${MCU_FLAGS} -DSTM32F103xB -Wall -Wextra -Wno-unused-parameter -Os -ffunction-sections -fdata-sections -g")
set(CMAKE_ASM_FLAGS "${MCU_FLAGS} -DSTM32F103xB -x assembler-with-cpp -g")

# ── Linker ──
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linkers/STM32F103XB_FLASH.ld")
#set(CMAKE_EXE_LINKER_FLAGS "${MCU_FLAGS} -T${LINKER_SCRIPT} -Wl,--gc-sections -Wl,-Map=${CMAKE_BINARY_DIR}/output.map -specs=nosys.specs -specs=nano.specs  -u _printf_float")
set(CMAKE_EXE_LINKER_FLAGS "${MCU_FLAGS} -T${LINKER_SCRIPT} -Wl,--gc-sections -Wl,-Map=${CMAKE_BINARY_DIR}/output.map -specs=nano.specs -specs=nosys.specs -u _printf_float")
# ── Paths to STM32CubeF1 ──
set(CUBE_DIR "/home/pi/stm32/STM32CubeF1")
set(HAL_INC   "${CUBE_DIR}/Drivers/STM32F1xx_HAL_Driver/Inc")
set(CMSIS_INC "${CUBE_DIR}/Drivers/CMSIS/Core/Include")
set(DEV_INC   "${CUBE_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Include")
set(HAL_SRC   "${CUBE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src")

# ── Include directories ──
include_directories(
    ${CMAKE_SOURCE_DIR}/Core
    ${CMAKE_SOURCE_DIR}/Core/hardware
    ${HAL_INC}
    ${CMSIS_INC}
    ${DEV_INC}
)

# ── Startup & System ──
set(STARTUP_SRC "${CMAKE_SOURCE_DIR}/startup/startup_stm32f103xb.s")
set(SYSTEM_SRC  "${CMAKE_SOURCE_DIR}/Core/system_stm32f1xx.c")

# ── Minimal HAL sources for blink ──
set(HAL_SOURCES
    ${HAL_SRC}/stm32f1xx_hal.c
    ${HAL_SRC}/stm32f1xx_hal_cortex.c
    ${HAL_SRC}/stm32f1xx_hal_gpio.c
    ${HAL_SRC}/stm32f1xx_hal_rcc.c
    ${HAL_SRC}/stm32f1xx_hal_flash.c
    ${HAL_SRC}/stm32f1xx_hal_exti.c
    ${HAL_SRC}/stm32f1xx_hal_dma.c
    ${HAL_SRC}/stm32f1xx_hal_adc.c
    ${HAL_SRC}/stm32f1xx_hal_adc_ex.c
    ${HAL_SRC}/stm32f1xx_hal_tim.c
    ${HAL_SRC}/stm32f1xx_hal_tim_ex.c
    ${HAL_SRC}/stm32f1xx_hal_rcc_ex.c
    ${HAL_SRC}/stm32f1xx_hal_uart.c
)

# ── Application sources ──
set(APP_SOURCES
    ${CMAKE_SOURCE_DIR}/Core/main.c
    ${CMAKE_SOURCE_DIR}/Core/stm32f1xx_it.c
    ${CMAKE_SOURCE_DIR}/Core/hardware/App_Encoder.c
    ${CMAKE_SOURCE_DIR}/Core/hardware/App_Uart2.c
    ${CMAKE_SOURCE_DIR}/Core/hardware/App_Adc.c
    ${CMAKE_SOURCE_DIR}/Core/hardware/App_Pwm.c
)

# ── Executable ──
add_executable(main.elf
    ${STARTUP_SRC}
    ${SYSTEM_SRC}
    ${HAL_SOURCES}
    ${APP_SOURCES}
)

# ── Post-build: generate .bin and .hex ──
add_custom_command(TARGET main.elf POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary main.elf main.bin
    COMMAND arm-none-eabi-objcopy -O ihex   main.elf main.hex
    COMMAND arm-none-eabi-size --format=berkeley main.elf
    COMMENT "Generating binary and hex files"
)

# ── Flash via OpenOCD (ST-Link) ──
find_program(OPENOCD openocd)
if(OPENOCD)
    add_custom_target(flash
        COMMAND ${OPENOCD} -f interface/stlink.cfg -f target/stm32f1x.cfg
                -c "program ${CMAKE_BINARY_DIR}/main.elf verify reset exit"
        DEPENDS main.elf
        COMMENT "Flashing main.elf via OpenOCD (ST-Link)..."
    )
    add_custom_target(flash-bin
        COMMAND ${OPENOCD} -f interface/stlink.cfg -f target/stm32f1x.cfg
                -c "program ${CMAKE_BINARY_DIR}/main.bin 0x08000000 verify reset exit"
        DEPENDS main.bin
        COMMENT "Flashing main.bin via OpenOCD (ST-Link)..."
    )
    add_custom_target(openocd
        COMMAND ${OPENOCD} -f interface/stlink.cfg -f target/stm32f1x.cfg
        COMMENT "Starting OpenOCD server (connect GDB with: arm-none-eabi-gdb -ex 'target extended-remote :3333' main.elf)"
    )
else()
    add_custom_target(flash
        COMMAND ${CMAKE_COMMAND} -E echo "openocd not found. Install it or flash manually: openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c 'program build/main.elf verify reset exit'"
    )
endif()
