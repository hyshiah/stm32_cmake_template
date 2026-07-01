#ifndef PID_H
#define PID_H
#include <stm32f1xx_hal.h>
// PID controller implementation

typedef struct {
    float Kp;  // Proportional gain
    float Ki;  // Integral gain
    float Kd;  // Derivative gain
    float SP;  // Desired value
    float error_int_k_1;  // Integral term
    uint64_t t_k_1;
    float error_k_1;
} PID_TypeDef;

void PID_Init(PID_TypeDef *pid, float Kp, float Ki, float Kd);
void PID_SetSP(PID_TypeDef *pid, float SP);
float PID_Compute(PID_TypeDef *pid, float FB);

#endif // PID_H