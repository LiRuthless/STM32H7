/**
 * @file roundabout.c
 * @brief App 环岛配置与旧入口；配置供电磁影子机使用，旧判断函数当前不接入。
 * @details 依据电感、角度与里程推进左右环岛阶段，并按阶段部分覆写控制剖面。
 * @note 移植来源：Sirius20260718/project/code/roundabout.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include "roundabout.h"
#include "control.h"
#include "element.h"
#include "motor.h"
#include "track_sensor.h"
#include "pid.h"
#include "imu_proc.h"

enum
{
    ROUNDABOUT_ROUND_PROFILE_PREENTER, /**< 入环前直行阶段的部分控制剖面。 */
    ROUNDABOUT_ROUND_PROFILE_TURN,     /**< 首次转向入环阶段的部分控制剖面。 */
    ROUNDABOUT_ROUND_PROFILE_IN,       /**< 环内循迹阶段的部分控制剖面。 */
    ROUNDABOUT_ROUND_PROFILE_OUT       /**< 出环恢复阶段的部分控制剖面。 */
};

/* App 每 5 ms 发布的 DL1B 距离，单位 mm；8192 表示无效，本模块只读。 */
extern uint16_t g_app_dl1b_distance_mm;
extern float App_AbsFloat(float a);        /* config模块 */

/* 旧环岛入口状态仅由休眠判断函数和 Reset 读写；当前 App 不调度这些入口。 */
uint8_t g_roundabout_state = ROUNDABOUT_STATE_NORMAL;
uint8_t g_roundabout_left_flag = 0;
uint8_t g_roundabout_right_flag = 0;
int8_t g_roundabout_sign_round = 1;

/* 旧入口的里程阈值，单位为滤波编码器脉冲；休眠判断函数只读。 */
uint16_t g_roundabout_enter_distance1 = 8000;
uint16_t g_roundabout_out_distance1 = 10000;

/* 旧入口的角度阈值，单位度；休眠判断函数只读。 */
int16_t g_roundabout_enter_angle1 = 30;
int16_t g_roundabout_enter_angle2 = 60;
int16_t g_roundabout_out_angle1 = 200;

/* 电磁影子状态机只读的四阶段部分剖面；fields 未置位的控制组由调用方保留。 */
static const control_profile_t s_round_profiles[4] =
{
    [ROUNDABOUT_ROUND_PROFILE_PREENTER]
    =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 20, 10, 1, 10 },
        150,
        { 2.0f, 0.008f, 15.0f, 1.0f },
        { 0 }
    },
    [ROUNDABOUT_ROUND_PROFILE_TURN] = { 0 },
    [ROUNDABOUT_ROUND_PROFILE_IN] =
    {
        CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 0 },
        150,
        { 2.0f, 0.008f, 15.0f, 1.0f },
        { 0 }
    },
    [ROUNDABOUT_ROUND_PROFILE_OUT]
    =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 20, 10, 1, 10 },
        150,
        { 2.0f, 0.008f, 15.0f, 1.0f },
        { 0 }
    }
};

/**
 * @brief 读取原四类环岛部分覆写配置。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @param[in] state ROUNDABOUT_ISLAND_* 状态；未知值返回 TURN 空配置。
 * @return 非空静态只读配置指针。
 * @note 电磁观察器调用，TURN 的字段位图为零，不复位上一拍配置。
 */
const control_profile_t *Roundabout_GetProfile(uint8_t state)
{
    /* 检查阶段索引后返回对应静态部分剖面；越界阶段回退到第一项，调用方再按 fields 合并。 */
    switch (state)
    {
        case ROUNDABOUT_ISLAND_LPREENTER:
            return &s_round_profiles[ROUNDABOUT_ROUND_PROFILE_PREENTER];
        case ROUNDABOUT_ISLAND_TURN_LEFT:
            return &s_round_profiles[ROUNDABOUT_ROUND_PROFILE_TURN];
        case ROUNDABOUT_ISLAND_IN:
            return &s_round_profiles[ROUNDABOUT_ROUND_PROFILE_IN];
        case ROUNDABOUT_ISLAND_OUT:
            return &s_round_profiles[ROUNDABOUT_ROUND_PROFILE_OUT];
        default:
            return &s_round_profiles[ROUNDABOUT_ROUND_PROFILE_TURN];
    }
}

