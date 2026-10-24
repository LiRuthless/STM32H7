/**
 * @file tracking.c
 * @brief 双模式观察与唯一执行协调。
 * @details 前台选择模式档位，TIM6 推进观察、选择输出并发布完整拍；保护累加使用原临界区。
 * @note 移植来源：Sirius20260718/project/code/control.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源文件未单列作者及独立许可证；保留出处，关联 Phase 13/15。
 */
#include "bsp.h"
#include "app_config.h"
#include "app.h"
#include "system/lifecycle.h"
#include "control/tracking.h"
#include "control/motor.h"
#include "sensing/track_sensor.h"
#include "control/pid.h"
#include "sensing/gray_sensor.h"
#include "control/optical_fsm.h"
#include "control/electromagnetic_fsm.h"
#include "sensing/imu_proc.h"
#include "services/parameter_bank.h"
#include "control/tuning.h"

/* 监视量由原发布点更新，与完整拍 track_out 生命周期不同。 */
static int16_t s_track_output;
static uint8_t s_kernel_state = CONTROL_KERNEL_TRACKING;
static uint8_t s_cask_flag;
static uint8_t s_roundabout_state = ROUNDABOUT_STATE_NORMAL;

/* 前台选择模式/档位，TIM6 发布拍计数和输出；保护位仅在既有临界区累加。 */
static tracking_state_t s_tracking = { .mode = TRACKING_MODE_OPTICAL };

/**
 * @brief 读取双模式调度状态。
 * @return 非空模块静态对象的只读指针，生命周期覆盖整个运行期。
 * @note 状态由既有控制/停车路径更新；前台要求一致快照时须在对应更新停止后读取。
 */
const tracking_state_t *Tracking_GetState(void)
{
    return &s_tracking;
}



/**
 * @brief 在有效范围内更新所选档位。
 * @details 仅在 gear 小于 2 时写所选档位，其他值无操作；不应用参数或改变模式。
 * @param[in] gear 0 低档、1 高档；其他值保持原档位。
 * @note 停车菜单或参数加载路径调用；只更新 s_tracking.gear，参数应用由原调用方协调。
 */
void Tracking_SetGear(uint8_t gear)
{
    if (gear < 2)
    {
        s_tracking.gear = gear;
    }
}

/**
 * @brief 将完整控制拍序号增加一次。
 * @details 只递增uint32_t完整控制拍序号一次，按原整数语义自然回绕；不更新其他状态。
 * @note TIM6 完成控制后调用，uint32_t 自然回绕。
 */
void Tracking_CompleteControl(void)
{
    ++s_tracking.control_sequence;
}

/**
 * @brief 以停车原因推进光电状态并清方向输出。
 * @details 用当前灰度帧及累计保护构造停止输入推进光电状态，再依次清方向监视值和拍输出。
 * @param[in] reason 原 app_stop_reason_t 对应的 uint8_t 原因值。
 * @note 保留灰度失线标志与首次原因的既有组合，不推进电磁执行。
 */
void Tracking_Stopped(uint8_t reason)
{
    const gray_state_t *g = Gray_GetState();
    const optical_inputs_t in
        =
        {
            0,
            (uint8_t)!(s_tracking.protection_flags & TRACKING_PROTECT_GRAY_LOST),
            0,
            reason,
            Lifecycle_GetControlTicks(),
            g->error,
            0,
            0,
            { 0 }
        };
    OpticalFSM_Update(&in);
    Tracking_SetTrackOutput(0);
    s_tracking.track_out = 0;
}

/**
 * @brief 在原临界区内累计保护位。
 * @details 在保存PRIMASK的原短临界区内按位或累计保护，退出恢复原中断屏蔽；不清既有位。
 * @param[in] flags TRACKING_PROTECT_* 位图，保持原 uint16_t 组合。
 * @note 前台或 ISR 可调用；恢复进入前 PRIMASK，不改变临界区边界。
 */
void Tracking_AddProtection(uint16_t flags)
{
    uint32_t mask = BSP_Time_EnterCritical();
    s_tracking.protection_flags |= flags;
    BSP_Time_ExitCritical(mask);
}

/**
 * @brief 用同一传感器帧分别推进电磁与光电观察状态。
 * @details 同帧构造两模式输入，先选择各自状态和本拍调参副本，再各计算一次方向；
 *     最后发布电感观察和选中偏差，两个FSM独立持有方向历史。
 * @param[in] running 零表示停车观察，非零表示已起跑；等待拍条件仍独立判断。
 * @param[in] stop_reason 原停车原因，零表示本次无停车原因。
 * @note TIM6 或停车路径调用；电磁只写私有上下文，发布监视字段后按所选模式取偏差。
 */
