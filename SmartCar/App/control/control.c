/**
 * @file control.c
 * @brief App 双模式观察与唯一控制执行口；TIM6 调用，旧整体入口保留。
 * @details 汇合电磁与光电观察结果，选择当前模式的控制剖面并执行方向、速度与保护链；由 TIM6 控制拍驱动。
 * @note 移植来源：Sirius20260718/project/code/control.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include "app.h"
#include "control/control.h"
#include "control/motor.h"
#include "sensing/track_sensor.h"
#include "control/pid.h"
#include "control/element.h"
#include "control/roundabout.h"
#include "sensing/gray_sensor.h"
#include "control/optical_fsm.h"
#include "control/electromagnetic_fsm.h"
#include "sensing/imu_proc.h"
#include "services/param.h"
#include "control/tuning.h"

/* 共享监视量由既有控制/复位路径写入；双模式观察使用各自私有上下文，只有选中模式执行。 */
/* 方向控制输出（由循迹PID计算） */
int16_t g_control_track_output = 0;
uint8_t g_control_kernel_state = CONTROL_KERNEL_TRACKING;
uint8_t g_control_cask_flag = 0;

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

/* 前台选择模式/档位，TIM6 发布拍计数和输出；保护位仅在既有临界区累加。 */
static tracking_state_t s_tracking = { .mode = TRACKING_MODE_OPTICAL };

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
 * @brief 读取双模式调度状态。
 * @return 非空模块静态对象的只读指针，生命周期覆盖整个运行期。
 * @note 状态由既有控制/停车路径更新；前台要求一致快照时须在对应更新停止后读取。
 */
const tracking_state_t *Tracking_GetState(void)
{
    return &s_tracking;
}

/**
 * @brief 在停车和待写参数保存成功后切换循迹模式。
 * @details 按现有调用顺序使用
 *     App_IsStopped、Param_TryFlushPending、BSP_UART_WriteString、Control_ResetRunState
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] mode 0 电磁、1 光电；其他值返回 ARG。
 * @return OK 成功；ARG 模式越界；BUSY 未停车；SAVE_FAILED 参数保存失败。
 * @note 前台调用；失败路径打印及副作用、成功后的各模块复位顺序保持。
 */
tracking_result_t Tracking_TrySetMode(uint8_t mode)
{
    /* 先校验模式编号和停车条件；允许切换时更新模式、复位两套观察历史并重新应用当前档位参数。 */
    if (mode > 1)
    {
        return TRACKING_ARG;
    }
    if (!App_IsStopped())
    {
        return TRACKING_BUSY;
    }
    if (!Param_TryFlushPending())
    {
        BSP_UART_WriteString("track mode save failed\r\n");
        return TRACKING_SAVE_FAILED;
    }
    s_tracking.mode = mode;
    Control_ResetRunState();
    Motor_ResetRunState();
    PID_ResetAll();
    IMU_ResetRunState();
    Param_ApplyCurrentDirection();
    BSP_UART_WriteString(
        mode == TRACKING_MODE_OPTICAL ? "track mode: optical\r\n" : "track mode: em\r\n");
    return TRACKING_OK;
}

/**
 * @brief 在有效范围内更新所选档位。
 * @details 把非零输入归一为高档、零归一为低档，更新当前档位后立即重新应用该档参数；不改变当前循迹模式。
 * @param[in] gear 0 低档、1 高档；其他值保持原档位。
 * @note 停车菜单或参数加载路径调用；更新 s_tracking.gear，并由参数模块重载方向与基础速度。
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
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @note TIM6 完成控制后调用，uint32_t 自然回绕。
 */
void Tracking_CompleteControl(void)
{
    ++s_tracking.control_sequence;
}

