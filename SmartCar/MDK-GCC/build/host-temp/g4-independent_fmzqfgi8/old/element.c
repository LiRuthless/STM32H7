// ============================================================
// 文件名: element.c
// 功能说明: 赛道元素处理模块
// 十字交叉、跷跷板、路障等元素的进入/离开判断，阈值沿用源工程。
// ============================================================

#include "bsp.h"
#include "app_config.h"
#include "element.h"
#include "control.h"
#include "roundabout.h"
#include "track_sensor.h"
#include "imu_proc.h"

extern int32_t time;    // 系统运行计时（2ms计数，config模块）

int16_t in_time = 0;

// 元素判断汇总（当前主状态机未调用，保留壳）
void element_judge(void)
{
    L_roundabout_judge();
    R_roundabout_judge();
    crossroads_judge();
    teeterboard_judge();
}

// 直道判断（源工程为空函数，保留壳）
void straight_judge(void)
{


}

void crossroads_judge(void)
{
    const track_state_t *track = Track_GetState();
    if( track->filtered[1] + track->filtered[2] > 2800 && track->symmetry_y < 25 )
    {
        kernel_state = KERNEL_CROSSROADS;
    }
}

void crossroads_out_judge(void)
{
    const track_state_t *track = Track_GetState();
    if( track->filtered[1] + track->filtered[2] < 2800 )
    {
        kernel_state = KERNEL_TRACKING;
    }
}

void teeterboard_judge(void)
{
    const track_state_t *track = Track_GetState();
    if( track->filtered[0] + track->filtered[1] + track->filtered[2] + track->filtered[3] < 800 )
    {
        kernel_state = KERNEL_TEETERBOARD;
        in_time = time;
    }
}

void teeterboard_out_judge(void)
{
    const track_state_t *track = Track_GetState();
    if( track->filtered[0] + track->filtered[1] + track->filtered[2] + track->filtered[3] > 1000 )
    {
        kernel_state = KERNEL_TRACKING;
    }

    if( (time - in_time) > 300 )
    {
        kernel_state = KERNEL_TRACKING;
    }
}

void cask_judge(void)
{
    if( IMU_GetState()->angle[1] > 60 )
    {
        kernel_state = KERNEL_CASK;
    }
}

void cask_out_judge(void)
{
    if( IMU_GetState()->angle[1] < 20 )
    {
        kernel_state = KERNEL_TRACKING;
    }
}
