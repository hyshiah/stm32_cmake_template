#ifndef PID_H
#define PID_H
#include <stm32f1xx_hal.h>
// PID controller implementation

/* ════════════════════════════════════════════════════
 * ErrToV 转换常数（需根据实际电机和硬件校准）
 * PID 输出 co (rad/s) → 电压 → PWM 解析度
 * ════════════════════════════════════════════════════ */
/* 空載 11500 RPM @ 12V → K₀ = (11500×2π/60) / 12 ≈ 100.36 */
#define PID_K0            100.36f /* 直流增益 G(0): (rad/s)/V          */
#define PID_VBUS          19.0f   /* H 桥母线电压 (V)                   */
#define PID_PWM_RES       1200.0f /* TIM1_CH1 解析度 (ARR+1)           */
#define PID_MOTOR_VMAX    12.0f   /* 电机额定电压 (V)                   */
#define PID_DUTY_MAX      (PID_PWM_RES * PID_MOTOR_VMAX / PID_VBUS)  /* 1200×12/19≈758 */

typedef struct {
    float Kp;  // Proportional gain
    float Ki;  // Integral gain
    float Kd;  // Derivative gain
    float SP;  // Desired value
    float error_int_k_1;  // Integral term
    uint64_t t_k_1;
    float error_k_1;
    float cop;  // last P output (debug)
    float coi;  // last I output (debug)
    float cod;  // last D output (debug)
} PID_TypeDef;

void PID_Init(PID_TypeDef *pid, float Kp, float Ki, float Kd);
void PID_SetSP(PID_TypeDef *pid, float SP);
float PID_Compute(PID_TypeDef *pid, float FB);
float PID_ErrToV(PID_TypeDef *pid, float co);

#endif // PID_H