/**
 * @file control.h
 * @brief App 双模式观察与唯一控制执行口；TIM6 调用，旧整体入口保留。
 * @details 声明双模式观察、执行及运行状态接口；完整算法、共享状态与调度约束位于 control.c。
 * @note 移植来源：Sirius20260718/project/code/control.h，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#ifndef SMARTCAR_APP_CONTROL_H
#define SMARTCAR_APP_CONTROL_H

#include <stdint.h>
#include "track_sensor.h"
#include "pid.h"

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

typedef enum
{
    TRACKING_MODE_EM = 0,      /**< 电磁循迹，执行电感影子状态机输出。 */
    TRACKING_MODE_OPTICAL = 1  /**< 光电循迹，执行灰度影子状态机输出。 */
} tracking_mode_t;

#define TRACKING_PROTECT_EM_LOST (1u << 0)
#define TRACKING_PROTECT_GRAY_LOST (1u << 1)
#define TRACKING_PROTECT_CALIBRATION (1u << 2)
#define TRACKING_PROTECT_ACQUIRE (1u << 3)
#define TRACKING_PROTECT_OVERRUN (1u << 4)
#define TRACKING_PROTECT_LOG_OVERFLOW (1u << 5)
#define TRACKING_PROTECT_LOG_WRITE (1u << 6)
#define TRACKING_PROTECT_LOG_FULL (1u << 7)
#define TRACKING_PROTECT_LOW_BATTERY (1u << 8)

typedef enum
{
    TRACKING_OK = 0,       /**< 请求完成，运行态与持久化状态均符合要求。 */
    TRACKING_BUSY,         /**< 车辆正在运行，不允许切换模式或档位。 */
    TRACKING_SAVE_FAILED,  /**< 运行态已更新，但参数持久化写入失败。 */
    TRACKING_ARG           /**< 模式或档位参数超出 0..1。 */
} tracking_result_t;

typedef struct
{
    uint8_t mode;              /**< 所选模式，0 电磁、1 光电；上电默认光电。 */
    uint8_t gear;              /**< 所选档位，0 低、1 高；上电默认低档。 */
    int16_t active_error;      /**< 所选模式本拍偏差，原 int16_t 标度。 */
    int16_t track_out;         /**< 唯一执行口发布的本拍方向输出，停车清零。 */
    uint32_t control_sequence; /**< 完整控制拍计数，TIM6 增加，起跑清零。 */
    uint16_t protection_flags; /**< 累计 TRACKING_PROTECT_* 保护位，原临界区内按位或。 */
} tracking_state_t;

/* 方向控制输出 */
extern int16_t g_control_track_output;

extern uint8_t g_control_kernel_state;

extern uint8_t g_control_cask_flag;

/**
 * @brief 仅应用配置位图指定的控制字段。
 * @note profile 非空配置；fields 仅选择原权重、基础速度、方向及角度四组。 未选字段沿用上一拍，不将部分覆写改为整体重置；保持四个独立 if 的顺序。 完整契约见
 *     control.c::Control_ApplyProfile。
 */
void Control_ApplyProfile(const control_profile_t *profile);

/**
 * @brief 读取原七个主状态对应的部分配置。
 * @note 完整契约见 control.c::Control_GetKernelProfile。
 */
const control_profile_t *Control_GetKernelProfile(uint8_t state);

/**
 * @brief 将选中字段合入私有观察配置。
 * @note context 非空目标；只覆盖 profile 选中的组并累计 fields。 profile 非空源配置，保持原部分写入语义。 不直接写入电机、共享 PID 或 IMU。
 *     完整契约见 control.c::Control_MergeProfile。
 */
void Control_MergeProfile(control_profile_t *context, const control_profile_t *profile);

/**
 * @brief 读取双模式调度状态。
 * @note 状态由既有控制/停车路径更新；前台要求一致快照时须在对应更新停止后读取。 完整契约见 control.c::Tracking_GetState。
 */
const tracking_state_t *Tracking_GetState(void);

/**
 * @brief 在停车和待写参数保存成功后切换循迹模式。
 * @note 前台调用；失败路径打印及副作用、成功后的各模块复位顺序保持。 完整契约见 control.c::Tracking_TrySetMode。
 */
tracking_result_t Tracking_TrySetMode(uint8_t mode);

/**
 * @brief 在有效范围内更新所选档位。
 * @note 完整契约见 control.c::Tracking_SetGear。
 */
void Tracking_SetGear(uint8_t gear);

/**
 * @brief 在原临界区内累计保护位。
 * @note 前台或 ISR 可调用；恢复进入前 PRIMASK，不改变临界区边界。 完整契约见 control.c::Tracking_AddProtection。
 */
void Tracking_AddProtection(uint16_t flags);

/**
 * @brief 用同一传感器帧分别推进电磁与光电观察状态。
 * @note TIM6 或停车路径调用；电磁只写私有上下文，发布监视字段后按所选模式取偏差。 完整契约见 control.c::Tracking_Observe。
 */
void Tracking_Observe(uint8_t running, uint8_t stop_reason);

/**
 * @brief 仅把所选模式观察结果应用到共享控制执行口。
 * @note TIM6 调用；光电仅 TRACKING 输出，电磁保留转向、里程/角度复位及最终输出顺序。 完整契约见 control.c::Tracking_Execute。
 */
void Tracking_Execute(void);

/**
 * @brief 将最新滤波轮速分别累加到电磁私有里程。
 * @note TIM6 编码器更新后调用，不复位共享运动基准。 完整契约见 control.c::Tracking_AccumulateMotion。
 */
void Tracking_AccumulateMotion(void);

/**
 * @brief 将完整控制拍序号增加一次。
 * @note TIM6 完成控制后调用，uint32_t 自然回绕。 完整契约见 control.c::Tracking_CompleteControl。
 */
void Tracking_CompleteControl(void);

/**
 * @brief 以停车原因推进光电状态并清方向输出。
 * @note 保留灰度失线标志与首次原因的既有组合，不推进电磁执行。 完整契约见 control.c::Tracking_Stopped。
 */
void Tracking_Stopped(uint8_t reason);

/**
 * @brief 保留完整双传感器检查与控制的旧封装入口。
 * @note 当前 App 调度使用分步接口；本函数调用关系保持，不新增接入或合并错误分支。 完整契约见 control.c::Control_UpdateLegacy。
 */
void Control_UpdateLegacy(void);

/**
 * @brief 复位双观察状态与共享监视量，保持模式和档位。
 * @note 起跑/模式切换调用；控制参数、传感器标定及既有复位顺序不变。 完整契约见 control.c::Control_ResetRunState。
 */
void Control_ResetRunState(void);

#endif
