// ============================================================
// 文件名: pid.c
// 功能说明: PID控制器实现模块
// 包含位置式循迹PID（带非线性P2项）、增量式速度PID（左右轮独立）和位置式速度PID（左右轮独立），
// 用于智能车电磁循迹系统的方向控制和速度闭环控制。
// ============================================================

#include "config.h"

float KP_x = 0.0f;              // 方向环比例系数
float K2P_x = 0.0f;             // 方向环非线性二次比例系数
float KI_x = 0.0f;              // 方向环积分系数（预留）
float KD_x = 0.0f;              // 方向环微分系数
float K2D_x = 0.0f;              // 方向环微分系数

float KP_a = 0.0f;              // 角度环比例系数
float KD_a = 0.0f;              // 角度环微分系数
float KG_a = 0.0f;              // 角度环陀螺仪阻尼系数

float KP_v = 0.0f;              // 速度环比例系数
float KI_v = 0.0f;              // 速度环积分系数
float KD_v = 0.0f;              // 速度环微分系数

float angle_err = 0.0;          // 角度环当前误差
float angle_out = 0.0;          // 角度环输出
float PID_outL = 0.0f;          // 左轮速度PID总输出
float PID_outR = 0.0f;          // 右轮速度PID总输出


// 函数名: PID_track
// 功能: 位置式循迹PID控制器
// 参数: track_error - 循迹偏差输入（由adc.c计算得到）
// 返回值: 方向控制输出，范围[-MAX_DIR_OUT, MAX_DIR_OUT]（±200）
// 说明: 采用改进型位置式PID，增加了非线性P2项（K2P_x * error * |error|），
//       在小偏差时响应柔和，大偏差时快速修正。
int16 PID_track(void)
{   
	int32 error = 0.0;
	
    float P_out = 0.0;       // P环节输出
    float P2_out = 0.0;      // P2环节输出（非线性项）
    float D_out = 0.0;       // D环节输出
	float D2_out = 0.0; 
    
	static int32 error_last = 0.0;   // 上次偏差
    
    int16 PID_out = 0.0;     // PID总输出
    
    error = track_error;            // 更新当前偏差
//  error_sum += error;
    
    P_out  = KP_x  * (float) error;                       // 计算P环节输出
    P2_out = K2P_x * (float) error * abs(error);          // 计算非线性P2项
    D_out  = KD_x  * (float)(error - error_last);         // 计算D环节输出（微分项）
	D2_out = K2D_x * accel_x;
    
    error_last = error;             // 更新上次偏差，供下次微分计算使用
    
    PID_out = (P_out + P2_out/*I_out*/ + D_out - D2_out);        // 汇总PID各环节输出
    
    if(PID_out >  MAX_DIR_OUT) PID_out =  MAX_DIR_OUT;      // 输出限幅上限
    if(PID_out < -MAX_DIR_OUT) PID_out = -MAX_DIR_OUT;      // 输出限幅下限
    
    return PID_out;                 // 返回PID控制输出
}


// 函数名: PID_angle
// 功能: 角度环PD控制器（用于环岛等需要固定角度转向的场景）
// 参数: target_angle - 目标角度（单位：度）
// 说明: 根据目标角度与当前角度的偏差计算方向输出，并叠加陀螺仪阻尼项。
//       输出通过 speed_control() 分配左右轮速度，实现按角度转向。
void PID_angle(int16 target_angle)
{
	static float angle_err_last = 0.0;
	
	angle_err = target_angle - angle_x;  // 角度误差 = 目标角度 - 当前角度

	angle_out = KP_a * angle_err + KD_a * (angle_err - angle_err_last) - KG_a * gyro_x;  // 角度PD控制：角度误差比例+微分-陀螺仪阻尼

	/*		40 * 0.5	+		0		-	7 *		gyro_yy()		*/
	angle_err_last = angle_err;
	
	speed_control( -angle_out );
}