/**
 * @brief 以停车原因推进光电状态并清方向输出。
 * @details 按现有调用顺序使用 Gray_GetState、OpticalFSM_Update 完成本函数处理。
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
            g_app_control_ticks,
            g->error,
            0,
            0,
            { 0 }
        };
    OpticalFSM_Update(&in);
    g_control_track_output = 0;
    s_tracking.track_out = 0;
}

/**
 * @brief 在原临界区内累计保护位。
 * @details 按现有调用顺序使用 BSP_Time_EnterCritical、BSP_Time_ExitCritical 完成本函数处理。
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
 * @brief 读取原七个主状态对应的部分配置。
 * @details 返回电磁状态机持有的控制剖面地址，供观察合并过程只读使用；不复制数据，也不改变状态机。
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

/**
 * @brief 用同一传感器帧分别推进电磁与光电观察状态。
 * @details 按现有调用顺序使用 Track_GetState、Gray_GetState、Param_GetDirection、IMU_GetState 完成本函数处理。
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
            g_app_dl1b_distance_mm,
            IMU_GetState()->accel[0],
            IMU_GetAngleDeltaX(),
            (uint8_t)(running && g_app_control_ticks > APP_RUN_DELAY_COUNT)
        };
    const optical_inputs_t opt =
    {
        running,
        g->line_valid,
        (uint8_t)(t->filtered[0] + t->filtered[1] + t->filtered[2] + t->filtered[3] > 300),
        stop_reason,
        g_app_control_ticks,
        g->error,
        p->base_speed,
        IMU_GetState()->accel[0],
        { p->kp, p->k2p, p->kd, 1.0f }
    };
    const electromagnetic_state_t *e;
    scb_values_t values;
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
    Tuning_SelectControl(e->kernel_state == CONTROL_KERNEL_TRACKING
        && e->roundabout_state == ROUNDABOUT_STATE_NORMAL, em.enabled, &gains,
        s_tracking.mode == TRACKING_MODE_OPTICAL ? opt.base_speed
        : (em.enabled ? e->configuration.base_speed : Motor_GetState()->base_speed));
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
    g_control_kernel_state = e->kernel_state;
    g_roundabout_state = e->roundabout_state;
    s_tracking.active_error = (s_tracking.mode == TRACKING_MODE_OPTICAL) ? g->error : e->error;
}

/**
 * @brief 仅把所选模式观察结果应用到共享控制执行口。
 * @details 按现有调用顺序使用
 *     OpticalFSM_GetState、Motor_SetBaseSpeed、Motor_DistributeSpeed、ElectromagneticFSM_GetState
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
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
        g_control_track_output = o->output;
        Motor_DistributeSpeed(g_control_track_output);
    }
    else
    {
        const electromagnetic_state_t *e = ElectromagneticFSM_GetState();
        scb_values_t values;
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
        g_control_track_output = e->output;
    }
    s_tracking.track_out = g_control_track_output;
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
 * @brief 保留完整双传感器检查与控制的旧封装入口。
 * @details 按现有调用顺序使用 Track_ReadAdc、Gray_Update、Gray_GetState、Track_GetState
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 当前 App 调度使用分步接口；本函数调用关系保持，不新增接入或合并错误分支。
 */
void Control_UpdateLegacy(void)
{
    /* 保留入口先刷新双传感器并累计全部保护位；存在故障时观察停车并请求首因，否则依次观察、置运行标志和执行控制。 */
    uint16_t flags = 0;
    Track_ReadAdc();
    Gray_Update();
    if (Gray_GetState()->acquisition_status)
    {
        flags |= TRACKING_PROTECT_ACQUIRE;
    }
    if (!Gray_GetState()->calibration_valid)
    {
        flags |= TRACKING_PROTECT_CALIBRATION;
    }
    if (!Gray_GetState()->line_valid)
    {
        flags |= TRACKING_PROTECT_GRAY_LOST;
    }
    if (Track_GetState()->filtered[0] + Track_GetState()->filtered[1]
            + Track_GetState()->filtered[2] + Track_GetState()->filtered[3]
        <= 300)
    {
        flags |= TRACKING_PROTECT_EM_LOST;
    }
    Tracking_AddProtection(flags);
    if (flags)
    {
        app_stop_reason_t reason
            = (flags & (TRACKING_PROTECT_ACQUIRE | TRACKING_PROTECT_CALIBRATION))
            ? APP_STOP_GRAY_ACQUIRE
            : ((flags & TRACKING_PROTECT_GRAY_LOST) ? APP_STOP_GRAY_LOST : APP_STOP_OFF_TRACK);
        Tracking_Observe(0, (uint8_t)reason);
        App_RequestStop(reason);
        return;
    }
    Tracking_Observe(1, 0);
    g_app_run_flag = 1;
    Tracking_Execute();
}

/**
 * @brief 复位双观察状态与共享监视量，保持模式和档位。
 * @details 按现有调用顺序使用 ElectromagneticFSM_Reset、OpticalFSM_Reset 完成本函数处理。
 * @note 起跑/模式切换调用；控制参数、传感器标定及既有复位顺序不变。
 */
void Control_ResetRunState(void)
{
    g_control_kernel_state = CONTROL_KERNEL_TRACKING;
    g_control_cask_flag = 0;
    g_control_track_output = 0;
    s_tracking.active_error = 0;
    s_tracking.track_out = 0;
    s_tracking.control_sequence = 0;
    s_tracking.protection_flags = 0;
    ElectromagneticFSM_Reset();
    OpticalFSM_Reset();
}
