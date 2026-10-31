/**
 * @file legacy_imu.c
 * @brief 旧加速度算法，无持久状态。
 * @details owner 提供状态和独立偏置/历史地址，读取均经 BSP。
 * @note 移植来源：Sirius20260718/project/code/gyroscope.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源文件未单列作者及独立许可证；保留出处，关联 Phase 13/15。
 */
#include "bsp.h"
#include "app_config.h"
#include "sensing/legacy_imu.h"
/**
 * @brief 用旧公式更新加速度与速度积分。
 * @details 先读 Gyro 到 raw_gyro，仍用 raw_accel 减偏置除 14.3 得到 accel；
 *     速度累加 (gyro+前拍 accel)*0.001，最后以本拍 accel 更新历史。
 *     读数来源和积分输入不一致属于保留的旧算法疑点，本次不修正。
 * @param[in,out] state 非空 owner 状态，更新原加速度或速度字段。
 * @param[in] offsets 三轴偏置本次副本。
 * @param[in,out] history 三个非空指针，分别指向 owner 独立历史。
 * @note 仅由 imu_proc 同名入口组参转发，不新增活动调用。
 */
void LegacyIMU_UpdateAccelVelocity(imu_state_t *state, const float offsets[3],
    float *const history[3])
{
    BSP_IMU660RB_GetGyro(&state->raw_gyro[0], &state->raw_gyro[1], &state->raw_gyro[2]);

    state->accel[0] = ((float)state->raw_accel[0] - offsets[0]) / IMU_GYRO_RAW_TO_DPS;
    state->accel[1] = ((float)state->raw_accel[1] - offsets[1]) / IMU_GYRO_RAW_TO_DPS;
    state->accel[2] = ((float)state->raw_accel[2] - offsets[2]) / IMU_GYRO_RAW_TO_DPS;

    state->velocity[0] += (state->gyro[0] + *history[0]) * 0.001f; /* 0.5 * 0.002 */
    state->velocity[1] += (state->gyro[1] + *history[1]) * 0.001f;
    state->velocity[2] += (state->gyro[2] + *history[2]) * 0.001f;

    *history[0] = state->accel[0];
    *history[1] = state->accel[1];
    *history[2] = state->accel[2];
}

/**
 * @brief 采一百次加速度，写三轴偏置并清速度。
 * @details 以 int8_t 计数逐次 GetAcc 累加到 int32_t，再按 X/Y/Z 顺序求均值；
 *     最后清三轴 velocity，不清积分历史。当前入口不参与活动校准流程。
 * @param[in,out] state 非空 owner 状态，更新原加速度或速度字段。
 * @param[out] offsets 三个非空指针，依次写 owner 零偏。
 * @note 仅由 imu_proc 同名入口组参转发，不新增活动调用。
 */
void LegacyIMU_CalibrateAccel(imu_state_t *state, float *const offsets[3])
{
    const int8_t samples = 100;
    int32_t temp[3] = { 0 }; /* 临时计算变量 */
    int8_t i = 0;            /* 循环计数器 */

    for (i = 0; i < samples; i++)
    {
        BSP_IMU660RB_GetAcc(&state->raw_accel[0], &state->raw_accel[1], &state->raw_accel[2]);
        temp[0] += state->raw_accel[0];
        temp[1] += state->raw_accel[1];
        temp[2] += state->raw_accel[2];
    }

    *offsets[0] = (float)temp[0] / (float)samples;
    *offsets[1] = (float)temp[1] / (float)samples;
    *offsets[2] = (float)temp[2] / (float)samples;

    state->velocity[0] = 0, state->velocity[1] = 0, state->velocity[2] = 0;
}