// 函数名: PID_L
// 功能: 左轮增量式速度PID控制器
// 参数: target_speed_L - 左轮目标速度
//       real_speed_L   - 左轮实际速度（编码器反馈）
// 返回值: 左轮电机PWM控制增量输出，范围[-MAX_SPD_OUT, MAX_SPD_OUT]（±7000）
// 说明: 采用增量式PID算法，输出为PWM增量，抗积分饱和。
//       微分项使用二阶差分：error - 2*Last_error + Previous_error
int16 PID_L(void)
{
    static float P_outL = 0.0,      // P环节输出
                 I_outL = 0.0,      // I环节输出
                 D_outL = 0.0,      // D环节输出
				 PID_sumL = 0.0;      // PID累计输出
    
    static int32 errorL = 0,        // 当前偏差
                 Last_errorL = 0,   // 上次偏差
                 Previous_errorL = 0;   // 上上次偏差
    
    // 串口打印目标速度和实际速度
//    sprintf(uart, "%d,%d\r\n", target_speed_L, real_speed_L);
//    uart_write_buffer(UART_1, uart, strlen(uart));
    
    errorL = target_speed_L - real_speed_L;     // 计算速度偏差 = 目标速度 - 实际速度
    
    P_outL = KP_v * (float)(errorL - Last_errorL);     // P环节：比例控制（基于偏差变化率）
    I_outL = KI_v * (float) errorL;                    // I环节：积分控制（累积偏差）
//    D_outL = KD_v * (float)(errorL - 2 * Last_errorL + Previous_errorL); // D环节：微分控制
    
    PID_sumL += (P_outL + I_outL + D_outL);     // PID总输出累加
    
    Previous_errorL = Last_errorL;              // 更新上上次偏差
    Last_errorL = errorL;                       // 更新上次偏差
    
    if(PID_sumL >  MAX_SPD_OUT) PID_sumL =  MAX_SPD_OUT;      // 输出限幅上限
    if(PID_sumL < -MAX_SPD_OUT) PID_sumL = -MAX_SPD_OUT;      // 输出限幅下限
    
    return (int32)PID_sumL;                            // 返回左轮电机PWM控制值
}

// 函数名: PID_R
// 功能: 右轮增量式速度PID控制器
// 参数: target_speed_R - 右轮目标速度
//       real_speed_R   - 右轮实际速度（编码器反馈）
// 返回值: 右轮电机PWM控制增量输出，范围[-MAX_SPD_OUT, MAX_SPD_OUT]（±7000）
// 说明: 与PID_L对称，实现右轮独立速度闭环控制。
int16 PID_R(void)
{
    static float P_outR = 0.0,      // P环节输出
                 I_outR = 0.0,      // I环节输出
                 D_outR = 0.0,      // D环节输出
				 PID_sumR = 0.0;      // PID累计输出
    
    static int32 errorR = 0,        // 当前偏差
                 Last_errorR = 0,   // 上次偏差
                 Previous_errorR = 0;   // 上上次偏差
    
    errorR = target_speed_R - real_speed_R;     // 计算速度偏差 = 目标速度 - 实际速度
                
    P_outR = KP_v * (float)(errorR - Last_errorR);     // P环节：比例控制
    I_outR = KI_v * (float) errorR;                    // I环节：积分控制
//    D_outR = KD_v * (float)(errorR - 2 * Last_errorR + Previous_errorR); // D环节：微分控制
                
    PID_sumR += (P_outR + I_outR + D_outR);     // PID总输出累加
    
    Previous_errorR = Last_errorR;              // 更新上上次偏差
    Last_errorR = errorR;                       // 更新上次偏差
    
    if(PID_sumR >  MAX_SPD_OUT) PID_sumR =  MAX_SPD_OUT;      // 输出限幅上限
    if(PID_sumR < -MAX_SPD_OUT) PID_sumR = -MAX_SPD_OUT;      // 输出限幅下限
    
    return (int32)PID_sumR;                            // 返回右轮电机PWM控制值
}

