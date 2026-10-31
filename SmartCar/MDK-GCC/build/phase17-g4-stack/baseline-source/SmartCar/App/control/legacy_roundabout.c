/**
 * @file legacy_roundabout.c
 * @brief 旧环岛判断、执行及复位入口，活动双模式调度不接入。
 * @details 按旧电感、角度及里程条件推进状态，通过只读 profile 接口取配置；
 *     私有旧标志由本模块维护，活动配置和距离门限由 control_profile 唯一持有。
 * @note 移植来源：Sirius20260718/project/code/roundabout.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "system/lifecycle.h"
#include "app_config.h"
#include "control/legacy_roundabout.h"
#include "control/tracking.h"
#include "control/legacy_element.h"
#include "control/motor.h"
#include "sensing/track_sensor.h"
#include "control/pid.h"
#include "control/legacy_pid.h"
#include "sensing/imu_proc.h"

typedef struct
{
    uint8_t left_flag; /**< 旧左环标志。 */
    uint8_t right_flag; /**< 旧右环标志。 */
    int8_t sign_round; /**< 旧方向标记，初值及 reset 为 1。 */
    int16_t enter_angle1; /**< 原入环角度 30 度。 */
    int16_t enter_angle2; /**< 原入环角度 60 度。 */
    int16_t out_angle1; /**< 原出环角度 200 度。 */
} legacy_roundabout_state_t;
/* 旧判断独占，起跑复位只清标志，不清保留角度配置。 */
static legacy_roundabout_state_t s_roundabout = { 0, 0, 1, 30, 60, 200 };
/**
 * @brief 按原分支求浮点绝对值。
 * @details 输入为负时取相反数，否则原值返回；不调用数学库，也不改变任何模块状态。
 * @param[in] a 待取绝对值的单精度数。
 * @return a<0 时取反，否则保持原值。
 * @note 保留旧环岛入口调用边。
 */
static float Roundabout_AbsFloat(float a)
{
    if (a < 0)
    {
        a = -a;
    }
    return a;
}

/**
 * @brief 执行旧距离驱动环岛分支。
 * @details 按现有调用顺序使用 Control_ApplyProfile、Track_UpdateError、PID_UpdateTrack、Motor_DistributeSpeed
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。
 */
void Roundabout_Update(void)
{
    switch (Tracking_GetRoundaboutState())
    {
        case ROUNDABOUT_ISLAND_LPREENTER:

            Control_ApplyProfile(Roundabout_GetProfile(Tracking_GetRoundaboutState()));

            (void)Track_UpdateError();
            Tracking_SetTrackOutput(PID_UpdateTrack(Track_GetState()->error, IMU_GetState()->accel[0]));

            Motor_DistributeSpeed(Tracking_GetTrackOutput());

            Roundabout_JudgeAhead(); /* 是否满足距离 */
            break;

        case ROUNDABOUT_ISLAND_TURN_LEFT:

            Control_ApplyProfile(Roundabout_GetProfile(Tracking_GetRoundaboutState()));

            Motor_DistributeSpeed(-30);

            Roundabout_JudgeEntered(); /* 检测距离是否完成 */
            break;

        case ROUNDABOUT_ISLAND_IN:

            Control_ApplyProfile(Roundabout_GetProfile(Tracking_GetRoundaboutState()));

            (void)Track_UpdateError();
            Tracking_SetTrackOutput(PID_UpdateTrack(Track_GetState()->error, IMU_GetState()->accel[0]));
            Motor_DistributeSpeed(Tracking_GetTrackOutput());

            Roundabout_JudgeExit();
            break;

        case ROUNDABOUT_ISLAND_OUT:

            Control_ApplyProfile(Roundabout_GetProfile(Tracking_GetRoundaboutState()));

            (void)Track_UpdateError();
            Tracking_SetTrackOutput(PID_UpdateTrack(Track_GetState()->error, IMU_GetState()->accel[0]));

            Motor_DistributeSpeed(Tracking_GetTrackOutput());

            Roundabout_JudgeExited();
            break;
    }
}

/**
 * @brief 按原横和阈值及障碍距离判断左预入环。
 * @details 按现有调用顺序使用 Track_GetState、IMU_ZeroAngleX、Motor_ResetDistance
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。
 */
void Roundabout_JudgePreentryLeft(void)
{
    const track_state_t *track = Track_GetState();
    if ((track->filtered[0] + track->filtered[3] > 2800))
    {
        if (track->filtered[0] > track->filtered[3])
        {
            s_roundabout.left_flag = 1;
        }
        else
        {
            s_roundabout.right_flag = 1;
        }

        Tracking_SetCaskFlag(0);
        IMU_ZeroAngleX();
        Motor_ResetDistance();
        Tracking_SetKernelState(CONTROL_KERNEL_REISLAND);
    }

    if (Lifecycle_GetDistanceMm() < 100) /* DL1B检测到近距障碍，置路障标志 */
    {
        Tracking_SetCaskFlag(1);
    }
}

/**
 * @brief 按原四路和/横和阈值判断右预入环。
 * @details 按现有调用顺序使用 Track_GetState、IMU_ZeroAngleX 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。
 */
