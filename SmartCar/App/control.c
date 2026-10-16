// ============================================================
// 文件名: control.c
// 功能说明: 控制策略模块
// whole_test() 主状态机：读四路电感→出赛道保护→按状态设权重/PID/
// 基础速度→循迹偏差→方向PID→差速→元素进出判断。
// ============================================================

#include "bsp.h"
#include "app_config.h"
#include "app.h"
#include "control.h"
#include "motor.h"
#include "track_sensor.h"
#include "pid.h"
#include "element.h"
#include "roundabout.h"
#include "gray_sensor.h"
#include "optical_fsm.h"
#include "electromagnetic_fsm.h"
#include "imu_proc.h"
#include "param.h"

int16_t track_out = 0;          // 方向控制输出（由循迹PID计算）
uint8_t kernel_state = KERNEL_TRACKING;
uint8_t cask_flag = 0;

/* 只覆盖旧分支实际赋值的字段；未出现的字段沿用上一拍。 */
static const control_profile_t s_kernel_profiles[7] = {
    [KERNEL_TRACKING] = {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        {15, 20, 22, 10}, 245, {2.0f, 0.008f, 15.0f, 1.0f}, {0}
    },
    [KERNEL_ISLAND_L] = {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_ANGLE,
        {15, 20, 22, 10}, 0, {0}, {2.0f, 1.2f, 0.0f}
    },
    [KERNEL_ISLAND_R] = {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_ANGLE,
        {15, 20, 22, 10}, 150, {0}, {2.0f, 1.2f, 0.0f}
    },
    [KERNEL_TEETERBOARD] = {
        CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        {0}, 100, {4.5f, 0.0f, 6.2f, 0.9f}, {0}
    },
    [KERNEL_CROSSROADS] = {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        {15, 20, 2, 10}, 240, {2.0f, 0.008f, 15.0f, 1.0f}, {0}
    },
    [KERNEL_CASK] = {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        {15, 20, 2, 5}, 210, {2.0f, 0.008f, 15.0f, 1.0f}, {0}
    },
    [KERNEL_REISLAND] = {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        {20, 10, 1, 10}, 180, {2.0f, 0.008f, 15.0f, 1.0f}, {0}
    }
};

void Control_ApplyProfile(const control_profile_t *profile)
{
    if(profile->fields & CONTROL_PROFILE_WEIGHTS) Track_SetWeights(&profile->weights);
    if(profile->fields & CONTROL_PROFILE_BASE) Motor_SetBaseSpeed(profile->base_speed);
    if(profile->fields & CONTROL_PROFILE_TRACK) PID_SetTrackGains(&profile->track);
    if(profile->fields & CONTROL_PROFILE_ANGLE) PID_SetAngleGains(&profile->angle);
}