// 函数名: PID_L_pos
// 功能: 左轮位置式速度PID控制器（含快速制动与死区前馈）
// 参数: target_speed_L - 左轮目标速度
//       real_speed_L   - 左轮实际速度（编码器反馈）
// 返回值: 左轮电机PWM控制输出，范围[-MAX_SPD_OUT, MAX_SPD_OUT]
// 说明: 采用位置式PID算法，直接输出PWM控制量。
//       当目标速度为0且实际速度较大时，输出反向制动力矩快速停车。
//       内置死区前馈，target=0时不加偏置，确保能真正停车。
int16 PID_L_pos(void)
{
    static int32 Last_errorL = 0;       // 上次偏差
    
    int32 errorL = 0;                   // 当前偏差
    float P_outL = 0.0f;                // P环节输出
    static float I_outL = 0.0f;         // I环节输出
    float D_outL = 0.0f;                // D环节输出
	float PID_baseL = 0.0f;
    
    // 目标速度方向改变时，清零积分以避免反向拖后腿
    static int16 last_target_L = 0;
    if ((target_speed_L > 0 && last_target_L < 0) || 
        (target_speed_L < 0 && last_target_L > 0))
    {
        I_outL = 0.0f;
    }
    last_target_L = target_speed_L;
    
    errorL = target_speed_L - real_speed_L;     // 计算速度偏差 = 目标速度 - 实际速度
    
    P_outL = KP_v * (float)errorL;
    I_outL += KI_v * (float)errorL;
    
    
    // 积分限幅：给死区前馈和P项留出余量，防止暗饱和
    if(I_outL >  (MAX_SPD_OUT - MOTOR_DEAD_ZONE_L)) I_outL =  (MAX_SPD_OUT - MOTOR_DEAD_ZONE_L);
    if(I_outL < -(MAX_SPD_OUT - MOTOR_DEAD_ZONE_L)) I_outL = -(MAX_SPD_OUT - MOTOR_DEAD_ZONE_L);
    
//    D_outL = KD_v * (float)(errorL - Last_errorL);  // D环节：微分控制（当前KD_v=0，暂不使用）
    
    PID_outL = P_outL + I_outL + D_outL;             // 计算基础PID输出（不含死区前馈）
    
    // 根据目标速度方向叠加死区前馈偏置
    // target=0 时不加偏置，确保能真正停车
    if      (target_speed_L > 0) PID_outL += MOTOR_DEAD_ZONE_L;
    else if (target_speed_L < 0) PID_outL -= MOTOR_DEAD_ZONE_L;
    
    if(PID_outL >  MAX_SPD_OUT) PID_outL =  MAX_SPD_OUT;        // 输出限幅上限
    if(PID_outL < -MAX_SPD_OUT) PID_outL = -MAX_SPD_OUT;        // 输出限幅下限
    
    Last_errorL = errorL;                       // 更新上次偏差，供下次微分计算使用
    
    return (int32)PID_outL;                     // 返回左轮电机PWM控制值
}

// 函数名: PID_R_pos
// 功能: 右轮位置式速度PID控制器（含快速制动与死区前馈）
// 参数: target_speed_R - 右轮目标速度
//       real_speed_R   - 右轮实际速度（编码器反馈）
// 返回值: 右轮电机PWM控制输出，范围[-MAX_SPD_OUT, MAX_SPD_OUT]
// 说明: 与PID_L_pos对称，采用位置式PID算法实现右轮独立速度闭环控制。
//       当目标速度为0且实际速度较大时，主动反向制动快速停车。
//       内置死区前馈，target=0时不加偏置，确保能真正停车。
int16 PID_R_pos(void)
{
    static int32 Last_errorR = 0;       // 上次偏差
    
    int32 errorR = 0;                   // 当前偏差
    float P_outR = 0.0f;                // P环节输出
    static float I_outR = 0.0f;         // I环节输出
    float D_outR = 0.0f;                // D环节输出
    float PID_baseR = 0.0f;             // 基础PID输出（不含死区前馈）
    
    // 目标速度方向改变时，清零积分以避免反向拖后腿
    static int16 last_target_R = 0;
    if ((target_speed_R > 0 && last_target_R < 0) || 
        (target_speed_R < 0 && last_target_R > 0))
    {
        I_outR = 0.0f;
    }
    last_target_R = target_speed_R;
    
    errorR = target_speed_R - real_speed_R;     // 计算速度偏差 = 目标速度 - 实际速度
    
    P_outR = KP_v * (float)errorR;
    I_outR += KI_v * (float)errorR;
     
    // 积分限幅：给死区前馈和P项留出余量，防止暗饱和
    if(I_outR >  (MAX_SPD_OUT - MOTOR_DEAD_ZONE_R)) I_outR =  (MAX_SPD_OUT - MOTOR_DEAD_ZONE_R);
    if(I_outR < -(MAX_SPD_OUT - MOTOR_DEAD_ZONE_R)) I_outR = -(MAX_SPD_OUT - MOTOR_DEAD_ZONE_R);
    
//    D_outR = KD_v * (float)(errorR - Last_errorR);  // D环节：微分控制（当前KD_v=0，暂不使用）
    
    PID_outR = P_outR + I_outR + D_outR;             // 计算基础PID输出（不含死区前馈）
    
    // 根据目标速度方向叠加死区前馈偏置
    // target=0 时不加偏置，确保能真正停车
    if      (target_speed_R > 0) PID_outR += MOTOR_DEAD_ZONE_R;
    else if (target_speed_R < 0) PID_outR -= MOTOR_DEAD_ZONE_R;
    
    if(PID_outR >  MAX_SPD_OUT) PID_outR =  MAX_SPD_OUT;        // 输出限幅上限
    if(PID_outR < -MAX_SPD_OUT) PID_outR = -MAX_SPD_OUT;        // 输出限幅下限
    
    Last_errorR = errorR;                       // 更新上次偏差，供下次微分计算使用
    
    return (int32)PID_outR;                     // 返回右轮电机PWM控制值
}
