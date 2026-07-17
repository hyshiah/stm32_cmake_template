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

debug_index_t calculate_index(void){
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
    debug_index_t index;
    index.last_index = last_index;
    index.second_last_index = second_last_index;
    index.last_tick = ring_buffer[index.last_index];
    index.second_last_tick = ring_buffer[index.second_last_index];
    return index;
}

/**
 * @brief 主循环调用：获取时间差
 * @return 当前时间 - 缓冲区最后一个有效记录 tick
 *         如果缓冲区未准备好，返回 0xFFFFFFFF
 * 
 * @note 这里需要计算"最后一条"记录，实际上是当前写入索引的前一个位置
 *       因为 set() 每次写入后索引都会指向下一个位置
 */
uint32_t App_Det_Ticks(void) {
    // 检查缓冲区是否已准备好
    if (!buffer_ready) {
        return 0xFFFFFFFF;  // 无效值
    }
    
    // 获取当前 TIM2 計數值（10 µs/tick）
    //uint32_t current_tick = App_Tim2_GetTick();
    
    // 计算"最后一条有效记录"的索引
    // 因为 write_index 指向的是下一个要写入的位置，
    // 所以最后一条记录在 write_index - 1（循环考虑）
    debug_index_t index = calculate_index();   
    // 读取最后一条记录的值
    uint32_t last_tick = index.last_tick;
    uint32_t second_last_tick = index.second_last_tick;
    // 计算时间差（处理 Systick 溢出回绕的情况）
    // 无符号减法在 C 语言中定义良好，即使回绕也能正确计算差值
    uint32_t interval = 0;
    uint32_t temp = 0;
    if(last_tick >= second_last_tick){
        interval = last_tick - second_last_tick;
    }else{
        temp = (UINT32_MAX -second_last_tick);
        interval = last_tick + temp;
    }   
    return interval;
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


float App_Tick_to_speed(void) {
    time_diff = App_Det_Ticks();
    if (time_diff == 0xFFFFFFFF) {
        return -1.0f;  // 缓冲区未准备好
    } else if (time_diff == 0) {
        return -2.0f;  // 理论上不应该为 0，除非在同一个 Tick 内触发了两次中断
    } else {
        // TIM2 tick = 10 µs → 100000 ticks/s
        // freq (Hz) = 100000 / (time_diff × 26)
        float ticks_rev = time_diff * ENCODER_PULSE_PER_REV;        // ticks per revolution
        float freq = 100000.0f / ticks_rev;                         // rev/s (Hz)
        return freq * 3.14159f * 2.0f;                              // rad/s                        
    } 
}

