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

enum {
    ROUND_PROFILE_PREENTER,
    ROUND_PROFILE_TURN,
    ROUND_PROFILE_IN,
    ROUND_PROFILE_OUT
};

static const control_profile_t s_round_profiles[4] = {
    [ROUND_PROFILE_PREENTER] = {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        {20, 10, 1, 10}, 150, {2.0f, 0.008f, 15.0f, 1.0f}, {0}
    },
    [ROUND_PROFILE_TURN] = {0},
    [ROUND_PROFILE_IN] = {
        CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        {0}, 150, {2.0f, 0.008f, 15.0f, 1.0f}, {0}
    },
    [ROUND_PROFILE_OUT] = {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        {20, 10, 1, 10}, 150, {2.0f, 0.008f, 15.0f, 1.0f}, {0}
    }
};


void roundabout(void)
{
    switch( roundabout_state )
    {
        case ISLAND_LPREENTER:

            Control_ApplyProfile(&s_round_profiles[ROUND_PROFILE_PREENTER]);


            (void)get_track_error();
            track_out = PID_track();

            speed_control(track_out);

            ahead_judge();      // 是否满足距离
        break;

        case ISLAND_TURN_LEFT:

            Control_ApplyProfile(&s_round_profiles[ROUND_PROFILE_TURN]);

            speed_control(-30);

            entered_judge();    // 检测距离是否完成
        break;

        case ISLAND_IN:

            Control_ApplyProfile(&s_round_profiles[ROUND_PROFILE_IN]);


            (void)get_track_error();
            track_out = PID_track();
            speed_control(track_out);

            exit_judge();
        break;

        case ISLAND_OUT:

            Control_ApplyProfile(&s_round_profiles[ROUND_PROFILE_OUT]);


            (void)get_track_error();
            track_out = PID_track();

            speed_control(track_out);

            outed_judge();
        break;
    }
}


void L_reroundabout_judge(void)
{
    const track_state_t *track = Track_GetState();
    if( (track->filtered[0] + track->filtered[3] > 2800) )
    {
        if( track->filtered[0] > track->filtered[3] )
        {
            L_round_flag = 1;
        }
        else
        {
            R_round_flag = 1;
        }

        cask_flag = 0;
        IMU_ZeroAngleX();
        Motor_ResetDistance();
        kernel_state = KERNEL_REISLAND;
    }

    if( dl1b_distance_mm < 100 )    // DL1B检测到近距障碍，置路障标志
    {
        cask_flag = 1;
    }
}

void R_reroundabout_judge(void)
{
    const track_state_t *track = Track_GetState();
    if( (track->filtered[0] + track->filtered[1] + track->filtered[2] + track->filtered[3] > 4500)
     || (track->filtered[0] + track->filtered[3] > 3000) )
    {
        IMU_ZeroAngleX();
        kernel_state = KERNEL_REISLAND;
    }
}

void reroundabout_out_judge(void)
{
    if( Motor_GetState()->distance > 5000 )
    {
        kernel_state = KERNEL_TRACKING;
        L_round_flag = 0;
        R_round_flag = 0;

        track_out = 0;
        IMU_ZeroAngleX();
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
    const track_state_t *track = Track_GetState();
    if( (track->filtered[1] + track->filtered[2] < 300) && R_round_flag == 1 && !cask_flag )
    {
        roundabout_state = ISLAND_LPREENTER;
        kernel_state = KERNEL_ISLAND_R;

        track_out = 0;
        IMU_ZeroAngleX();
        Motor_ResetDistance();
    }
}


// 函数名: ahead_judge
// 功能: 预入环阶段距离判断：直行距离达到设定值后转入打角入环
void ahead_judge(void)
{
    if( Motor_GetState()->distance >= enter_distance1 )
    {
        roundabout_state = ISLAND_TURN_LEFT;
        Motor_ResetDistance();
    }
}

// 函数名: entered_judge
// 功能: 入环打角完成判断（距离驱动）
void entered_judge(void)
{
    if( Motor_GetState()->distance > 13000 )
    {
        roundabout_state = ISLAND_IN;
        Motor_ResetDistance();
    }
}

void entered_entered_judge(void)
{
    if( float_abs(PID_GetState()->angle_err) < 5.0f )   // 角度误差小于5度，认为入环姿态已调整好
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
    if( Motor_GetState()->distance > 32000 )
    {
        roundabout_state = ISLAND_OUT;
        Motor_ResetDistance();
    }
}

// 函数名: outed_judge
// 功能: 出环完成判断：出环后直行足够距离，清环岛标志并返回正常循迹
void outed_judge(void)
{
    if( Motor_GetState()->distance > out_distance1 )  // 行驶足够距离，确认出环完成
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
