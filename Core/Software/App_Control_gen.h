#ifndef APP_CONTROL_GEN_H
#define APP_CONTROL_GEN_H
#include <stm32f1xx_hal.h>
#include "pid.h"
#include "App_Encoder.h"
#include "App_Uart2.h"
#include "App_Pwm.h"
#include "App_Adc.h"
#include "App_det_speed.h" // 包含速度检测模块的头文件

typedef struct {
    float speed_omega;
    float control_output; // PID controller for speed
    float kp_output;
    float ki_output;
    float kd_output;
} debug_control_gen_t;

void App_Control_Gen_Init(void);

debug_control_gen_t App_Control_Gen_Proc(void);


#endif // APP_CONTROL_GEN_H