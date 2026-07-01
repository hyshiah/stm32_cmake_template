#include "App_det_speed.h"
#include "App_tim2_freeCount.h"

static volatile uint32_t time_diff = 0;

// 环形缓冲区：存储 Tick 值
static uint32_t ring_buffer[SPEED_RING_BUFFER_SIZE];

// 写入索引（指向下一个要写入的位置）
static uint8_t write_index = 0;

// 记录已写入的数量（用于判断缓冲区是否已初始化完成）
static uint8_t write_count = 0;

// 标志：缓冲区是否已经至少填满一次（达到3个有效记录）
static bool buffer_ready = false;

// 互斥保护标志（用于防止主循环在读取时被中断打断）
// 由于 get() 只是读取，set() 只是写入不同位置，
// 且写入索引是原子的（单字节操作），在 Cortex-M3 上是安全的
// 但为了严谨，用 volatile 确保编译器不优化
static volatile bool is_updating = false;

// 为了确保中断不会被打断，set() 中不做复杂操作

/**
 * @brief 初始化速度检测模块，為一環型緩衝區，紀錄最近三次的 Tim2 值
 * 用來計算兩次中斷之間的時間差，單位為 Tick（10 µs/tick）
 */
void App_Speed_Init(void) {
    App_Tim2_FreeCount_Init(); // 初始化 TIM2 自由计数器（10 µs/tick，32-bit free-running）
    // 清空缓冲区
    for (uint8_t i = 0; i < SPEED_RING_BUFFER_SIZE; i++) {
        ring_buffer[i] = 0;
    }
    write_index = 0;
    write_count = 0;
    buffer_ready = false;
    is_updating = false;
}

/**
 * @brief 外部中断调用：记录当前 Tick
 * @note  此函数在中断上下文中执行，必须快速完成
 * @note  不需要禁用中断，因为操作是原子的
 */
void App_Speed_Set(void) {
    // 读取 TIM2 計數值（10 µs/tick）
    uint32_t current_tick = App_Tim2_GetTick();

    // 写入缓冲区
    ring_buffer[write_index] = current_tick;
    
    // 更新写入索引（循环）
    write_index++;
    if (write_index >= SPEED_RING_BUFFER_SIZE) {
        write_index = 0;
    }
    
    // 更新写入计数（最多到缓冲区大小）
    if (write_count < SPEED_RING_BUFFER_SIZE) {
        write_count++;
    }
    
    // 当写入次数达到缓冲区大小时，标记为已准备好
    if (write_count >= SPEED_RING_BUFFER_SIZE) {
        buffer_ready = true;
    }
}

/**
 * @brief 主循环调用：获取时间差
 * @return 当前时间 - 缓冲区最后一个有效记录
 *         如果缓冲区未准备好，返回 0xFFFFFFFF
 * 
 * @note 这里需要计算"最后一条"记录，实际上是当前写入索引的前一个位置
 *       因为 set() 每次写入后索引都会指向下一个位置
 */
uint32_t App_Speed_Get(void) {
    // 检查缓冲区是否已准备好
    if (!buffer_ready) {
        return 0xFFFFFFFF;  // 无效值
    }
    
    // 获取当前 TIM2 計數值（10 µs/tick）
    uint32_t current_tick = App_Tim2_GetTick();
    
    // 计算"最后一条有效记录"的索引
    // 因为 write_index 指向的是下一个要写入的位置，
    // 所以最后一条记录在 write_index - 1（循环考虑）
    uint8_t last_index;
    if (write_index == 0) {
        last_index = SPEED_RING_BUFFER_SIZE - 1;
    } else {
        last_index = write_index - 1;
    }
    uint8_t second_last_index;
    if (last_index == 0) {
        second_last_index = SPEED_RING_BUFFER_SIZE - 1;
    } else {
        second_last_index = last_index - 1;
    }
    
    // 读取最后一条记录的值
    uint32_t last_tick = ring_buffer[last_index];
    uint32_t second_last_tick = ring_buffer[second_last_index];
    // 计算时间差（处理 Systick 溢出回绕的情况）
    // 无符号减法在 C 语言中定义良好，即使回绕也能正确计算差值
    
    uint32_t time_diff = current_tick - last_tick;
    uint32_t interval = last_tick - second_last_tick;
    if (time_diff > interval) {
        return time_diff; // 返回时间差
    } else {
        return interval; // 返回两条记录之间的间隔
        // 这里选择返回两条记录之间的间隔，表示可能有中断
        // 如果当前时间与最后一条记录的差值大于两条记录之间的间隔，
        // 说明可能有中断丢失或其他异常情况
        // 可以选择返回一个特殊值，或者继续返回 time_diff
        // 这里选择继续返回 time_diff
    }
}

/**
 * @brief 获取当前写入索引（调试用）
 */
uint8_t App_Speed_GetIndex(void) {
    return write_index;
}

/**
 * @brief 检查缓冲区是否已准备好（调试用）
 */
bool App_Speed_IsBufferReady(void) {
    return buffer_ready;
}

void App_Print_Det_Speed(void) {
    char buffer[80];
    // 获取当前速度对应的时间差
    time_diff = App_Speed_Get();
    if (time_diff == 0xFFFFFFFF) {
            // 缓冲区未准备好
        printf("Speed module not ready yet!\r\n");
    } else if (time_diff == 0) {
        // 理论上不应该为 0，除非在同一个 Tick 内触发了两次中断
        printf("Time diff is 0! Too fast!\r\n");
    } else {
        // 計算頻率：100000 * 10µs / time_diff = Hz
        // 例如：time_diff = 10000（= 100ms），則頻率 = 10Hz
        float freq = 100000.0f / time_diff;               // 原始頻率 (Hz)
        float freq_corrected = freq / ENCODER_PULSE_PER_REV;  // 实际物理频率
        float rpm = freq_corrected * 60.0f;               // 转速 (RPM)
        // 格式化成字符串
        // 手动转换浮点数
        char freq_str[16];
        float_to_string(freq_corrected, 2, freq_str);  // 保留2位小数
        char rpm_str[16];
        float_to_string(rpm, 1, rpm_str);   // 保留1位小数
        snprintf(buffer, sizeof(buffer),
                 "dt=%" PRId32 " *10us  freq=%s Hz  RPM=%s\r\n",
                 time_diff, freq_str, rpm_str);
        // 发送到串口
        printf("%s", buffer);                 
    } 
}

float App_Det_Speed(void) {
    time_diff = App_Speed_Get();
    if (time_diff == 0xFFFFFFFF) {
        return -1.0f;  // 缓冲区未准备好
    } else if (time_diff == 0) {
        return -2.0f;  // 理论上不应该为 0，除非在同一个 Tick 内触发了两次中断
    } else {
        // 計算頻率：100000 * 10µs / time_diff = Hz
        // 例如：time_diff = 10000（= 100ms），則頻率 = 10Hz
        float freq = 100000.0f / time_diff;               // 原始頻率 (Hz)
        float freq_corrected = freq / ENCODER_PULSE_PER_REV;  // 实际物理频率
        return freq_corrected * 3.1415 * 2.0f ;  // 返回实际物理频率 (Hz)                        
    } 
}
