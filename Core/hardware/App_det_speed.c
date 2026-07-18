#include "App_det_speed.h"
#include "App_tim2_freeCount.h"

/* ══════════════════════════════════════════════════════
 * 方案 A：整圈平均測速
 * 每個編碼器邊沿只計數，每滿 26 次（一圈）才記錄一次總時間
 * 自動平均掉 13 線分布不均的影響
 * ══════════════════════════════════════════════════════ */

#define MAX_REV_TICKS   200000  /* 最大容許一圈 tick 數（= 2 秒） */
                                 /* 低於此視為靜止，避免 TIM2 溢位或 */
                                 /* 電機堵轉造成 rev_time 暴增       */

static volatile uint8_t edge_count = 0;        // 當前圈已計邊沿數
static volatile uint32_t rev_time = 0;         // 上一圈總時間（tick）
static volatile bool rev_ready = false;        // 是否已完成至少一圈

void App_Speed_Init(void) {
    App_Tim2_FreeCount_Init();
    edge_count = 0;
    rev_time = 0;
    rev_ready = false;
}

void App_Speed_Set(void) {
    static uint32_t last_tick = 0;
    uint32_t current_tick = App_Tim2_GetTick();

    /* 軟體防抖：間隔 < 20 tick (200 µs) 視為雜訊跳過
     * 11500 RPM 時最小沿間隔 ≈ 200 µs (20 tick) */
    if (current_tick - last_tick < 20) {
        return;
    }
    last_tick = current_tick;

    edge_count++;

    if (edge_count >= (uint8_t)ENCODER_PULSE_PER_REV) {  // 26 次 = 一圈
        rev_time = current_tick;   // TIM2 從 0 開始，直接是一圈 tick 數
        rev_ready = true;
        edge_count = 0;
        App_Tim2_Reset();          // 重置 TIM2，下一圈從 0 開始
    }
}

uint32_t App_Det_Ticks(void) {
    if (!rev_ready) return 0xFFFFFFFF;
    return rev_time;  // 一圈的總 tick 數（10 µs/tick）
}

float App_Tick_to_speed(void) {
    uint32_t ticks = App_Det_Ticks();
    if (ticks == 0xFFFFFFFF) {
        return -1.0f;
    }
    if (ticks == 0) {
        return -2.0f;
    }
    /* 超過 2 秒沒完成一圈 → 電機已停或堵轉 */
    if (ticks > MAX_REV_TICKS) {
        return 0.0f;
    }
    // ticks 是一圈總時間（10 µs/tick）
    // 一圈時間（秒）= ticks × 10⁻⁵
    // 頻率（rev/s）= 1 / (ticks × 10⁻⁵) = 100000 / ticks
    float freq = 100000.0f / (float)ticks;  // rev/s (Hz)
    return freq * 3.14159f * 2.0f;           // rad/s
}
