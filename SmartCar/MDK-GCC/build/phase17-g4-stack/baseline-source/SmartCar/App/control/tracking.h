/**
 * @file tracking.h
 * @brief 双模式状态与监视量访问。
 * @details 声明对应实现的模块接口，私有历史不通过本头公开。
 */

#ifndef SMARTCAR_APP_CONTROL_TRACKING_H
#define SMARTCAR_APP_CONTROL_TRACKING_H

#include "control/control_profile.h"

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

/**
 * @brief 读取双模式调度状态。
 * @note 状态由既有控制/停车路径更新；前台要求一致快照时须在对应更新停止后读取。
 *     完整契约见 tracking.c::Tracking_GetState。
 */
const tracking_state_t *Tracking_GetState(void);

/**
 * @brief 在停车和待写参数保存成功后切换循迹模式。
 * @note 前台调用；失败路径打印及副作用、成功后的各模块复位顺序保持。
 *     完整契约见 tracking.c::Tracking_TrySetMode。
 */
tracking_result_t Tracking_TrySetMode(uint8_t mode);

/**
 * @brief 在有效范围内更新所选档位。
 * @note 停车菜单或参数加载路径调用；只更新 s_tracking.gear，参数应用由原调用方协调。
 *     完整契约见 tracking.c::Tracking_SetGear。
 */
void Tracking_SetGear(uint8_t gear);

/**
 * @brief 将完整控制拍序号增加一次。
 * @note TIM6 完成控制后调用，uint32_t 自然回绕。
 *     完整契约见 tracking.c::Tracking_CompleteControl。
 */
void Tracking_CompleteControl(void);

/**
 * @brief 以停车原因推进光电状态并清方向输出。
 * @note 保留灰度失线标志与首次原因的既有组合，不推进电磁执行。
 *     完整契约见 tracking.c::Tracking_Stopped。
 */
void Tracking_Stopped(uint8_t reason);

/**
 * @brief 在原临界区内累计保护位。
 * @note 前台或 ISR 可调用；恢复进入前 PRIMASK，不改变临界区边界。
 *     完整契约见 tracking.c::Tracking_AddProtection。
 */
void Tracking_AddProtection(uint16_t flags);

/**
 * @brief 用同一传感器帧分别推进电磁与光电观察状态。
 * @note TIM6 或停车路径调用；电磁只写私有上下文，发布监视字段后按所选模式取偏差。
 *     完整契约见 tracking.c::Tracking_Observe。
 */
void Tracking_Observe(uint8_t running, uint8_t stop_reason);

/**
 * @brief 仅把所选模式观察结果应用到共享控制执行口。
 * @note TIM6 调用；光电仅 TRACKING 输出，电磁保留转向、里程/角度复位及最终输出顺序。
 *     完整契约见 tracking.c::Tracking_Execute。
 */
void Tracking_Execute(void);

/**
 * @brief 将最新滤波轮速分别累加到电磁私有里程。
 * @note TIM6 编码器更新后调用，不复位共享运动基准。
 *     完整契约见 tracking.c::Tracking_AccumulateMotion。
 */
void Tracking_AccumulateMotion(void);

/**
 * @brief 复位双观察状态与共享监视量，保持模式和档位。
 * @note 起跑/模式切换调用；控制参数、传感器标定及既有复位顺序不变。
 *     完整契约见 tracking.c::Control_ResetRunState。
 */
void Control_ResetRunState(void);

/**
 * @brief 读取方向输出监视值。
 * @note 完整契约见 tracking.c::Tracking_GetTrackOutput。
 */
int16_t Tracking_GetTrackOutput(void);

/**
 * @brief 写入方向输出监视值。
 * @note 只写监视字段，不修改完整拍快照或 FSM 历史。
 *     完整契约见 tracking.c::Tracking_SetTrackOutput。
 */
void Tracking_SetTrackOutput(int16_t value);

/**
 * @brief 读取电磁主状态监视值。
 * @note 完整契约见 tracking.c::Tracking_GetKernelState。
 */
uint8_t Tracking_GetKernelState(void);

/**
 * @brief 写入电磁主状态监视值。
 * @note 只写监视字段，不修改完整拍快照或 FSM 历史。
 *     完整契约见 tracking.c::Tracking_SetKernelState。
 */
void Tracking_SetKernelState(uint8_t value);

/**
 * @brief 读取旧路障标志监视值。
 * @note 完整契约见 tracking.c::Tracking_GetCaskFlag。
 */
uint8_t Tracking_GetCaskFlag(void);

/**
 * @brief 写入旧路障标志监视值。
 * @note 只写监视字段，不修改完整拍快照或 FSM 历史。
 *     完整契约见 tracking.c::Tracking_SetCaskFlag。
 */
void Tracking_SetCaskFlag(uint8_t value);

/**
 * @brief 读取环岛阶段监视值。
 * @note 完整契约见 tracking.c::Tracking_GetRoundaboutState。
 */
uint8_t Tracking_GetRoundaboutState(void);

/**
 * @brief 写入环岛阶段监视值。
 * @note 只写监视字段，不修改完整拍快照或 FSM 历史。
 *     完整契约见 tracking.c::Tracking_SetRoundaboutState。
 */
void Tracking_SetRoundaboutState(uint8_t value);

#endif
