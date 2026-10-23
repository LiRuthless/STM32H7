/**
 * @file imu_proc.c
 * @brief App IMU 零偏、滤波与积分；TIM6 使用采样缓存，保留休眠积分入口。
 * @details 读取采样器缓存或保留入口的直接读数，执行零偏、高通和梯形积分；运行态由 TIM6 更新。
 * @note 移植来源：Sirius20260718/project/code/gyroscope.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include "sensing/imu_proc.h"
#include "sensing/filter.h"
#include "sensing/legacy_imu.h"

/* TIM6 独占活动积分；启动前校准和停车复位，零偏跨再次起跑保留。 */
static imu_state_t s_imu;
static float s_angle_delta_x;

static filter_highpass_t s_gyro_hpf_x;
static filter_highpass_t s_gyro_hpf_y;

/* X轴加速度零偏 */
static float s_accel_offset_x = 0;
/* Y轴加速度零偏 */
static float s_accel_offset_y = 0;
/* Z轴加速度零偏 */
static float s_accel_offset_z = 0;
/* X轴陀螺仪零偏 */
static float s_gyro_offset_x = 0;
/* Y轴陀螺仪零偏 */
static float s_gyro_offset_y = 0;
/* Z轴陀螺仪零偏 */
static float s_gyro_offset_z = 0;

/* 梯形积分历史集中保存，支持再次起跑从确定状态开始。 */
static float s_accel_last_x = 0.0f;
static float s_accel_last_y = 0.0f;
static float s_accel_last_z = 0.0f;
static float s_gyro_last_x = 0.0f;
static float s_gyro_last_y = 0.0f;
static float s_gyro_last_z = 0.0f;

/**
 * @brief 读取最近一拍 x 轴角度增量。
 * @return 度，保留 0.001 梯形积分系数。
 * @note 控制拍更新，供电磁影子机使用。
 */
float IMU_GetAngleDeltaX(void)
{
    return s_angle_delta_x;
}

/**
 * @brief 读取模块持有的只读运行状态。
 * @return 非空只读指针，不转移所有权。
 * @note 静态对象寿命为整个程序；控制拍可更新，前台一致性读取须遵守既有停车约定。
 */
const imu_state_t *IMU_GetState(void)
{
    return &s_imu;
}

/**
 * @brief 仅清 x 轴累计角度。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @note 不清梯形积分历史或本拍增量；控制拍状态转移使用。
 */
void IMU_ZeroAngleX(void)
{
    s_imu.angle[0] = 0.0f;
}

/**
 * @brief 初始化 IMU 和两个原高通实例。
 * @details 按现有调用顺序使用 BSP_IMU660RB_Init、Filter_InitHighpass 完成本函数处理。
 * @note 上电前台调用；保持忽略 BSP 返回值及 0.2 Hz/2 ms 初始化，不重复校准。
 */
void IMU_Init(void)
{
    BSP_IMU660RB_Init();
    Filter_InitHighpass(&s_gyro_hpf_x, 0.2f, APP_CTRL_DT_S);
    Filter_InitHighpass(&s_gyro_hpf_y, 0.2f, APP_CTRL_DT_S);
}

/**
 * @brief 保留源加速度与速度积分入口。
 * @details 按现有调用顺序使用 BSP_IMU660RB_GetGyro 完成本函数处理。
 * @note 当前不接入；保留读取陀螺、raw_accel/14.3 及 gyro 积分的原写法，不借风格整改修正。
 */
void IMU_UpdateAccelVelocity(void)
{
    const float offsets[3] = { s_accel_offset_x, s_accel_offset_y, s_accel_offset_z };
    float *const history[3] = { &s_accel_last_x, &s_accel_last_y, &s_accel_last_z };
    LegacyIMU_UpdateAccelVelocity(&s_imu, offsets, history);
}

/**
 * @brief 从采样器缓存去偏置并梯形积分角度。
 * @details 从 TIM15 缓存读取三轴原始计数，扣除零偏并换算角速度，仅对 Y 轴高通；随后以 2 ms 梯形积分系数更新本拍 X 增量和三轴累计角度，最后保存本拍历史。
 * @note TIM6 每 2 ms 调用；只对 y 轴高通，保持浮点顺序和 x 增量发布位置。
 */
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

/**
 * @brief 静止时采一百次并计算加速度零偏。
 * @details 按现有调用顺序使用 BSP_IMU660RB_GetAcc 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 启动前前台调用，有界阻塞；保持原样本类型、读取顺序及积分量逗号表达式清零。
 */
void IMU_CalibrateAccel(void)
{
    float *const offsets[3] = { &s_accel_offset_x, &s_accel_offset_y, &s_accel_offset_z };
    LegacyIMU_CalibrateAccel(&s_imu, offsets);
}

/**
 * @brief 静止时采一百次并计算陀螺零偏。
 * @details 按现有调用顺序使用 BSP_IMU660RB_GetGyro 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 启动前前台调用，有界阻塞；保持原样本类型、读取顺序及积分量逗号表达式清零。
 */
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

/**
 * @brief 清积分量和梯形历史，保留上电零偏。
 * @details 按现有调用顺序使用 Filter_InitHighpass 完成本函数处理。
 * @note 再次起跑前停车调用，不访问 SPI 或重新校准。
 */
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
