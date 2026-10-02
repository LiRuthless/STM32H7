// ============================================================
// 文件名: roundabout.c
// 功能说明: 环岛（roundabout）识别与控制模块
// 当前生效逻辑为纯距离驱动的环岛子状态机（预入环→打角入环→
// 环内循迹→出环），源工程中被整体注释的角度法代码未移植。
// ============================================================

#include "bsp.h"
#include "app_config.h"
#include "roundabout.h"
#include "control.h"
#include "element.h"
#include "motor.h"
#include "track_sensor.h"
#include "pid.h"
#include "imu_proc.h"

extern uint16_t dl1b_distance_mm;   // DL1B激光测距（mm），由app.c定义并周期更新
extern float float_abs(float a);    // config模块

uint8_t roundabout_state = STATE_NORMAL;    // 当前赛道元素状态机状态
uint8_t L_round_flag = 0;                   // 左环岛处理标志（1表示正在处理环岛）
uint8_t R_round_flag = 0;                   // 右环岛处理标志（1表示正在处理环岛）

int8_t sign_round = 1;

uint16_t enter_distance1 = 8000;        // 环岛1入环前直行距离
uint16_t out_distance1 = 10000;         // 环岛1出环后直行距离

int16_t enter_angle1 = 30;              // 环岛1入环初始打角角度
int16_t enter_angle2 = 60;
int16_t out_angle1 = 200;               // 环岛1出环目标角度


void roundabout(void)
{
    switch( roundabout_state )
    {
        case ISLAND_LPREENTER:

            weight_x   = 20;    //15
            weight_xx  = 10;    //20
            weight_y   = 1;     //28
            weight_abs = 10;    //5

            base_speed = 150;   //500--2.9
            KP_x  = 2;          //2
            K2P_x = 0.008;      //0.008
            KD_x  = 15;         //15
            K2D_x = 1;          //1

            track_error = get_track_error();
            track_out = PID_track();

            speed_control(track_out);

            ahead_judge();      // 是否满足距离
        break;

        case ISLAND_TURN_LEFT:

            speed_control(-30);

            entered_judge();    // 检测距离是否完成
        break;

        case ISLAND_IN:

            base_speed = 150;   //500--2.9
            KP_x  = 2;          //2
            K2P_x = 0.008;      //0.008
            KD_x  = 15;         //15
            K2D_x = 1;          //1

            track_error = get_track_error();
            track_out = PID_track();
            speed_control(track_out);

            exit_judge();
        break;

        case ISLAND_OUT:

            weight_x   = 20;    //15
            weight_xx  = 10;    //20
            weight_y   = 1;     //28
            weight_abs = 10;    //5

            base_speed = 150;   //500--2.9
            KP_x  = 2;          //2
            K2P_x = 0.008;      //0.008
            KD_x  = 15;         //15
            K2D_x = 1;          //1

            track_error = get_track_error();
            track_out = PID_track();

            speed_control(track_out);

            outed_judge();
        break;
    }
}


void L_reroundabout_judge(void)
{
    if( (adc_filted[0] + adc_filted[3] > 2800) )
    {
        if( adc_filted[0] > adc_filted[3] )
        {
            L_round_flag = 1;
        }
        else
        {
            R_round_flag = 1;
        }

        cask_flag = 0;
        angle_x = 0;
        Distance = 0;
        distance_L = 0;
        distance_R = 0;
        kernel_state = KERNEL_REISLAND;
    }

    if( dl1b_distance_mm < 100 )    // DL1B检测到近距障碍，置路障标志
    {
        cask_flag = 1;
    }
}

void R_reroundabout_judge(void)
{
    if( (adc_filted[0] + adc_filted[1] + adc_filted[2] + adc_filted[3] > 4500)
     || (adc_filted[0] + adc_filted[3] > 3000) )
    {
        angle_x = 0;
        kernel_state = KERNEL_REISLAND;
    }
}

void reroundabout_out_judge(void)
{
    if( Distance > 5000 )
    {
        kernel_state = KERNEL_TRACKING;
        L_round_flag = 0;
        R_round_flag = 0;

        track_out = 0;
        angle_x = 0;
    }
}


// 函数名: L_roundabout_judge
// 功能: 左环岛入环条件判断（源工程条件已整体注释，保留空壳）
void L_roundabout_judge(void)
{

}

// 函数名: R_roundabout_judge
// 功能: 右环岛入环条件判断
void R_roundabout_judge(void)
{
    if( (adc_filted[1] + adc_filted[2] < 300) && R_round_flag == 1 && !cask_flag )
    {
        roundabout_state = ISLAND_LPREENTER;
        kernel_state = KERNEL_ISLAND_R;

        track_out = 0;
        angle_x = 0;
        distance_L = distance_R = Distance = 0;
    }
}


// 函数名: ahead_judge
// 功能: 预入环阶段距离判断：直行距离达到设定值后转入打角入环
void ahead_judge(void)
{
    if( Distance >= enter_distance1 )
    {
        roundabout_state = ISLAND_TURN_LEFT;
        distance_L = distance_R = Distance = 0;
    }
}

// 函数名: entered_judge
// 功能: 入环打角完成判断（距离驱动）
void entered_judge(void)
{
    if( Distance > 13000 )
    {
        roundabout_state = ISLAND_IN;
        distance_L = distance_R = Distance = 0;
    }
}

void entered_entered_judge(void)
{
    if( float_abs(angle_err) < 5.0f )   // 角度误差小于5度，认为入环姿态已调整好
    {
        roundabout_state = ISLAND_IN;
    }
}

// 函数名: pre_out_judge
// 功能: 环岛预出环判断（源工程已整体注释，保留空壳）
void pre_out_judge(void)
{

}

// 函数名: exit_judge
// 功能: 出环判断（距离驱动）：环内行驶足够距离后转入出环直行
void exit_judge(void)
{
    if( Distance > 32000 )
    {
        roundabout_state = ISLAND_OUT;
        distance_L = distance_R = Distance = 0;
    }
}

// 函数名: outed_judge
// 功能: 出环完成判断：出环后直行足够距离，清环岛标志并返回正常循迹
void outed_judge(void)
{
    if( Distance > out_distance1 )  // 行驶足够距离，确认出环完成
    {
        L_round_flag = 0;
        R_round_flag = 0;

        roundabout_state = STATE_NORMAL;
        kernel_state = KERNEL_TRACKING;
    }
}

// 函数名: judge
// 功能: 赛道元素状态机主分支（源工程已整体注释，保留空壳）
void judge(void)
{

}

// 函数名: Roundabout_ResetRunState
// 功能: 再次起跑前恢复环岛状态机初值，不修改距离/角度标定参数
void Roundabout_ResetRunState(void)
{
    roundabout_state = STATE_NORMAL;
    L_round_flag = 0;
    R_round_flag = 0;
    sign_round = 1;
}
