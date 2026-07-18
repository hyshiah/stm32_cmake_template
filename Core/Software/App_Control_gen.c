#include "App_Control_gen.h"
#include "App_det_speed.h"

static PID_TypeDef pid_speed;


void App_Control_Gen_Init(void) {
    // Initialize the PID controller with desired gains
    PID_Init(&pid_speed, 1.0f, 0.01f, 0.0f); // Kp=1, Ki=0, Kd=0 → P-only 控制器
    PID_SetSP(&pid_speed, 2.0f * 3.1415f * 160.0f); // Set desired speed (SP)
    
    
    
    Gen_Encoder_Init();   // 初始化 AB 相編碼器
    Motor_Encoder_init(); // 初始化 Motor 編碼器
    App_Pwm_Init();           // 初始化 PWM（PA8=TIM1_CH1, PB6=TIM4_CH1, 1kHz）
    App_Adc_Init();           // 初始化 ADC1（PB0, TIM1 TRGO 觸發, EOC 中斷）
    App_Speed_Init();        
}

debug_control_gen_t App_Control_Gen_Proc(void) {
    debug_control_gen_t debug_data;
    // Get the current speed (feedback) from the encoder
    float omega = App_Tick_to_speed(); // Replace with actual speed calculation

    // Compute the control output using the PID controller
    float co = PID_Compute(&pid_speed, omega);

    // Convert PID output (rad/s) → PWM 解析度
    uint16_t duty_cycle = (uint16_t)PID_ErrToV(&pid_speed, co);
    debug_data.speed_omega = omega;
    debug_data.control_output = co;
    debug_data.kp_output = pid_speed.cop;
    debug_data.ki_output = pid_speed.coi;
    debug_data.kd_output = pid_speed.cod;
    debug_data.duty_cycle = duty_cycle;
    App_Pwm1_SetDuty(duty_cycle); // Set PWM duty cycle for generator control
    return debug_data;
}