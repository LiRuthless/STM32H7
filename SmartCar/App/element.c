/**
 * @file element.c
 * @brief App 旧元素判断保留模块；当前活动双模式调度不接入这些入口。
 * @note 移植来源：Sirius20260718/project/code/element.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include "element.h"
#include "control.h"
#include "roundabout.h"
#include "track_sensor.h"
#include "imu_proc.h"

extern int32_t g_app_control_ticks; /* 系统运行计时（2ms计数，config模块） */

/* 旧元素入口记录的控制拍时间，保留 int16_t；当前 App 不接入。 */
int16_t g_element_enter_tick = 0;

/* 元素判断汇总（当前主状态机未调用，保留壳） */
void Element_Judge(void)
{
    Roundabout_JudgeEntryLeft();
    Roundabout_JudgeEntryRight();
    Element_JudgeCrossroads();
    Element_JudgeTeeterboard();
}

/* 直道判断（源工程为空函数，保留壳） */
void Element_JudgeStraight(void)
{
    /* 源工程保留空壳，当前不接入；保持原空操作。 */
}

void Element_JudgeCrossroads(void)
{
    const track_state_t *track = Track_GetState();
    if (track->filtered[1] + track->filtered[2] > 2800 && track->symmetry_y < 25)
    {
        g_control_kernel_state = CONTROL_KERNEL_CROSSROADS;
    }
}

void Element_JudgeCrossroadsExit(void)
{
    const track_state_t *track = Track_GetState();
    if (track->filtered[1] + track->filtered[2] < 2800)
    {
        g_control_kernel_state = CONTROL_KERNEL_TRACKING;
    }
}

void Element_JudgeTeeterboard(void)
{
    const track_state_t *track = Track_GetState();
    if (track->filtered[0] + track->filtered[1] + track->filtered[2] + track->filtered[3] < 800)
    {
        g_control_kernel_state = CONTROL_KERNEL_TEETERBOARD;
        g_element_enter_tick = g_app_control_ticks;
    }
}

void Element_JudgeTeeterboardExit(void)
{
    const track_state_t *track = Track_GetState();
    if (track->filtered[0] + track->filtered[1] + track->filtered[2] + track->filtered[3] > 1000)
    {
        g_control_kernel_state = CONTROL_KERNEL_TRACKING;
    }

    if ((g_app_control_ticks - g_element_enter_tick) > 300)
    {
        g_control_kernel_state = CONTROL_KERNEL_TRACKING;
    }
}

void Element_JudgeCask(void)
{
    if (IMU_GetState()->angle[1] > 60)
    {
        g_control_kernel_state = CONTROL_KERNEL_CASK;
    }
}

void Element_JudgeCaskExit(void)
{
    if (IMU_GetState()->angle[1] < 20)
    {
        g_control_kernel_state = CONTROL_KERNEL_TRACKING;
    }
}
