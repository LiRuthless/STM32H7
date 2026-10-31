// ============================================================
// 文件名: control.c
// 功能说明: 控制策略模块
// whole_test() 主状态机：读四路电感→出赛道保护→按状态设权重/PID/
// 基础速度→循迹偏差→方向PID→差速→元素进出判断。
// ============================================================

#include "bsp.h"
#include "app_config.h"
#include "app.h"
#include "control.h"
#include "motor.h"
#include "track_sensor.h"
#include "pid.h"
#include "element.h"
#include "roundabout.h"

int16_t track_out = 0;          // 方向控制输出（由循迹PID计算）
uint8_t kernel_state = KERNEL_TRACKING;
uint8_t cask_flag = 0;

/* 只覆盖旧分支实际赋值的字段；未出现的字段沿用上一拍。 */
static const control_profile_t s_kernel_profiles[7] = {
    [KERNEL_TRACKING] = {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        {15, 20, 22, 10}, 245, {2.0f, 0.008f, 15.0f, 1.0f}, {0}
    },
    [KERNEL_ISLAND_L] = {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_ANGLE,
        {15, 20, 22, 10}, 0, {0}, {2.0f, 1.2f, 0.0f}
    },
    [KERNEL_ISLAND_R] = {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_ANGLE,
        {15, 20, 22, 10}, 150, {0}, {2.0f, 1.2f, 0.0f}
    },
    [KERNEL_TEETERBOARD] = {
        CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        {0}, 100, {4.5f, 0.0f, 6.2f, 0.9f}, {0}
    },
    [KERNEL_CROSSROADS] = {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        {15, 20, 2, 10}, 240, {2.0f, 0.008f, 15.0f, 1.0f}, {0}
    },
    [KERNEL_CASK] = {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        {15, 20, 2, 5}, 210, {2.0f, 0.008f, 15.0f, 1.0f}, {0}
    },
    [KERNEL_REISLAND] = {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        {20, 10, 1, 10}, 180, {2.0f, 0.008f, 15.0f, 1.0f}, {0}
    }
};

void Control_ApplyProfile(const control_profile_t *profile)
{
    if(profile->fields & CONTROL_PROFILE_WEIGHTS) Track_SetWeights(&profile->weights);
    if(profile->fields & CONTROL_PROFILE_BASE) Motor_SetBaseSpeed(profile->base_speed);
    if(profile->fields & CONTROL_PROFILE_TRACK) PID_SetTrackGains(&profile->track);
    if(profile->fields & CONTROL_PROFILE_ANGLE) PID_SetAngleGains(&profile->angle);
}

void whole_test(void)
{
    const track_state_t *track = Track_GetState();
    read_adc();     // 读取四路电感ADC值

    if( track->filtered[0] + track->filtered[1] + track->filtered[2] + track->filtered[3] > 300 )
    {
        Run_flag = 1;   // 标记已启动

        switch( kernel_state )
        {
            case KERNEL_TRACKING:

                Control_ApplyProfile(&s_kernel_profiles[KERNEL_TRACKING]);


                (void)get_track_error();
                track_out = PID_track();
                speed_control(track_out);

                crossroads_judge();
                L_reroundabout_judge();

            break;

            case KERNEL_REISLAND:

                Control_ApplyProfile(&s_kernel_profiles[KERNEL_REISLAND]);


                (void)get_track_error();
                track_out = PID_track();

                speed_control(track_out);

                R_roundabout_judge();
                reroundabout_out_judge();

            break;

            case KERNEL_CROSSROADS:

                Control_ApplyProfile(&s_kernel_profiles[KERNEL_CROSSROADS]);


                (void)get_track_error();
                track_out = PID_track();

                if( track_out >  10 )   track_out =  10;
                if( track_out < -10 )   track_out = -10;

                speed_control(track_out);

                crossroads_out_judge();

            break;

            case KERNEL_ISLAND_L:

                Control_ApplyProfile(&s_kernel_profiles[KERNEL_ISLAND_L]);


                sign_round = -1;
                roundabout();

            break;

            case KERNEL_ISLAND_R:

                Control_ApplyProfile(&s_kernel_profiles[KERNEL_ISLAND_R]);


                sign_round = 1;
                roundabout();

            break;

            case KERNEL_TEETERBOARD:

                Control_ApplyProfile(&s_kernel_profiles[KERNEL_TEETERBOARD]);


                (void)get_track_error();
                track_out = PID_track();
                speed_control(track_out);

                teeterboard_out_judge();

            break;

            case KERNEL_CASK:

                Control_ApplyProfile(&s_kernel_profiles[KERNEL_CASK]);


                (void)get_track_error();
                track_out = PID_track();
                speed_control(track_out);
                cask_out_judge();

            break;
        }
    }
    else    // 电感值过低，认为出赛道或停止线
    {
        App_RequestStop(APP_STOP_OFF_TRACK);
    }
}

// 函数名: Control_ResetRunState
// 功能: 再次起跑前复位主状态机，不修改任何控制参数
void Control_ResetRunState(void)
{
    kernel_state = KERNEL_TRACKING;
    cask_flag = 0;
    track_out = 0;
}
