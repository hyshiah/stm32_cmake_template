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
    float error_int = pid->error_int_k_1 + (error - pid->error_k_1) * DeltaT * 0.5f;
    float error_dev = (error - pid->error_k_1) / DeltaT;

    float cop = pid->Kp * error; 
    float coi =  pid->Ki * error_int; 
    float cod = + pid->Kd * error_dev;
    pid->error_k_1 = error;
    pid->error_int_k_1 = error_int;
    pid->t_k_1 = t_k;

    float co = cop + coi + cod;
    return co;
}

