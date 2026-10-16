// ============================================================
// 文件名: control.h
// 功能说明: 控制策略模块头文件
// 声明主状态机 kernel_state 宏与主控制函数。
// 源工程测试函数（roundabout_test/track_test/speed_test/
// speed_test2/gyro_test/adc_test）未移植。
// ============================================================

#ifndef _CONTROL_H_
#define _CONTROL_H_

#include <stdint.h>
#include "track_sensor.h"
#include "pid.h"

#define KERNEL_TRACKING     0
#define KERNEL_ISLAND_L     1
#define KERNEL_ISLAND_R     2
#define KERNEL_TEETERBOARD  3
#define KERNEL_CROSSROADS   4
#define KERNEL_CASK         5
#define KERNEL_REISLAND     6

extern int16_t track_out;   // 方向控制输出
extern uint8_t kernel_state;
extern uint8_t cask_flag;

#define CONTROL_PROFILE_WEIGHTS  (1u << 0)
#define CONTROL_PROFILE_BASE     (1u << 1)
#define CONTROL_PROFILE_TRACK    (1u << 2)
#define CONTROL_PROFILE_ANGLE    (1u << 3)

typedef struct {
    uint8_t fields;
    track_weights_t weights;
    int16_t base_speed;
    pid_track_gains_t track;
    pid_angle_gains_t angle;
} control_profile_t;

void Control_ApplyProfile(const control_profile_t *profile);
const control_profile_t *Control_GetKernelProfile(uint8_t state);
void Control_MergeProfile(control_profile_t *context, const control_profile_t *profile);

typedef enum { TRACK_MODE_EM=0, TRACK_MODE_OPTICAL=1 } tracking_mode_t;
#define PROTECT_EM_LOST      (1u<<0)
#define PROTECT_GRAY_LOST    (1u<<1)
#define PROTECT_CALIBRATION  (1u<<2)
#define PROTECT_ACQUIRE      (1u<<3)
#define PROTECT_OVERRUN      (1u<<4)
#define PROTECT_LOG_OVERFLOW (1u<<5)
#define PROTECT_LOG_WRITE    (1u<<6)
#define PROTECT_LOG_FULL     (1u<<7)
#define PROTECT_LOW_BATTERY  (1u<<8)
typedef enum { TRACKING_OK=0, TRACKING_BUSY, TRACKING_SAVE_FAILED, TRACKING_ARG } tracking_result_t;
typedef struct {
    uint8_t mode, gear;
    int16_t active_error, track_out;
    uint32_t control_sequence;
    uint16_t protection_flags;
} tracking_state_t;
const tracking_state_t *Tracking_GetState(void);
tracking_result_t Tracking_TrySetMode(uint8_t mode);
void Tracking_SetGear(uint8_t gear);
void Tracking_AddProtection(uint16_t flags);
void Tracking_Observe(uint8_t running, uint8_t stop_reason);
void Tracking_Execute(void);
void Tracking_AccumulateMotion(void);
void Tracking_CompleteControl(void);
void Tracking_Stopped(uint8_t reason);

void whole_test(void);      // 主控制状态机（2ms周期调用）
void Control_ResetRunState(void); // 再次起跑前复位主状态机与方向输出

#endif