void Roundabout_JudgePreentryRight(void)
{
    const track_state_t *track = Track_GetState();
    if ((track->filtered[0] + track->filtered[1] + track->filtered[2] + track->filtered[3] > 4500)
        || (track->filtered[0] + track->filtered[3] > 3000))
    {
        IMU_ZeroAngleX();
        Tracking_SetKernelState(CONTROL_KERNEL_REISLAND);
    }
}

/**
 * @brief 累计脉冲大于 5000 时清旧预入环状态。
 * @details 按现有调用顺序使用 Motor_GetState、IMU_ZeroAngleX 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。
 */
void Roundabout_JudgePreentryExit(void)
{
    if (Motor_GetState()->distance > 5000)
    {
        Tracking_SetKernelState(CONTROL_KERNEL_TRACKING);
        s_roundabout.left_flag = 0;
        s_roundabout.right_flag = 0;

        Tracking_SetTrackOutput(0);
        IMU_ZeroAngleX();
    }
}

/**
 * @brief 保留源左入环空壳。
 * @details 函数体不读取传感器、不写环岛状态且无返回值；保留旧汇总调用边，不补入左环策略。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。
 */
void Roundabout_JudgeEntryLeft(void)
{
    /* 源工程保留空壳，当前不接入；保持原空操作。 */
}

/**
 * @brief 按原竖和、方向及路障条件判断右入环。
 * @details 按现有调用顺序使用 Track_GetState、IMU_ZeroAngleX、Motor_ResetDistance
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。
 */
void Roundabout_JudgeEntryRight(void)
{
    const track_state_t *track = Track_GetState();
    if ((track->filtered[1] + track->filtered[2] < 300) && s_roundabout.right_flag == 1
        && !Tracking_GetCaskFlag())
    {
        Tracking_SetRoundaboutState(ROUNDABOUT_ISLAND_LPREENTER);
        Tracking_SetKernelState(CONTROL_KERNEL_ISLAND_R);

        Tracking_SetTrackOutput(0);
        IMU_ZeroAngleX();
        Motor_ResetDistance();
    }
}

/**
 * @brief 累计脉冲达到原入环距离时转打角状态。
 * @details 按现有调用顺序使用 Motor_GetState、Motor_ResetDistance 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。
 */
void Roundabout_JudgeAhead(void)
{
    if (Motor_GetState()->distance >= Control_GetRoundaboutEnterDistance())
    {
        Tracking_SetRoundaboutState(ROUNDABOUT_ISLAND_TURN_LEFT);
        Motor_ResetDistance();
    }
}

/**
 * @brief 累计脉冲大于 13000 时转环内。
 * @details 按现有调用顺序使用 Motor_GetState、Motor_ResetDistance 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。
 */
void Roundabout_JudgeEntered(void)
{
    if (Motor_GetState()->distance > 13000)
    {
        Tracking_SetRoundaboutState(ROUNDABOUT_ISLAND_IN);
        Motor_ResetDistance();
    }
}

/**
 * @brief 角度误差绝对值小于 5 度时转环内。
 * @details 按现有调用顺序使用 Roundabout_AbsFloat、PID_GetState 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。
 */
void Roundabout_JudgeEnteredAgain(void)
{
    if (Roundabout_AbsFloat(PID_GetState()->angle_err) < 5.0f) /* 角度误差小于5度，认为入环姿态已调整好 */
    {
        Tracking_SetRoundaboutState(ROUNDABOUT_ISLAND_IN);
    }
}

/**
 * @brief 保留源预出环空壳。
 * @details 函数体不读取里程或角度、不写状态且无返回值；保留旧阶段入口，不补入预出环策略。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。
 */
void Roundabout_JudgePreExit(void)
{
    /* 源工程保留空壳，当前不接入；保持原空操作。 */
}

/**
 * @brief 累计脉冲大于 32000 时转出环。
 * @details 按现有调用顺序使用 Motor_GetState、Motor_ResetDistance 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。
 */
void Roundabout_JudgeExit(void)
{
    if (Motor_GetState()->distance > 32000)
    {
        Tracking_SetRoundaboutState(ROUNDABOUT_ISLAND_OUT);
        Motor_ResetDistance();
    }
}

/**
 * @brief 累计脉冲大于原出环距离时清环岛状态。
 * @details 按现有调用顺序使用 Motor_GetState 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。
 */
void Roundabout_JudgeExited(void)
{
    if (Motor_GetState()->distance > Control_GetRoundaboutOutDistance()) /* 行驶足够距离，确认出环完成 */
    {
        s_roundabout.left_flag = 0;
        s_roundabout.right_flag = 0;

        Tracking_SetRoundaboutState(ROUNDABOUT_STATE_NORMAL);
        Tracking_SetKernelState(CONTROL_KERNEL_TRACKING);
    }
}

/**
 * @brief 保留源赛道判断空壳。
 * @details 函数体不读取传感器、不写共享状态且无返回值；保留旧公共入口，不接入当前调度。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。
 */
void Roundabout_Judge(void)
{
    /* 源工程保留空壳，当前不接入；保持原空操作。 */
}

/**
 * @brief 复位环岛状态和方向标志。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @note 起跑准备调用，不改距离与角度标定参数。
 */
void Roundabout_ResetRunState(void)
{
    Tracking_SetRoundaboutState(ROUNDABOUT_STATE_NORMAL);
    s_roundabout.left_flag = 0;
    s_roundabout.right_flag = 0;
    s_roundabout.sign_round = 1;
}