static tracking_state_t s_tracking = {.mode=TRACK_MODE_OPTICAL};
const tracking_state_t *Tracking_GetState(void) { return &s_tracking; }
tracking_result_t Tracking_TrySetMode(uint8_t mode)
{
    if(mode>1) return TRACKING_ARG;
    if(!App_IsStopped()) return TRACKING_BUSY;
    if(!Param_TryFlushPending())
    { BSP_UART_WriteString("track mode save failed\r\n"); return TRACKING_SAVE_FAILED; }
    s_tracking.mode=mode;
    Control_ResetRunState(); Motor_ResetRunState(); PID_ResetAll(); IMU_ResetRunState();
    Param_ApplyCurrentDirection();
    BSP_UART_WriteString(mode==TRACK_MODE_OPTICAL ? "track mode: optical\r\n" : "track mode: em\r\n");
    return TRACKING_OK;
}
void Tracking_SetGear(uint8_t gear) { if(gear<2) s_tracking.gear=gear; }
void Tracking_CompleteControl(void) { ++s_tracking.control_sequence; }
void Tracking_Stopped(uint8_t reason)
{
    const gray_state_t *g=Gray_GetState();
    const optical_inputs_t in={0,(uint8_t)!(s_tracking.protection_flags&PROTECT_GRAY_LOST),
        0,reason,time,g->error,0,0,{0}};
    OpticalFSM_Update(&in);
    track_out=0;s_tracking.track_out=0;
}
void Tracking_AddProtection(uint16_t flags)
{
    uint32_t mask=BSP_Time_EnterCritical();
    s_tracking.protection_flags |= flags;
    BSP_Time_ExitCritical(mask);
}
const control_profile_t *Control_GetKernelProfile(uint8_t state)
{ return &s_kernel_profiles[state<7 ? state : KERNEL_TRACKING]; }
void Control_MergeProfile(control_profile_t *dst, const control_profile_t *src)
{
    if(src->fields & CONTROL_PROFILE_WEIGHTS) dst->weights=src->weights;
    if(src->fields & CONTROL_PROFILE_BASE) dst->base_speed=src->base_speed;
    if(src->fields & CONTROL_PROFILE_TRACK) dst->track=src->track;
    if(src->fields & CONTROL_PROFILE_ANGLE) dst->angle=src->angle;
    dst->fields |= src->fields;
}
void Tracking_Observe(uint8_t running, uint8_t stop_reason)
{
    const track_state_t *t=Track_GetState();
    const gray_state_t *g=Gray_GetState();
    const param_direction_t *p=Param_GetDirection(TRACK_MODE_OPTICAL,s_tracking.gear);
    const electromagnetic_inputs_t em={t->filtered,dl1b_distance_mm,
        IMU_GetState()->accel[0],IMU_GetAngleDeltaX(),(uint8_t)(running && time>RUN_DELAY_COUNT)};
    const optical_inputs_t opt={running,g->line_valid,
        (uint8_t)(t->filtered[0]+t->filtered[1]+t->filtered[2]+t->filtered[3]>300),
        stop_reason,time,g->error,p->base_speed,IMU_GetState()->accel[0],{p->kp,p->k2p,p->kd,1.0f}};
    const electromagnetic_state_t *e;
    ElectromagneticFSM_Update(&em);
    OpticalFSM_Update(&opt);
    e=ElectromagneticFSM_GetState();
    Track_PublishObservation(&e->configuration.weights,e->error,e->symmetry_x,e->symmetry_y);
    /* 兼容旧监视字段；所有活动判断仅写私有电磁上下文。 */
    kernel_state=e->kernel_state; roundabout_state=e->roundabout_state;
    s_tracking.active_error=(s_tracking.mode==TRACK_MODE_OPTICAL) ? g->error : e->error;
}
void Tracking_Execute(void)
{
    if(s_tracking.mode==TRACK_MODE_OPTICAL)
    {
        const optical_state_t *o=OpticalFSM_GetState();
        if(o->state!=OPTICAL_TRACKING) return;
        Motor_SetBaseSpeed(o->base_speed);
        track_out=o->output;
        speed_control(track_out);
    }
    else
    {
        const electromagnetic_state_t *e=ElectromagneticFSM_GetState();
        Control_ApplyProfile(&e->configuration);
        speed_control(e->steering);
        if(e->reset_distance) Motor_ResetDistance();
        if(e->zero_angle) IMU_ZeroAngleX();
        track_out=e->output;
    }
    s_tracking.track_out=track_out;
}
void Tracking_AccumulateMotion(void)
{
    ElectromagneticFSM_Accumulate(Motor_GetState()->left.real_speed,Motor_GetState()->right.real_speed);
}
void whole_test(void)
{
    uint16_t flags=0;
    read_adc(); Gray_Update();
    if(Gray_GetState()->acquisition_status) flags|=PROTECT_ACQUIRE;
    if(!Gray_GetState()->calibration_valid) flags|=PROTECT_CALIBRATION;
    if(!Gray_GetState()->line_valid) flags|=PROTECT_GRAY_LOST;
    if(Track_GetState()->filtered[0]+Track_GetState()->filtered[1]+
       Track_GetState()->filtered[2]+Track_GetState()->filtered[3]<=300) flags|=PROTECT_EM_LOST;
    Tracking_AddProtection(flags);
    if(flags)
    {
        app_stop_reason_t reason=(flags&(PROTECT_ACQUIRE|PROTECT_CALIBRATION)) ? APP_STOP_GRAY_ACQUIRE :
            ((flags&PROTECT_GRAY_LOST) ? APP_STOP_GRAY_LOST : APP_STOP_OFF_TRACK);
        Tracking_Observe(0,(uint8_t)reason); App_RequestStop(reason); return;
    }
    Tracking_Observe(1,0); Run_flag=1; Tracking_Execute();
}

// 函数名: Control_ResetRunState
// 功能: 再次起跑前复位主状态机，不修改任何控制参数
void Control_ResetRunState(void)
{
    kernel_state = KERNEL_TRACKING;
    cask_flag = 0;
    track_out = 0;
    s_tracking.active_error=0; s_tracking.track_out=0;
    s_tracking.control_sequence=0; s_tracking.protection_flags=0;
    ElectromagneticFSM_Reset(); OpticalFSM_Reset();
}
