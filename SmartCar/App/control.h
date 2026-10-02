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

void whole_test(void);      // 主控制状态机（2ms周期调用）
void Control_ResetRunState(void); // 再次起跑前复位主状态机与方向输出

#endif
