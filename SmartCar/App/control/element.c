/**
 * @file element.c
 * @brief App 旧元素判断保留模块；当前活动双模式调度不接入这些入口。
 * @details 保存赛道元素辅助状态并提供十字、环岛等判定函数；未接入入口保持休眠，不改变主状态机。
 * @note 移植来源：Sirius20260718/project/code/element.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include "control/element.h"
#include "control/control.h"
#include "control/roundabout.h"
#include "sensing/track_sensor.h"
#include "sensing/imu_proc.h"

/* 系统运行计时（2ms计数，config模块） */
extern int32_t g_app_control_ticks;

/* 旧元素入口记录的控制拍时间，保留 int16_t；当前 App 不接入。 */
int16_t g_element_enter_tick = 0;

/* 元素判断汇总（当前主状态机未调用，保留壳） */
/**
 * @brief 保留旧元素汇总调用壳及原调用边。
 * @details 按现有调用顺序使用
 *     Roundabout_JudgeEntryLeft、Roundabout_JudgeEntryRight、Element_JudgeCrossroads、
 *     Element_JudgeTeeterboard
 *     完成本函数处理。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。
 */
void Element_Judge(void)
{
    Roundabout_JudgeEntryLeft();
    Roundabout_JudgeEntryRight();
    Element_JudgeCrossroads();
    Element_JudgeTeeterboard();
}

/* 直道判断（源工程为空函数，保留壳） */
/**
 * @brief 保留源工程空直道判断入口。
 * @details 函数体不读取输入、不写状态且无返回值；保留符号供旧元素汇总调用，不补入新判定。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。
 */
void Element_JudgeStraight(void)
{
    /* 源工程保留空壳，当前不接入；保持原空操作。 */
}

/**
 * @brief 按原电感和与对称度判断十字进入。
 * @details 按现有调用顺序使用 Track_GetState 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。
 */
void Element_JudgeCrossroads(void)
{
    const track_state_t *track = Track_GetState();
    if (track->filtered[1] + track->filtered[2] > 2800 && track->symmetry_y < 25)
    {
        g_control_kernel_state = CONTROL_KERNEL_CROSSROADS;
    }
}

/**
 * @brief 按原电感和判断十字退出。
 * @details 按现有调用顺序使用 Track_GetState 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。
 */
void Element_JudgeCrossroadsExit(void)
{
    const track_state_t *track = Track_GetState();
    if (track->filtered[1] + track->filtered[2] < 2800)
    {
        g_control_kernel_state = CONTROL_KERNEL_TRACKING;
    }
}

/**
 * @brief 保留跷跷板进入判断与 int16_t 计时赋值。
 * @details 按现有调用顺序使用 Track_GetState 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。
 */
void Element_JudgeTeeterboard(void)
{
    const track_state_t *track = Track_GetState();
    if (track->filtered[0] + track->filtered[1] + track->filtered[2] + track->filtered[3] < 800)
    {
        g_control_kernel_state = CONTROL_KERNEL_TEETERBOARD;
        g_element_enter_tick = g_app_control_ticks;
    }
}

/**
 * @brief 保留跷跷板信号和计时两个独立退出判断。
 * @details 按现有调用顺序使用 Track_GetState 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。
 */
void Element_JudgeTeeterboardExit(void)
{
    /* 按当前角速度符号和既有状态分支更新跷跷板退出标志；该入口仍未接入活动调度。 */
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

/**
 * @brief 保留按 Y 角度进入路障的旧判断。
 * @details 按现有调用顺序使用 IMU_GetState 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。
 */
void Element_JudgeCask(void)
{
    if (IMU_GetState()->angle[1] > 60)
    {
        g_control_kernel_state = CONTROL_KERNEL_CASK;
    }
}

/**
 * @brief 保留按 Y 角度退出路障的旧判断。
 * @details 按现有调用顺序使用 IMU_GetState 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。
 */
void Element_JudgeCaskExit(void)
{
    if (IMU_GetState()->angle[1] < 20)
    {
        g_control_kernel_state = CONTROL_KERNEL_TRACKING;
    }
}
