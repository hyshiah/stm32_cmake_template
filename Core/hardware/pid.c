#include "pid.h"


void PID_Init(PID_TypeDef *pid, float Kp, float Ki, float Kd) {
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;    
    pid->SP = 0.0f;
    pid->error_int_k_1 = 0.0f;
    pid->error_k_1 = 0.0f;
    pid->t_k_1 = 0;
}

void PID_SetSP(PID_TypeDef *pid, float SP) {
    pid->SP = SP;
}

float PID_Compute(PID_TypeDef *pid, float FB) {
    float error = pid->SP - FB;
    uint64_t t_k = HAL_GetTick();
    float DeltaT = (float)(t_k - pid->t_k_1) / 1000.0f; // Convert milliseconds to seconds
    float error_int = pid->error_int_k_1 + (error + pid->error_k_1) * DeltaT * 0.5f;
    float error_dev = (error - pid->error_k_1) / DeltaT;

    float cop = pid->Kp * error; 
    float coi =  pid->Ki * error_int; 
    float cod = + pid->Kd * error_dev;
    pid->error_k_1 = error;
    pid->error_int_k_1 = error_int;
    pid->t_k_1 = t_k;
    pid->cop = cop;
    pid->coi = coi;
    pid->cod = cod;

    float co = cop + coi + cod;
    return co;
}

float PID_ErrToV(PID_TypeDef *pid, float co) {
    (void)pid;  /* 保留接口，后续可用于 Ki/Kd 补偿 */
    /* co (rad/s) → 所需电压 → 解析度 11500 RPM = 191 rev/s = 1204.3 rad/s*/
    // PID_KO 12（伏特）/1204=0.00996 倒數 100.36
    float v_need = co / PID_K0;                         /* rad/s → V */
    float duty_f = v_need / PID_VBUS * PID_PWM_RES;     /* V → duty */

    if (duty_f < 400.0f)         duty_f = 400.0f;
    if (duty_f > PID_DUTY_MAX)   duty_f = PID_DUTY_MAX;
    /* test assume co need 120 rev/sec = 753 rad/s
     * need 7.5V equip 473 dutycycle
    */ 
    return duty_f;
}