void Tracking_Observe(uint8_t running, uint8_t stop_reason)
{
    const track_state_t *t = Track_GetState();
    const gray_state_t *g = Gray_GetState();
    const param_direction_t *p = Param_GetDirection(TRACKING_MODE_OPTICAL, s_tracking.gear);
    const electromagnetic_inputs_t em
        =
        {
            t->filtered,
            Lifecycle_GetDistanceMm(),
            IMU_GetState()->accel[0],
            IMU_GetAngleDeltaX(),
            (uint8_t)(running && Lifecycle_GetControlTicks() > APP_RUN_DELAY_COUNT)
        };
    const optical_inputs_t opt =
    {
        running,
        g->line_valid,
        (uint8_t)(t->filtered[0] + t->filtered[1] + t->filtered[2] + t->filtered[3] > 300),
        stop_reason,
        Lifecycle_GetControlTicks(),
        g->error,
        p->base_speed,
        IMU_GetState()->accel[0],
        { p->kp, p->k2p, p->kd, 1.0f }
    };
    const electromagnetic_state_t *e;
    tuning_values_t values;
    pid_track_gains_t gains;
    uint8_t overridden;
    /* 先确定新元素状态，再选择本拍增益，最后各算一次方向；不得用二次观察回滚历史。 */
    ElectromagneticFSM_Select(&em);
    OpticalFSM_Select(&opt);
    e = ElectromagneticFSM_GetState();
    gains = s_tracking.mode == TRACKING_MODE_OPTICAL ? opt.gains : e->configuration.track;
    if (!em.enabled && s_tracking.mode == TRACKING_MODE_EM)
    {
        const pid_state_t *active = PID_GetState();
        gains = (pid_track_gains_t){ active->kp_x, active->k2p_x, active->kd_x, active->k2d_x };
    }
    tuning_context_t tuning_context;
    tuning_context = (tuning_context_t)
    {
        0, Lifecycle_IsStarted(), s_tracking.mode, s_tracking.gear,
        (uint8_t)(e->kernel_state == CONTROL_KERNEL_TRACKING
            && e->roundabout_state == ROUNDABOUT_STATE_NORMAL),
        s_tracking.protection_flags, (uint32_t)Lifecycle_GetControlTicks(),
        0, s_tracking.control_sequence
    };
    Tuning_SelectControl(e->kernel_state == CONTROL_KERNEL_TRACKING
        && e->roundabout_state == ROUNDABOUT_STATE_NORMAL, em.enabled, &gains,
        s_tracking.mode == TRACKING_MODE_OPTICAL ? opt.base_speed
        : (em.enabled ? e->configuration.base_speed : Motor_GetState()->base_speed),
        &tuning_context);
    overridden = Tuning_GetControl(&values);
    gains = (pid_track_gains_t){ values.kp, values.k2p, values.kd, 1.0f };
    ElectromagneticFSM_Calculate(&em,
        overridden && s_tracking.mode == TRACKING_MODE_EM ? &gains : 0);
    OpticalFSM_Calculate(&opt,
        overridden && s_tracking.mode == TRACKING_MODE_OPTICAL ? &gains : 0,
        overridden && s_tracking.mode == TRACKING_MODE_OPTICAL
        ? (int16_t)values.base_speed : opt.base_speed);
    Track_PublishObservation(&e->configuration.weights, e->error, e->symmetry_x, e->symmetry_y);
    /* 兼容旧监视字段；所有活动判断仅写私有电磁上下文。 */
    Tracking_SetKernelState(e->kernel_state);
    Tracking_SetRoundaboutState(e->roundabout_state);
    s_tracking.active_error = (s_tracking.mode == TRACKING_MODE_OPTICAL) ? g->error : e->error;
}

/**
 * @brief 仅把所选模式观察结果应用到共享控制执行口。
 * @details 光电只有TRACKING态应用基础速度及方向；电磁先将调参副本覆盖到局部配置，
 *     再应用增益/基础速度并执行差速、里程和角度复位，最后发布选中方向输出。
 * @note TIM6 调用；光电仅 TRACKING 输出，电磁保留转向、里程/角度复位及最终输出顺序。
 */
