/**
 * @file tuning_values.c
 * @brief 完整调参值的唯一范围校验。
 * @details 仅按同源配置限值执行有序比较；NaN、无穷及整数越界失败，不钳位。
 */
#include "control/tuning_values.h"
#include "app_config.h"

/**
 * @brief 使用同源limits复核完整C参数，避免绕过解析入口的半组更新。
 * @details 五项浮点用有序比较同时排除NaN/Inf，再检查三项整数边界；失败不修改对象。
 * @param[in] values 非空八项快照；空指针INVALID_ARG。
 * @return 空输入INVALID_ARG；有限且各项在配置范围内OK；其他输入OUT_OF_RANGE。
 * @note 纯计算，前台或控制路径可调用，不钳位、不修改输入。
 */
tuning_result_code_t Tuning_ValidateValues(const tuning_values_t *values)
{
    float gains[5];
    uint8_t i;
    if (!values)
    {
        return TUNING_INVALID_ARG;
    }
    gains[0] = values->kp;
    gains[1] = values->k2p;
    gains[2] = values->kd;
    gains[3] = values->kp_v;
    gains[4] = values->ki_v;
    for (i = 0; i < 5; ++i)
    {
        if (!(gains[i] >= TUNING_GAIN_MIN && gains[i] <= TUNING_GAIN_MAX))
        {
            return TUNING_OUT_OF_RANGE;
        }
    }
    if (values->base_speed < TUNING_SPEED_MIN || values->base_speed > TUNING_SPEED_MAX
        || values->fan_run < TUNING_FAN_MIN || values->fan_run > TUNING_FAN_MAX
        || values->fan_idle < TUNING_FAN_MIN || values->fan_idle > TUNING_FAN_MAX)
    {
        return TUNING_OUT_OF_RANGE;
    }
    return TUNING_OK;
}
