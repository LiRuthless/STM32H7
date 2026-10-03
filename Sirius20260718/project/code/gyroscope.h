#ifndef _GYROSCOPE_H_
#define _GYROSCOPE_H_

extern float accel_x;       // X轴加速度值
extern float accel_y;       // Y轴加速度值
extern float accel_z;       // Z轴加速度值
extern float gyro_x;        // X轴陀螺仪角速度数据
extern float gyro_y;        // Y轴陀螺仪角速度数据
extern float gyro_z;        // Z轴陀螺仪角速度数据

extern float velocity_x;		// X轴速度积分值
extern float velocity_y;		// Y轴速度积分值
extern float velocity_z;		// Z轴速度积分值
extern float angle_x;			// X轴角度积分值
extern float angle_y;			// Y轴角度积分值
extern float angle_z;			// Z轴角度积分值



void read_accel_velocity(void);		// 读取加速度并积分得到速度
void read_gyro_angle(void);			// 读取陀螺仪并积分得到角度
void accel_calibrate(void);			// 加速度计静态零偏校准
void gyro_calibrate(void);			// 陀螺仪静态零偏校准


#endif
