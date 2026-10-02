// ============================================================
// 文件名: imu_proc.c  （移植自源工程 gyroscope.c）
// 功能说明: IMU660RB 陀螺仪/加速度计数据处理
// 读取原始数据、去零偏、单位换算，并梯形积分得到角度/速度。
// 运行状态由本模块持有，其它模块经只读接口访问。
// ============================================================

#include "bsp.h"
#include "app_config.h"
#include "imu_proc.h"
#include "filter.h"

static imu_state_t s_imu;
static gyro_hpf_t s_gyro_hpf_x;
static gyro_hpf_t s_gyro_hpf_y;

const imu_state_t *IMU_GetState(void)
{
    return &s_imu;
}

void IMU_ZeroAngleX(void)
{
    s_imu.angle[0] = 0.0f;
}

static float accel_offset_x = 0;	 // X轴加速度零偏
static float accel_offset_y = 0;	 // Y轴加速度零偏
static float accel_offset_z = 0;	 // Z轴加速度零偏
static float gyro_offset_x = 0;	     // X轴陀螺仪零偏
static float gyro_offset_y = 0;	     // Y轴陀螺仪零偏
static float gyro_offset_z = 0;	     // Z轴陀螺仪零偏

/* 梯形积分历史集中保存，支持再次起跑从确定状态开始。 */
static float s_accel_last_x = 0.0f;
static float s_accel_last_y = 0.0f;
static float s_accel_last_z = 0.0f;
static float s_gyro_last_x = 0.0f;
static float s_gyro_last_y = 0.0f;
static float s_gyro_last_z = 0.0f;


// 函数名: imu_proc_init
// 功能: IMU模块初始化
// 说明: 初始化 IMU660RB 硬件（SPI），并初始化 x/y 轴陀螺仪高通滤波器
//       （源工程在 config.c 中 gyro_hpf_init 0.2Hz/0.002，此处收拢）。
//       上电时调用一次，之后调用 gyro_calibrate()/accel_calibrate() 校准零偏。
void imu_proc_init(void)
{
    BSP_IMU660RB_Init();
    gyro_hpf_init(&s_gyro_hpf_x, 0.2f, CTRL_DT_S);
    gyro_hpf_init(&s_gyro_hpf_y, 0.2f, CTRL_DT_S);
}


// 函数名: read_accel_velocity
// 功能: 读取加速度并积分得到速度
// 说明: 照搬源工程实现（含源工程中读取陀螺仪原始值、除以14.3、
//       积分项使用 s_imu.gyro[0]/s_imu.gyro[1]/s_imu.gyro[2] 的原始写法），保持语义一致。
//       本工程默认不调用，仅保留接口备用。
void read_accel_velocity(void)
{
	BSP_IMU660RB_GetGyro(&s_imu.raw_gyro[0], &s_imu.raw_gyro[1], &s_imu.raw_gyro[2]);

	s_imu.accel[0] = ((float)s_imu.raw_accel[0] - accel_offset_x) / GYRO_RAW_TO_DPS;
	s_imu.accel[1] = ((float)s_imu.raw_accel[1] - accel_offset_y) / GYRO_RAW_TO_DPS;
	s_imu.accel[2] = ((float)s_imu.raw_accel[2] - accel_offset_z) / GYRO_RAW_TO_DPS;

	s_imu.velocity[0] += (s_imu.gyro[0] + s_accel_last_x) * 0.001f;		//0.5 * 0.002
	s_imu.velocity[1] += (s_imu.gyro[1] + s_accel_last_y) * 0.001f;
	s_imu.velocity[2] += (s_imu.gyro[2] + s_accel_last_z) * 0.001f;

	s_accel_last_x = s_imu.accel[0];
	s_accel_last_y = s_imu.accel[1];
	s_accel_last_z = s_imu.accel[2];
}


