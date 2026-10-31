/**
 * @file control_profile.c
 * @brief 控制部分覆写配置与环岛距离。
 * @details 静态表供观察器只读；执行应用按权重、基础速度、方向、角度依次覆写。
 * @note 移植来源：Sirius20260718/project/code/control.c / roundabout.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源文件未单列作者及独立许可证；保留出处，关联 Phase 13/15。
 */
#include "control/control_profile.h"
#include "control/motor.h"
/* 只覆盖旧分支实际赋值的字段；未出现的字段沿用上一拍。 */
static const control_profile_t s_kernel_profiles[7] =
{
    [CONTROL_KERNEL_TRACKING]
    =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 15, 20, 22, 10 },
        245,
        { 2.0f, 0.008f, 15.0f, 1.0f },
        { 0 }
    },
    [CONTROL_KERNEL_ISLAND_L] =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_ANGLE,
        { 15, 20, 22, 10 },
        0,
        { 0 },
        { 2.0f, 1.2f, 0.0f }
    },
    [CONTROL_KERNEL_ISLAND_R]
    =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_ANGLE,
        { 15, 20, 22, 10 },
        150,
        { 0 },
        { 2.0f, 1.2f, 0.0f }
    },
    [CONTROL_KERNEL_TEETERBOARD] =
    {
        CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 0 },
        100,
        { 4.5f, 0.0f, 6.2f, 0.9f },
        { 0 }
    },
    [CONTROL_KERNEL_CROSSROADS]
    =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 15, 20, 2, 10 },
        240,
        { 2.0f, 0.008f, 15.0f, 1.0f },
        { 0 }
    },
    [CONTROL_KERNEL_CASK]
    =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 15, 20, 2, 5 },
        210,
        { 2.0f, 0.008f, 15.0f, 1.0f },
        { 0 }
    },
    [CONTROL_KERNEL_REISLAND]
    =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 20, 10, 1, 10 },
        180,
        { 2.0f, 0.008f, 15.0f, 1.0f },
        { 0 }
    }
};

/**
 * @brief 仅应用配置位图指定的控制字段。
 * @details 按现有调用顺序使用 Track_SetWeights、Motor_SetBaseSpeed、PID_SetTrackGains、PID_SetAngleGains
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] profile 非空配置；fields 仅选择原权重、基础速度、方向及角度四组。
 * @note 未选字段沿用上一拍，不将部分覆写改为整体重置；保持四个独立 if 的顺序。
 */
void Control_ApplyProfile(const control_profile_t *profile)
{
    /* 依次检查四个 fields 位并只覆写被选中的权重、基础速度、方向增益和角度增益；未选字段继续沿用上一拍。 */
    if (profile->fields & CONTROL_PROFILE_WEIGHTS)
    {
        Track_SetWeights(&profile->weights);
    }
    if (profile->fields & CONTROL_PROFILE_BASE)
    {
        Motor_SetBaseSpeed(profile->base_speed);
    }
    if (profile->fields & CONTROL_PROFILE_TRACK)
    {
        PID_SetTrackGains(&profile->track);
    }
    if (profile->fields & CONTROL_PROFILE_ANGLE)
    {
        PID_SetAngleGains(&profile->angle);
    }
}

/**
 * @brief 读取原七个主状态对应的部分配置。
 * @details 返回本模块持有的控制剖面地址，供观察合并过程只读使用；不复制数据，也不改变状态机。
 * @param[in] state 主状态编号；小于 7 时按索引选择，其他值取普通循迹配置。
 * @return 非空静态配置指针，仅供读取，不改变参数。
 * @note 返回静态只读表项，生命周期为整个程序；调用方不得修改其内容。
 */
const control_profile_t *Control_GetKernelProfile(uint8_t state)
{
    return &s_kernel_profiles[state < 7 ? state : CONTROL_KERNEL_TRACKING];
}

/**
 * @brief 将选中字段合入私有观察配置。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @param[out] dst dst 为调用方提供的输出对象，必须非空；函数按实现顺序写入。
 * @param[in] src src 为调用方提供的只读输入，必须指向满足接口容量要求的有效对象。
 * @note 不直接写入电机、共享 PID 或 IMU。
 */
void Control_MergeProfile(control_profile_t *dst, const control_profile_t *src)
{
    /* 按覆盖位从 src 向 dst 逐组复制配置并累计 fields；未声明的组保持 dst 原值，支持状态机分层部分覆写。 */
    if (src->fields & CONTROL_PROFILE_WEIGHTS)
    {
        dst->weights = src->weights;
    }
    if (src->fields & CONTROL_PROFILE_BASE)
    {
        dst->base_speed = src->base_speed;
    }
    if (src->fields & CONTROL_PROFILE_TRACK)
    {
        dst->track = src->track;
    }
    if (src->fields & CONTROL_PROFILE_ANGLE)
    {
        dst->angle = src->angle;
    }
    dst->fields |= src->fields;
}
enum
{
    ROUNDABOUT_ROUND_PROFILE_PREENTER, /**< 入环前直行阶段的部分控制剖面。 */
    ROUNDABOUT_ROUND_PROFILE_TURN,     /**< 首次转向入环阶段的部分控制剖面。 */
    ROUNDABOUT_ROUND_PROFILE_IN,       /**< 环内循迹阶段的部分控制剖面。 */
    ROUNDABOUT_ROUND_PROFILE_OUT       /**< 出环恢复阶段的部分控制剖面。 */
};

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
 * @details 仅左预进、左打角、岛中及出岛四个状态选择表项；其他值返回 TURN 空配置。
 * @param[in] state ROUNDABOUT_ISLAND_* 状态；未知值返回 TURN 空配置。
 * @return 非空静态只读配置指针。
 * @note 电磁观察器调用，TURN 的字段位图为零，不复位上一拍配置。
 */
const control_profile_t *Roundabout_GetProfile(uint8_t state)
{
    /* 检查阶段后返回对应静态部分剖面；未知阶段返回 TURN 空配置，调用方再按 fields 合并。 */
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

/* 配置距离为滤波编码器累计脉冲，只读访问保持原比较时点。 */
static uint16_t s_enter_distance = 8000;
static uint16_t s_out_distance = 10000;

/**
 * @brief 读取环岛入环距离配置。
 * @return 滤波编码器累计脉冲阈值。
 */
uint16_t Control_GetRoundaboutEnterDistance(void)
{
    return s_enter_distance;
}

/**
 * @brief 读取环岛出环距离配置。
 * @return 滤波编码器累计脉冲阈值。
 */
uint16_t Control_GetRoundaboutOutDistance(void)
{
    return s_out_distance;
}
