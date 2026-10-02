#ifndef _IMU_PROC_H_
#define _IMU_PROC_H_

#include <stdint.h>

extern float accel_x;       // X轴加速度值
extern float accel_y;       // Y轴加速度值
extern float accel_z;       // Z轴加速度值
extern float gyro_x;        // X轴陀螺仪角速度数据
extern float gyro_y;        // Y轴陀螺仪角速度数据
extern float gyro_z;        // Z轴陀螺仪角速度数据

extern float velocity_x;    // X轴速度积分值
extern float velocity_y;    // Y轴速度积分值
extern float velocity_z;    // Z轴速度积分值
extern float angle_x;       // X轴角度积分值
extern float angle_y;       // Y轴角度积分值
extern float angle_z;       // Z轴角度积分值

// IMU660RB 原始数据（与逐飞库同名，供其它模块引用），由本模块读取函数填充
extern int16_t imu660rb_gyro_x;
extern int16_t imu660rb_gyro_y;
extern int16_t imu660rb_gyro_z;
extern int16_t imu660rb_acc_x;
extern int16_t imu660rb_acc_y;
extern int16_t imu660rb_acc_z;

void imu_proc_init(void);           // IMU初始化（含陀螺高通滤波器初始化），上电时调用
void read_accel_velocity(void);     // 读取加速度并积分得到速度（保留接口，默认不调用）
void read_gyro_angle(void);         // 读取陀螺仪并积分得到角度（2ms周期调用）
void accel_calibrate(void);         // 加速度计静态零偏校准（上电静止时调用）
void gyro_calibrate(void);          // 陀螺仪静态零偏校准（上电静止时调用）
void IMU_ResetRunState(void);       // 清积分量与梯形积分历史，不重做零偏校准

#endif
