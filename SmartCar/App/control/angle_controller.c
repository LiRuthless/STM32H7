/**
 * @file angle_controller.c
 * @brief 纯角度控制计算。
 * @details 接收角度和角速度，按原顺序写 owner 误差输出并保存私有历史。
 * @note 移植来源：Sirius20260718/project/code/pid.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源文件未单列作者及独立许可证；保留出处，关联 Phase 13/15。
 */
#include "control/angle_controller.h"
/* 角度拍间差分历史，由计算更新、Reset 清零。 */
static float s_angle_err_last = 0.0f;
/**
 * @brief 计算角度 PD 并更新 owner 输出。
 * @details 按目标角度减当前 X 轴角度得到误差，计算比例加拍间差分再减角速度阻尼。
 *     依次写误差、写输出、保存本拍误差历史；本函数没有方向限幅。
 * @param[in] target_angle 目标 X 轴角度，单位为度。
 * @param[in] angle 当前角度，度。
 * @param[in] gyro 当前角速度，度/秒。
 * @param[in] gains 非空三项增益。
 * @param[out] error 非空 owner 误差字段。
 * @param[out] output 非空 owner 输出字段。
 * @note 本模块独占角度前拍历史，不执行电机动作。
 */
void AngleController_Update(int16_t target_angle, float angle, float gyro,
    const pid_angle_gains_t *gains, float *error, float *output)
{
    *error = target_angle - angle; /* 角度误差 = 目标角度 - 当前角度 */

    *output = gains->kp * *error
        + gains->kd * (*error - s_angle_err_last)
        - gains->kg * gyro; /* 角度PD控制：角度误差比例+微分-陀螺仪阻尼 */

    s_angle_err_last = *error;

}

/**
 * @brief 只清角度前拍历史。
 */
void AngleController_Reset(void)
{
    s_angle_err_last = 0.0f;
}
