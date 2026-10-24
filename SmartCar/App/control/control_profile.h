/**
 * @file control_profile.h
 * @brief 控制配置与环岛状态常量。
 * @details 声明主态与环岛部分配置、合并与应用接口；表和距离门限由control_profile.c持有。
 */

#ifndef SMARTCAR_APP_CONTROL_CONTROL_PROFILE_H
#define SMARTCAR_APP_CONTROL_CONTROL_PROFILE_H

#include <stdint.h>
#include "sensing/track_sensor.h"
#include "control/pid.h"

#define CONTROL_KERNEL_TRACKING 0
#define CONTROL_KERNEL_ISLAND_L 1
#define CONTROL_KERNEL_ISLAND_R 2
#define CONTROL_KERNEL_TEETERBOARD 3
#define CONTROL_KERNEL_CROSSROADS 4
#define CONTROL_KERNEL_CASK 5
#define CONTROL_KERNEL_REISLAND 6

#define CONTROL_PROFILE_WEIGHTS (1u << 0)
#define CONTROL_PROFILE_BASE (1u << 1)
#define CONTROL_PROFILE_TRACK (1u << 2)
#define CONTROL_PROFILE_ANGLE (1u << 3)

typedef struct
{
    uint8_t fields;          /**< CONTROL_PROFILE_* 选中组位图，未选组保持上一拍。 */
    track_weights_t weights; /**< 四项电感权重，仅在对应位设置时覆写。 */
    int16_t base_speed;      /**< 基础速度，每控制拍脉冲，仅在对应位设置时覆写。 */
    pid_track_gains_t track; /**< 方向四项增益，仅在对应位设置时覆写。 */
    pid_angle_gains_t angle; /**< 角度三项增益，仅在对应位设置时覆写。 */
} control_profile_t;

#define ROUNDABOUT_STATE_NORMAL 0      /* 正常循迹状态 */
#define ROUNDABOUT_ISLAND_LPREENTER 1  /* 左预进岛 */
#define ROUNDABOUT_ISLAND_RPREENTER 2  /* 右预进岛 */
#define ROUNDABOUT_ISLAND_TURN_LEFT 3  /* 左打角进岛 */
#define ROUNDABOUT_ISLAND_TURN_RIGHT 4 /* 右打角进岛 */
#define ROUNDABOUT_ISLAND_IN 5         /* 岛中（环岛内部循迹） */
#define ROUNDABOUT_ISLAND_EXIT 6       /* 出岛 */
#define ROUNDABOUT_ISLAND_LENTER 9     /* 走距离准备左进岛（预留） */
#define ROUNDABOUT_ISLAND_RENTER 10    /* 走距离准备右进岛（预留） */
#define ROUNDABOUT_ISLAND_OUT 11       /* 走距离准备出岛 */
#define ROUNDABOUT_ISLAND_TURN_TURN_LEFT 12

/**
 * @brief 仅应用配置位图指定的控制字段。
 * @note 未选字段沿用上一拍，不将部分覆写改为整体重置；保持四个独立 if 的顺序。
 *     完整契约见 control_profile.c::Control_ApplyProfile。
 */
void Control_ApplyProfile(const control_profile_t *profile);

/**
 * @brief 读取原七个主状态对应的部分配置。
 * @note 返回静态只读表项，生命周期为整个程序；调用方不得修改其内容。
 *     完整契约见 control_profile.c::Control_GetKernelProfile。
 */
const control_profile_t *Control_GetKernelProfile(uint8_t state);

/**
 * @brief 将选中字段合入私有观察配置。
 * @note 不直接写入电机、共享 PID 或 IMU。
 *     完整契约见 control_profile.c::Control_MergeProfile。
 */
void Control_MergeProfile(control_profile_t *dst, const control_profile_t *src);

/**
 * @brief 读取原四类环岛部分覆写配置。
 * @note 电磁观察器调用，TURN 的字段位图为零，不复位上一拍配置。
 *     完整契约见 control_profile.c::Roundabout_GetProfile。
 */
const control_profile_t *Roundabout_GetProfile(uint8_t state);

/**
 * @brief 读取环岛入环距离配置。
 * @note 完整契约见 control_profile.c::Control_GetRoundaboutEnterDistance。
 */
uint16_t Control_GetRoundaboutEnterDistance(void);

/**
 * @brief 读取环岛出环距离配置。
 * @note 完整契约见 control_profile.c::Control_GetRoundaboutOutDistance。
 */
uint16_t Control_GetRoundaboutOutDistance(void);

#endif
