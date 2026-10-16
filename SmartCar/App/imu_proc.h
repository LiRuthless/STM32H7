/**
 * @file imu_proc.h
 * @brief App IMU 零偏、滤波与积分；TIM6 使用采样缓存，保留休眠积分入口。
 * @note 移植来源：Sirius20260718/project/code/gyroscope.h，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#ifndef SMARTCAR_APP_IMU_PROC_H
#define SMARTCAR_APP_IMU_PROC_H

#include <stdint.h>

typedef struct
{
    float accel[3];       /**< x/y/z，保留源算法标度；当前运行链不调用该积分入口。 */
    float gyro[3];        /**< x/y/z 角速度，度/秒；y 轴经过高通。 */
    float velocity[3];    /**< 保留源算法积分标度，当前运行链不调用该积分入口。 */
    float angle[3];       /**< x/y/z 累积角度，度；控制拍梯形积分。 */
    int16_t raw_gyro[3];  /**< x/y/z 有符号陀螺原始计数。 */
    int16_t raw_accel[3]; /**< x/y/z 有符号加速度原始计数。 */
} imu_state_t;

/**
 * @brief 读取最近一拍 x 轴角度增量。
 * @return 度，保留 0.001 梯形积分系数。
 * @note 控制拍更新，供电磁影子机使用。
 */
float IMU_GetAngleDeltaX(void);

/**
 * @brief 读取模块持有的只读运行状态。
 * @return 非空只读指针，不转移所有权。
 * @note 静态对象寿命为整个程序；控制拍可更新，前台一致性读取须遵守既有停车约定。
 */
const imu_state_t *IMU_GetState(void);

/**
 * @brief 仅清 x 轴累计角度。
 * @note 不清梯形积分历史或本拍增量；控制拍状态转移使用。
 */
void IMU_ZeroAngleX(void);

/**
 * @brief 初始化 IMU 和两个原高通实例。
 * @note 上电前台调用；保持忽略 BSP 返回值及 0.2 Hz/2 ms 初始化，不重复校准。
 */
void IMU_Init(void);

/**
 * @brief 保留源加速度与速度积分入口。
 * @note 当前不接入；保留读取陀螺、raw_accel/14.3 及 gyro 积分的原写法，不借风格整改修正。
 */
void IMU_UpdateAccelVelocity(void);

/**
 * @brief 从采样器缓存去偏置并梯形积分角度。
 * @note TIM6 每 2 ms 调用；只对 y 轴高通，保持浮点顺序和 x 增量发布位置。
 */
void IMU_UpdateGyroAngle(void);

/**
 * @brief 静止时采一百次并计算加速度零偏。
 * @note 启动前前台调用，有界阻塞；保持原样本类型、读取顺序及积分量逗号表达式清零。
 */
void IMU_CalibrateAccel(void);

/**
 * @brief 静止时采一百次并计算陀螺零偏。
 * @note 启动前前台调用，有界阻塞；保持原样本类型、读取顺序及积分量逗号表达式清零。
 */
void IMU_CalibrateGyro(void);

/**
 * @brief 清积分量和梯形历史，保留上电零偏。
 * @note 再次起跑前停车调用，不访问 SPI 或重新校准。
 */
void IMU_ResetRunState(void);

#endif
