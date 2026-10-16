/**
 * @file imu_proc.c
 * @brief App IMU 零偏、滤波与积分；TIM6 使用采样缓存，保留休眠积分入口。
 * @note 移植来源：Sirius20260718/project/code/gyroscope.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include "imu_proc.h"
#include "filter.h"

/* TIM6 独占活动积分；启动前校准和停车复位，零偏跨再次起跑保留。 */
static imu_state_t s_imu;
static float s_angle_delta_x;

static filter_highpass_t s_gyro_hpf_x;
static filter_highpass_t s_gyro_hpf_y;

static float s_accel_offset_x = 0; /* X轴加速度零偏 */
static float s_accel_offset_y = 0; /* Y轴加速度零偏 */
static float s_accel_offset_z = 0; /* Z轴加速度零偏 */
static float s_gyro_offset_x = 0;  /* X轴陀螺仪零偏 */
static float s_gyro_offset_y = 0;  /* Y轴陀螺仪零偏 */
static float s_gyro_offset_z = 0;  /* Z轴陀螺仪零偏 */

/* 梯形积分历史集中保存，支持再次起跑从确定状态开始。 */
static float s_accel_last_x = 0.0f;
static float s_accel_last_y = 0.0f;
static float s_accel_last_z = 0.0f;
static float s_gyro_last_x = 0.0f;
static float s_gyro_last_y = 0.0f;
static float s_gyro_last_z = 0.0f;

float IMU_GetAngleDeltaX(void)
{
    return s_angle_delta_x;
}

const imu_state_t *IMU_GetState(void)
{
    return &s_imu;
}

void IMU_ZeroAngleX(void)
{
    s_imu.angle[0] = 0.0f;
}

void IMU_Init(void)
{
    BSP_IMU660RB_Init();
    Filter_InitHighpass(&s_gyro_hpf_x, 0.2f, APP_CTRL_DT_S);
    Filter_InitHighpass(&s_gyro_hpf_y, 0.2f, APP_CTRL_DT_S);
}

void IMU_UpdateAccelVelocity(void)
{
    BSP_IMU660RB_GetGyro(&s_imu.raw_gyro[0], &s_imu.raw_gyro[1], &s_imu.raw_gyro[2]);

    s_imu.accel[0] = ((float)s_imu.raw_accel[0] - s_accel_offset_x) / IMU_GYRO_RAW_TO_DPS;
    s_imu.accel[1] = ((float)s_imu.raw_accel[1] - s_accel_offset_y) / IMU_GYRO_RAW_TO_DPS;
    s_imu.accel[2] = ((float)s_imu.raw_accel[2] - s_accel_offset_z) / IMU_GYRO_RAW_TO_DPS;

    s_imu.velocity[0] += (s_imu.gyro[0] + s_accel_last_x) * 0.001f; /* 0.5 * 0.002 */
    s_imu.velocity[1] += (s_imu.gyro[1] + s_accel_last_y) * 0.001f;
    s_imu.velocity[2] += (s_imu.gyro[2] + s_accel_last_z) * 0.001f;

    s_accel_last_x = s_imu.accel[0];
    s_accel_last_y = s_imu.accel[1];
    s_accel_last_z = s_imu.accel[2];
}

void IMU_UpdateGyroAngle(void)
{
    BSP_Sampler_GetGyroRaw(&s_imu.raw_gyro[0], &s_imu.raw_gyro[1],
        &s_imu.raw_gyro[2]); /* 采样器 1kHz 缓存（SPI 由 TIM15 读取），控制环 2ms 取用 */

    s_imu.gyro[0] = ((float)s_imu.raw_gyro[0] - s_gyro_offset_x) / IMU_GYRO_RAW_TO_DPS;
    s_imu.gyro[1] = ((float)s_imu.raw_gyro[1] - s_gyro_offset_y) / IMU_GYRO_RAW_TO_DPS;
    s_imu.gyro[2] = ((float)s_imu.raw_gyro[2] - s_gyro_offset_z) / IMU_GYRO_RAW_TO_DPS;

    s_imu.gyro[1] = Filter_UpdateHighpass(&s_gyro_hpf_y, s_imu.gyro[1]); /* y轴角速度高通滤波 */

    s_angle_delta_x = (s_imu.gyro[0] + s_gyro_last_x) * 0.001f;
    s_imu.angle[0] += s_angle_delta_x;
    s_imu.angle[1] += (s_imu.gyro[1] + s_gyro_last_y) * 0.001f;
    s_imu.angle[2] += (s_imu.gyro[2] + s_gyro_last_z) * 0.001f;

    s_gyro_last_x = s_imu.gyro[0];
    s_gyro_last_y = s_imu.gyro[1];
    s_gyro_last_z = s_imu.gyro[2];
}

void IMU_CalibrateAccel(void)
{
    const int8_t samples = 100;
    int32_t temp[3] = { 0 }; /* 临时计算变量 */
    int8_t i = 0;            /* 循环计数器 */

    for (i = 0; i < samples; i++)
    {
        BSP_IMU660RB_GetAcc(&s_imu.raw_accel[0], &s_imu.raw_accel[1], &s_imu.raw_accel[2]);
        temp[0] += s_imu.raw_accel[0];
        temp[1] += s_imu.raw_accel[1];
        temp[2] += s_imu.raw_accel[2];
    }

    s_accel_offset_x = (float)temp[0] / (float)samples;
    s_accel_offset_y = (float)temp[1] / (float)samples;
    s_accel_offset_z = (float)temp[2] / (float)samples;

    s_imu.velocity[0] = 0, s_imu.velocity[1] = 0, s_imu.velocity[2] = 0;
}

void IMU_CalibrateGyro(void)
{
    const int8_t samples = 100;
    int32_t temp[3] = { 0 }; /* 临时计算变量 */
    int8_t i = 0;            /* 循环计数器 */

    for (i = 0; i < samples; i++)
    {
        BSP_IMU660RB_GetGyro(&s_imu.raw_gyro[0], &s_imu.raw_gyro[1], &s_imu.raw_gyro[2]);
        temp[0] += s_imu.raw_gyro[0];
        temp[1] += s_imu.raw_gyro[1];
        temp[2] += s_imu.raw_gyro[2];
    }

    s_gyro_offset_x = (float)temp[0] / (float)samples;
    s_gyro_offset_y = (float)temp[1] / (float)samples;
    s_gyro_offset_z = (float)temp[2] / (float)samples;

    s_imu.angle[0] = 0, s_imu.angle[1] = 0, s_imu.angle[2] = 0;
}

void IMU_ResetRunState(void)
{
    s_angle_delta_x = 0.0f;
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
    Filter_InitHighpass(&s_gyro_hpf_x, 0.2f, APP_CTRL_DT_S);
    Filter_InitHighpass(&s_gyro_hpf_y, 0.2f, APP_CTRL_DT_S);
}
