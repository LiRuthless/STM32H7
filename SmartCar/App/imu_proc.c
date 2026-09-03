// ============================================================
// 文件名: imu_proc.c  （移植自源工程 gyroscope.c）
// 功能说明: IMU660RB 陀螺仪/加速度计数据处理
// 读取原始数据、去零偏、单位换算，并梯形积分得到角度/速度。
// 全局变量名与源工程保持一致，供其它模块直接引用。
// ============================================================

#include "bsp.h"
#include "app_config.h"
#include "imu_proc.h"
#include "filter.h"

float accel_x = 0;      // X轴加速度
float accel_y = 0;      // Y轴加速度
float accel_z = 0;      // Z轴加速度
float gyro_x = 0;       // X轴陀螺仪角速度
float gyro_y = 0;       // Y轴陀螺仪角速度
float gyro_z = 0;       // Z轴陀螺仪角速度

float velocity_x = 0;    // X轴速度积分值
float velocity_y = 0;    // Y轴速度积分值
float velocity_z = 0;    // Z轴速度积分值
float angle_x = 0;       // X轴角度积分值
float angle_y = 0;       // Y轴角度积分值
float angle_z = 0;       // Z轴角度积分值

// IMU660RB 原始数据（与逐飞库同名），由读取函数通过 BSP 填充
int16_t imu660rb_gyro_x = 0;
int16_t imu660rb_gyro_y = 0;
int16_t imu660rb_gyro_z = 0;
int16_t imu660rb_acc_x = 0;
int16_t imu660rb_acc_y = 0;
int16_t imu660rb_acc_z = 0;

static float accel_offset_x = 0;	 // X轴加速度零偏
static float accel_offset_y = 0;	 // Y轴加速度零偏
static float accel_offset_z = 0;	 // Z轴加速度零偏
static float gyro_offset_x = 0;	     // X轴陀螺仪零偏
static float gyro_offset_y = 0;	     // Y轴陀螺仪零偏
static float gyro_offset_z = 0;	     // Z轴陀螺仪零偏


// 函数名: imu_proc_init
// 功能: IMU模块初始化
// 说明: 初始化 IMU660RB 硬件（SPI），并初始化 x/y 轴陀螺仪高通滤波器
//       （源工程在 config.c 中 gyro_hpf_init 0.2Hz/0.002，此处收拢）。
//       上电时调用一次，之后调用 gyro_calibrate()/accel_calibrate() 校准零偏。
void imu_proc_init(void)
{
    BSP_IMU660RB_Init();
    gyro_hpf_init(&gyro_hpf_x, 0.2f, CTRL_DT_S);
    gyro_hpf_init(&gyro_hpf_y, 0.2f, CTRL_DT_S);
}


// 函数名: read_accel_velocity
// 功能: 读取加速度并积分得到速度
// 说明: 照搬源工程实现（含源工程中读取陀螺仪原始值、除以14.3、
//       积分项使用 gyro_x/gyro_y/gyro_z 的原始写法），保持语义一致。
//       本工程默认不调用，仅保留接口备用。
void read_accel_velocity(void)
{
	static float accel_last_x = 0;
	static float accel_last_y = 0;
	static float accel_last_z = 0;

	BSP_IMU660RB_GetGyro(&imu660rb_gyro_x, &imu660rb_gyro_y, &imu660rb_gyro_z);

	accel_x = ((float)imu660rb_acc_x - accel_offset_x) / GYRO_RAW_TO_DPS;
	accel_y = ((float)imu660rb_acc_y - accel_offset_y) / GYRO_RAW_TO_DPS;
	accel_z = ((float)imu660rb_acc_z - accel_offset_z) / GYRO_RAW_TO_DPS;

	velocity_x += (gyro_x + accel_last_x) * 0.001f;		//0.5 * 0.002
	velocity_y += (gyro_y + accel_last_y) * 0.001f;
	velocity_z += (gyro_z + accel_last_z) * 0.001f;

	accel_last_x = accel_x;
	accel_last_y = accel_y;
	accel_last_z = accel_z;
}


// 函数名: read_gyro_angle
// 功能: 读取陀螺仪并积分得到角度（2ms 周期调用）
// 说明: 获取IMU角速度数据，去零偏后除以14.3换算为°/s，
//       y轴经0.2Hz高通滤波，最后梯形积分得到各轴角度（系数0.001 = 0.5*0.002）。
void read_gyro_angle(void)
{
	static float gyro_last_x = 0;
	static float gyro_last_y = 0;
	static float gyro_last_z = 0;

	BSP_IMU660RB_GetGyro(&imu660rb_gyro_x, &imu660rb_gyro_y, &imu660rb_gyro_z);

	gyro_x = ((float)imu660rb_gyro_x - gyro_offset_x) / GYRO_RAW_TO_DPS;
	gyro_y = ((float)imu660rb_gyro_y - gyro_offset_y) / GYRO_RAW_TO_DPS;
	gyro_z = ((float)imu660rb_gyro_z - gyro_offset_z) / GYRO_RAW_TO_DPS;

	gyro_y = gyro_hpf_update(&gyro_hpf_y, gyro_y);		// y轴角速度高通滤波

	angle_x += (gyro_x + gyro_last_x) * 0.001f;		//0.5 * 0.002
	angle_y += (gyro_y + gyro_last_y) * 0.001f;
	angle_z += (gyro_z + gyro_last_z) * 0.001f;

	gyro_last_x = gyro_x;
	gyro_last_y = gyro_y;
	gyro_last_z = gyro_z;
}


// 函数名: accel_calibrate
// 功能: 加速度计静态零偏校准
// 说明: 上电静止时调用，采样100次取平均得到加速度零偏，并清零速度积分。
void accel_calibrate(void)
{
	const int8_t samples = 100;
	int32_t temp[3] = {0}; 		// 临时计算变量
	int8_t i = 0; 				// 循环计数器

	for(i = 0; i < samples; i++) {
		BSP_IMU660RB_GetAcc(&imu660rb_acc_x, &imu660rb_acc_y, &imu660rb_acc_z);
		temp[0] += imu660rb_acc_x;
        temp[1] += imu660rb_acc_y;
        temp[2] += imu660rb_acc_z;
    }

    accel_offset_x = (float)temp[0] / (float)samples;
    accel_offset_y = (float)temp[1] / (float)samples;
    accel_offset_z = (float)temp[2] / (float)samples;

	velocity_x = 0,	velocity_y = 0,	velocity_z = 0;
}


// 函数名: gyro_calibrate
// 功能: 陀螺仪静态零偏校准
// 说明: 上电静止时调用，采样100次取平均得到陀螺仪零偏，并清零角度积分。
void gyro_calibrate(void)
{
	const int8_t samples = 100;
	int32_t temp[3] = {0}; 		// 临时计算变量
	int8_t i = 0; 				// 循环计数器

	for(i = 0; i < samples; i++) {
		BSP_IMU660RB_GetGyro(&imu660rb_gyro_x, &imu660rb_gyro_y, &imu660rb_gyro_z);
		temp[0] += imu660rb_gyro_x;
        temp[1] += imu660rb_gyro_y;
        temp[2] += imu660rb_gyro_z;
    }

    gyro_offset_x = (float)temp[0] / (float)samples;
    gyro_offset_y = (float)temp[1] / (float)samples;
    gyro_offset_z = (float)temp[2] / (float)samples;

	angle_x = 0, angle_y = 0, angle_z = 0;
}
