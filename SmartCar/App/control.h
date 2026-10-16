/**
 * @file control.h
 * @brief App 双模式观察与唯一控制执行口；TIM6 调用，旧整体入口保留。
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
    TRACKING_MODE_EM = 0,
    TRACKING_MODE_OPTICAL = 1
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
    TRACKING_OK = 0,
    TRACKING_BUSY,
    TRACKING_SAVE_FAILED,
    TRACKING_ARG
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

extern int16_t g_control_track_output; /* 方向控制输出 */

extern uint8_t g_control_kernel_state;

extern uint8_t g_control_cask_flag;

/**
 * @brief 仅应用配置位图指定的控制字段。
 * @param[in] profile 非空配置；fields 仅选择原权重、基础速度、方向及角度四组。
 * @note 未选字段沿用上一拍，不将部分覆写改为整体重置；保持四个独立 if 的顺序。
 */
void Control_ApplyProfile(const control_profile_t *profile);

/**
 * @brief 读取原七个主状态对应的部分配置。
 * @param[in] state 主状态编号；小于 7 时按索引选择，其他值取普通循迹配置。
 * @return 非空静态配置指针，仅供读取，不改变参数。
 */
const control_profile_t *Control_GetKernelProfile(uint8_t state);

/**
 * @brief 将选中字段合入私有观察配置。
 * @param[in,out] context 非空目标；只覆盖 profile 选中的组并累计 fields。
 * @param[in] profile 非空源配置，保持原部分写入语义。
 * @note 不直接写入电机、共享 PID 或 IMU。
 */
void Control_MergeProfile(control_profile_t *context, const control_profile_t *profile);

/**
 * @brief 读取双模式调度状态。
 * @return 非空模块静态对象的只读指针，生命周期覆盖整个运行期。
 * @note 状态由既有控制/停车路径更新；前台要求一致快照时须在对应更新停止后读取。
 */
const tracking_state_t *Tracking_GetState(void);

/**
 * @brief 在停车和待写参数保存成功后切换循迹模式。
 * @param[in] mode 0 电磁、1 光电；其他值返回 ARG。
 * @return OK 成功；ARG 模式越界；BUSY 未停车；SAVE_FAILED 参数保存失败。
 * @note 前台调用；失败路径打印及副作用、成功后的各模块复位顺序保持。
 */
tracking_result_t Tracking_TrySetMode(uint8_t mode);

/**
 * @brief 在有效范围内更新所选档位。
 * @param[in] gear 0 低档、1 高档；其他值保持原档位。
 */
void Tracking_SetGear(uint8_t gear);

/**
 * @brief 在原临界区内累计保护位。
 * @param[in] flags TRACKING_PROTECT_* 位图，保持原 uint16_t 组合。
 * @note 前台或 ISR 可调用；恢复进入前 PRIMASK，不改变临界区边界。
 */
void Tracking_AddProtection(uint16_t flags);

/**
 * @brief 用同一传感器帧分别推进电磁与光电观察状态。
 * @param[in] running 零表示停车观察，非零表示已起跑；等待拍条件仍独立判断。
 * @param[in] stop_reason 原停车原因，零表示本次无停车原因。
 * @note TIM6 或停车路径调用；电磁只写私有上下文，发布监视字段后按所选模式取偏差。
 */
void Tracking_Observe(uint8_t running, uint8_t stop_reason);

/**
 * @brief 仅把所选模式观察结果应用到共享控制执行口。
 * @note TIM6 调用；光电仅 TRACKING 输出，电磁保留转向、里程/角度复位及最终输出顺序。
 */
void Tracking_Execute(void);

/**
 * @brief 将最新滤波轮速分别累加到电磁私有里程。
 * @note TIM6 编码器更新后调用，不复位共享运动基准。
 */
void Tracking_AccumulateMotion(void);

/**
 * @brief 将完整控制拍序号增加一次。
 * @note TIM6 完成控制后调用，uint32_t 自然回绕。
 */
void Tracking_CompleteControl(void);

/**
 * @brief 以停车原因推进光电状态并清方向输出。
 * @param[in] reason 原 app_stop_reason_t 对应的 uint8_t 原因值。
 * @note 保留灰度失线标志与首次原因的既有组合，不推进电磁执行。
 */
void Tracking_Stopped(uint8_t reason);

/**
 * @brief 保留完整双传感器检查与控制的旧封装入口。
 * @note 当前 App 调度使用分步接口；本函数调用关系保持，不新增接入或合并错误分支。
 */
void Control_UpdateLegacy(void);

/**
 * @brief 复位双观察状态与共享监视量，保持模式和档位。
 * @note 起跑/模式切换调用；控制参数、传感器标定及既有复位顺序不变。
 */
void Control_ResetRunState(void);

#endif
