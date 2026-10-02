#ifndef _IMU_PROC_H_
#define _IMU_PROC_H_

#include <stdint.h>

typedef struct {
    float accel[3];       // x/y/z
    float gyro[3];
    float velocity[3];
    float angle[3];
    int16_t raw_gyro[3];
    int16_t raw_accel[3];
} imu_state_t;

const imu_state_t *IMU_GetState(void);
void IMU_ZeroAngleX(void);

void imu_proc_init(void);           // IMU初始化（含陀螺高通滤波器初始化），上电时调用
void read_accel_velocity(void);     // 读取加速度并积分得到速度（保留接口，默认不调用）
void read_gyro_angle(void);         // 读取陀螺仪并积分得到角度（2ms周期调用）
void accel_calibrate(void);         // 加速度计静态零偏校准（上电静止时调用）
void gyro_calibrate(void);          // 陀螺仪静态零偏校准（上电静止时调用）
void IMU_ResetRunState(void);       // 清积分量与梯形积分历史，不重做零偏校准

#endif
