/**
 * @file legacy_element.c
 * @brief App 旧元素判断保留模块；当前活动双模式调度不接入这些入口。
 * @details 保存赛道元素辅助状态并提供十字、环岛等判定函数；未接入入口保持休眠，不改变主状态机。
 * @note 移植来源：Sirius20260718/project/code/element.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "system/lifecycle.h"
#include "app_config.h"
#include "control/legacy_element.h"
#include "control/tracking.h"
#include "control/legacy_roundabout.h"
#include "sensing/track_sensor.h"
#include "sensing/imu_proc.h"

/* 系统运行计时（2ms计数，config模块） */

/* 旧元素入口记录的控制拍时间，保留 int16_t；当前 App 不接入。 */
static int16_t s_enter_tick = 0;

/* 元素判断汇总（当前主状态机未调用，保留壳） */
/**
 * @brief 保留旧元素汇总调用壳及原调用边。
 * @details 按左环进入、右环进入、十字、跷跷板四个旧入口依次判断；
 *     这些调用之间不增加互斥或早退，后续判断仍可能覆写前一判断选择的主状态。
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
 * @details 竖电感和严格大于2800且竖不对称度严格小于25时写十字主态；条件不满足不写状态。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。
 */
void Element_JudgeCrossroads(void)
{
    const track_state_t *track = Track_GetState();
    if (track->filtered[1] + track->filtered[2] > 2800 && track->symmetry_y < 25)
    {
        Tracking_SetKernelState(CONTROL_KERNEL_CROSSROADS);
    }
}

/**
 * @brief 按原电感和判断十字退出。
 * @details 竖电感和严格小于2800时恢复普通循迹主态；等于门限时保留当前状态。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。
 */
void Element_JudgeCrossroadsExit(void)
{
    const track_state_t *track = Track_GetState();
    if (track->filtered[1] + track->filtered[2] < 2800)
    {
        Tracking_SetKernelState(CONTROL_KERNEL_TRACKING);
    }
}

/**
 * @brief 保留跷跷板进入判断与 int16_t 计时赋值。
 * @details 四电感和严格小于800时选择跷跷板，并把当前2ms拍计数窄化存入int16_t进入时间。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。
 */
void Element_JudgeTeeterboard(void)
{
    const track_state_t *track = Track_GetState();
    if (track->filtered[0] + track->filtered[1] + track->filtered[2] + track->filtered[3] < 800)
    {
        Tracking_SetKernelState(CONTROL_KERNEL_TEETERBOARD);
        s_enter_tick = Lifecycle_GetControlTicks();
    }
}

/**
 * @brief 保留跷跷板信号和计时两个独立退出判断。
 * @details 四电感和严格大于1000时恢复循迹，随后独立检查当前拍减进入拍是否大于300；
 *     两个if均可执行，进入时间仍使用原int16_t截断值，不合并退出条件。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。
 */
void Element_JudgeTeeterboardExit(void)
{
    /* 信号恢复与时间超限分别判断；两个 if 独立，保留 int16_t 进入计时截断。 */
    const track_state_t *track = Track_GetState();
    if (track->filtered[0] + track->filtered[1] + track->filtered[2] + track->filtered[3] > 1000)
    {
        Tracking_SetKernelState(CONTROL_KERNEL_TRACKING);
    }

    if ((Lifecycle_GetControlTicks() - s_enter_tick) > 300)
    {
        Tracking_SetKernelState(CONTROL_KERNEL_TRACKING);
    }
}

/**
 * @brief 保留按 Y 角度进入路障的旧判断。
 * @details 共享Y轴累计角严格大于60时写路障主态；不在本入口清角或执行电机控制。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。
 */
void Element_JudgeCask(void)
{
    if (IMU_GetState()->angle[1] > 60)
    {
        Tracking_SetKernelState(CONTROL_KERNEL_CASK);
    }
}

/**
 * @brief 保留按 Y 角度退出路障的旧判断。
 * @details 共享Y轴累计角严格小于20时恢复普通循迹主态；其余输入不改变状态。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。
 */
void Element_JudgeCaskExit(void)
{
    if (IMU_GetState()->angle[1] < 20)
    {
        Tracking_SetKernelState(CONTROL_KERNEL_TRACKING);
    }
}
