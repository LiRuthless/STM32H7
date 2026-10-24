/**
 * @file tuning_values.h
 * @brief 调参业务值、请求及结果。
 * @details 纯数值validator与业务类型不依赖事务、BSP或通信。
 */
#ifndef SMARTCAR_APP_CONTROL_TUNING_VALUES_H
#define SMARTCAR_APP_CONTROL_TUNING_VALUES_H
#include "app_types.h"

typedef enum
{
    TUNING_OK, /**< 参数合法或请求成功。 */
    TUNING_RECEIVED, /**< 请求等待完整拍。 */
    TUNING_APPLIED, /**< 已在完整拍应用。 */
    TUNING_SAVED, /**< 保存成功。 */
    TUNING_INVALID_ARG, /**< 参数形状无效。 */
    TUNING_OUT_OF_RANGE, /**< 参数越界或非有限。 */
    TUNING_BUSY, /**< 当前状态不允许。 */
    TUNING_SPECIAL_ELEMENT, /**< 赛道元素禁止覆盖。 */
    TUNING_NOT_FOUND, /**< 目标不存在。 */
    TUNING_IO_ERROR, /**< 保存或服务错误。 */
} tuning_result_code_t;
typedef struct
{
    uint8_t mode; /**< 目标模式，0电磁、1光电。 */
    uint8_t gear; /**< 目标档位，0低、1高。 */
    tuning_values_t values; /**< 完整八项请求值。 */
} tuning_request_t;

typedef struct
{
    uint32_t tick; /**< 原捕获采样tick，ms。 */
    uint32_t control_sequence; /**< 完整控制序号。 */
    uint32_t revision; /**< 本次有效参数版本。 */
} tuning_position_t;

typedef struct
{
    tuning_result_code_t code; /**< 业务状态；仅APPLIED携带values。 */
    tuning_values_t values; /**< 已应用请求或最终成功值。 */
    tuning_position_t position; /**< 原发生点捕获的位置。 */
} tuning_reply_t;
/**
 * @brief 使用同源limits复核完整C参数，避免绕过解析入口的半组更新。
 * @note 完整契约见 tuning_values.c::Tuning_ValidateValues。
 */
tuning_result_code_t Tuning_ValidateValues(const tuning_values_t *values);

#endif /* SMARTCAR_APP_CONTROL_TUNING_VALUES_H */