void Tracking_Execute(void)
{
    /* 光电模式只在 TRACKING 状态发布基础速度和转向；电磁模式先应用剖面和转向，再处理里程/角度复位并发布最终输出。 */
    if (s_tracking.mode == TRACKING_MODE_OPTICAL)
    {
        const optical_state_t *o = OpticalFSM_GetState();
        if (o->state != OPTICAL_TRACKING)
        {
            return;
        }
        Motor_SetBaseSpeed(o->base_speed);
        Tracking_SetTrackOutput(o->output);
        Motor_DistributeSpeed(Tracking_GetTrackOutput());
    }
    else
    {
        const electromagnetic_state_t *e = ElectromagneticFSM_GetState();
        tuning_values_t values;
        control_profile_t execution = e->configuration;
        if (Tuning_GetControl(&values))
        {
            execution.track = (pid_track_gains_t){ values.kp, values.k2p, values.kd, 1.0f };
            execution.base_speed = (int16_t)values.base_speed;
            execution.fields |= CONTROL_PROFILE_TRACK | CONTROL_PROFILE_BASE;
        }
        Control_ApplyProfile(&execution);
        Motor_DistributeSpeed(e->steering);
        if (e->reset_distance)
        {
            Motor_ResetDistance();
        }
        if (e->zero_angle)
        {
            IMU_ZeroAngleX();
        }
        Tracking_SetTrackOutput(e->output);
    }
    s_tracking.track_out = Tracking_GetTrackOutput();
}

/**
 * @brief 将最新滤波轮速分别累加到电磁私有里程。
 * @note TIM6 编码器更新后调用，不复位共享运动基准。
 */
void Tracking_AccumulateMotion(void)
{
    ElectromagneticFSM_Accumulate(
        Motor_GetState()->left.real_speed, Motor_GetState()->right.real_speed);
}

/**
 * @brief 复位双观察状态与共享监视量，保持模式和档位。
 * @details 先复位旧监视字段，再清选中偏差、输出、完整序号和累计保护，
 *     最后依次复位电磁/光电私有状态及方向历史；模式和档位不清。
 * @note 起跑/模式切换调用；控制参数、传感器标定及既有复位顺序不变。
 */
void Control_ResetRunState(void)
{
    Tracking_SetKernelState(CONTROL_KERNEL_TRACKING);
    Tracking_SetCaskFlag(0);
    Tracking_SetTrackOutput(0);
    s_tracking.active_error = 0;
    s_tracking.track_out = 0;
    s_tracking.control_sequence = 0;
    s_tracking.protection_flags = 0;
    ElectromagneticFSM_Reset();
    OpticalFSM_Reset();
}

/**
 * @brief 读取方向输出监视值。
 * @return 当前标量，不保证与其他字段组成原子快照。
 */
int16_t Tracking_GetTrackOutput(void)
{
    return s_track_output;
}

/**
 * @brief 写入方向输出监视值。
 * @param[in] value 原标量，不作范围归一化。
 * @note 只写监视字段，不修改完整拍快照或 FSM 历史。
 */
void Tracking_SetTrackOutput(int16_t value)
{
    s_track_output = value;
}

/**
 * @brief 读取电磁主状态监视值。
 * @return 当前标量，不保证与其他字段组成原子快照。
 */
uint8_t Tracking_GetKernelState(void)
{
    return s_kernel_state;
}

/**
 * @brief 写入电磁主状态监视值。
 * @param[in] value 原标量，不作范围归一化。
 * @note 只写监视字段，不修改完整拍快照或 FSM 历史。
 */
void Tracking_SetKernelState(uint8_t value)
{
    s_kernel_state = value;
}

/**
 * @brief 读取旧路障标志监视值。
 * @return 当前标量，不保证与其他字段组成原子快照。
 */
uint8_t Tracking_GetCaskFlag(void)
{
    return s_cask_flag;
}

/**
 * @brief 写入旧路障标志监视值。
 * @param[in] value 原标量，不作范围归一化。
 * @note 只写监视字段，不修改完整拍快照或 FSM 历史。
 */
void Tracking_SetCaskFlag(uint8_t value)
{
    s_cask_flag = value;
}

/**
 * @brief 读取环岛阶段监视值。
 * @return 当前标量，不保证与其他字段组成原子快照。
 */
uint8_t Tracking_GetRoundaboutState(void)
{
    return s_roundabout_state;
}

/**
 * @brief 写入环岛阶段监视值。
 * @param[in] value 原标量，不作范围归一化。
 * @note 只写监视字段，不修改完整拍快照或 FSM 历史。
 */
void Tracking_SetRoundaboutState(uint8_t value)
{
    s_roundabout_state = value;
}
/**
 * @brief 写入已校验的模式编号。
 * @details 只更新模式，不校验、保存、应用参数或清历史。
 * @param[in] mode 生命周期已校验的0或1。
 * @note 仅Lifecycle_TrySetMode调用。
 */
void Tracking_SetMode(uint8_t mode)
{
    s_tracking.mode = mode;
}