// 函数名: read_gyro_angle
// 功能: 读取陀螺仪并积分得到角度（2ms 周期调用）
// 说明: 获取IMU角速度数据，去零偏后除以14.3换算为°/s，
//       y轴经0.2Hz高通滤波，最后梯形积分得到各轴角度（系数0.001 = 0.5*0.002）。
void read_gyro_angle(void)
{
	BSP_Sampler_GetGyroRaw(&s_imu.raw_gyro[0], &s_imu.raw_gyro[1], &s_imu.raw_gyro[2]);	// 采样器 1kHz 缓存（SPI 由 TIM15 读取），控制环 2ms 取用

	s_imu.gyro[0] = ((float)s_imu.raw_gyro[0] - gyro_offset_x) / GYRO_RAW_TO_DPS;
	s_imu.gyro[1] = ((float)s_imu.raw_gyro[1] - gyro_offset_y) / GYRO_RAW_TO_DPS;
	s_imu.gyro[2] = ((float)s_imu.raw_gyro[2] - gyro_offset_z) / GYRO_RAW_TO_DPS;

	s_imu.gyro[1] = gyro_hpf_update(&s_gyro_hpf_y, s_imu.gyro[1]);		// y轴角速度高通滤波

	s_imu.angle[0] += (s_imu.gyro[0] + s_gyro_last_x) * 0.001f;		//0.5 * 0.002
	s_imu.angle[1] += (s_imu.gyro[1] + s_gyro_last_y) * 0.001f;
	s_imu.angle[2] += (s_imu.gyro[2] + s_gyro_last_z) * 0.001f;

	s_gyro_last_x = s_imu.gyro[0];
	s_gyro_last_y = s_imu.gyro[1];
	s_gyro_last_z = s_imu.gyro[2];
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
		BSP_IMU660RB_GetAcc(&s_imu.raw_accel[0], &s_imu.raw_accel[1], &s_imu.raw_accel[2]);
		temp[0] += s_imu.raw_accel[0];
        temp[1] += s_imu.raw_accel[1];
        temp[2] += s_imu.raw_accel[2];
    }

    accel_offset_x = (float)temp[0] / (float)samples;
    accel_offset_y = (float)temp[1] / (float)samples;
    accel_offset_z = (float)temp[2] / (float)samples;

	s_imu.velocity[0] = 0,	s_imu.velocity[1] = 0,	s_imu.velocity[2] = 0;
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
		BSP_IMU660RB_GetGyro(&s_imu.raw_gyro[0], &s_imu.raw_gyro[1], &s_imu.raw_gyro[2]);
		temp[0] += s_imu.raw_gyro[0];
        temp[1] += s_imu.raw_gyro[1];
        temp[2] += s_imu.raw_gyro[2];
    }

    gyro_offset_x = (float)temp[0] / (float)samples;
    gyro_offset_y = (float)temp[1] / (float)samples;
    gyro_offset_z = (float)temp[2] / (float)samples;

	s_imu.angle[0] = 0, s_imu.angle[1] = 0, s_imu.angle[2] = 0;
}

// 函数名: IMU_ResetRunState
// 功能: 清除运行积分量与梯形积分历史；保留上电校准得到的零偏
void IMU_ResetRunState(void)
{
    s_imu.accel[0] = 0.0f;
    s_imu.accel[1] = 0.0f;
    s_imu.accel[2] = 0.0f;
    s_imu.gyro[0] = 0.0f;
    s_imu.gyro[1] = 0.0f;
    s_imu.gyro[2] = 0.0f;
    s_imu.velocity[0] = 0.0f;
    s_imu.velocity[1] = 0.0f;
    s_imu.velocity[2] = 0.0f;
    s_imu.angle[0] = 0.0f;
    s_imu.angle[1] = 0.0f;
    s_imu.angle[2] = 0.0f;
    s_accel_last_x = 0.0f;
    s_accel_last_y = 0.0f;
    s_accel_last_z = 0.0f;
    s_gyro_last_x = 0.0f;
    s_gyro_last_y = 0.0f;
    s_gyro_last_z = 0.0f;
    gyro_hpf_init(&s_gyro_hpf_x, 0.2f, CTRL_DT_S);
    gyro_hpf_init(&s_gyro_hpf_y, 0.2f, CTRL_DT_S);
}