/**
 * @brief 执行旧距离驱动环岛分支。
 * @details 按现有调用顺序使用 Control_ApplyProfile、Track_UpdateError、PID_UpdateTrack、Motor_DistributeSpeed
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。
 */
void Roundabout_Update(void)
{
    switch (g_roundabout_state)
    {
        case ROUNDABOUT_ISLAND_LPREENTER:

            Control_ApplyProfile(&s_round_profiles[ROUNDABOUT_ROUND_PROFILE_PREENTER]);

            (void)Track_UpdateError();
            g_control_track_output = PID_UpdateTrack();

            Motor_DistributeSpeed(g_control_track_output);

            Roundabout_JudgeAhead(); /* 是否满足距离 */
            break;

        case ROUNDABOUT_ISLAND_TURN_LEFT:

            Control_ApplyProfile(&s_round_profiles[ROUNDABOUT_ROUND_PROFILE_TURN]);

            Motor_DistributeSpeed(-30);

            Roundabout_JudgeEntered(); /* 检测距离是否完成 */
            break;

        case ROUNDABOUT_ISLAND_IN:

            Control_ApplyProfile(&s_round_profiles[ROUNDABOUT_ROUND_PROFILE_IN]);

            (void)Track_UpdateError();
            g_control_track_output = PID_UpdateTrack();
            Motor_DistributeSpeed(g_control_track_output);

            Roundabout_JudgeExit();
            break;

        case ROUNDABOUT_ISLAND_OUT:

            Control_ApplyProfile(&s_round_profiles[ROUNDABOUT_ROUND_PROFILE_OUT]);

            (void)Track_UpdateError();
            g_control_track_output = PID_UpdateTrack();

            Motor_DistributeSpeed(g_control_track_output);

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
            g_roundabout_left_flag = 1;
        }
        else
        {
            g_roundabout_right_flag = 1;
        }

        g_control_cask_flag = 0;
        IMU_ZeroAngleX();
        Motor_ResetDistance();
        g_control_kernel_state = CONTROL_KERNEL_REISLAND;
    }

    if (g_app_dl1b_distance_mm < 100) /* DL1B检测到近距障碍，置路障标志 */
    {
        g_control_cask_flag = 1;
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
        g_control_kernel_state = CONTROL_KERNEL_REISLAND;
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
        g_control_kernel_state = CONTROL_KERNEL_TRACKING;
        g_roundabout_left_flag = 0;
        g_roundabout_right_flag = 0;

        g_control_track_output = 0;
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
    if ((track->filtered[1] + track->filtered[2] < 300) && g_roundabout_right_flag == 1
        && !g_control_cask_flag)
    {
        g_roundabout_state = ROUNDABOUT_ISLAND_LPREENTER;
        g_control_kernel_state = CONTROL_KERNEL_ISLAND_R;

        g_control_track_output = 0;
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
    if (Motor_GetState()->distance >= g_roundabout_enter_distance1)
    {
        g_roundabout_state = ROUNDABOUT_ISLAND_TURN_LEFT;
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
        g_roundabout_state = ROUNDABOUT_ISLAND_IN;
        Motor_ResetDistance();
    }
}

/**
 * @brief 角度误差绝对值小于 5 度时转环内。
 * @details 按现有调用顺序使用 App_AbsFloat、PID_GetState 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。
 */
void Roundabout_JudgeEnteredAgain(void)
{
    if (App_AbsFloat(PID_GetState()->angle_err) < 5.0f) /* 角度误差小于5度，认为入环姿态已调整好 */
    {
        g_roundabout_state = ROUNDABOUT_ISLAND_IN;
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
        g_roundabout_state = ROUNDABOUT_ISLAND_OUT;
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
    if (Motor_GetState()->distance > g_roundabout_out_distance1) /* 行驶足够距离，确认出环完成 */
    {
        g_roundabout_left_flag = 0;
        g_roundabout_right_flag = 0;

        g_roundabout_state = ROUNDABOUT_STATE_NORMAL;
        g_control_kernel_state = CONTROL_KERNEL_TRACKING;
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
    g_roundabout_state = ROUNDABOUT_STATE_NORMAL;
    g_roundabout_left_flag = 0;
    g_roundabout_right_flag = 0;
    g_roundabout_sign_round = 1;
}
