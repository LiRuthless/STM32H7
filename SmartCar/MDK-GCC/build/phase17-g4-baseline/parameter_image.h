/**
 * @file parameter_image.h
 * @brief 参数镜像版本布局与保存快照。
 * @details v1/v2只读解码，v3按原88字节存储；本头不定义可写镜像。
 */
#ifndef SMARTCAR_APP_SERVICES_PARAMETER_IMAGE_H
#define SMARTCAR_APP_SERVICES_PARAMETER_IMAGE_H
#include <stdint.h>
#include <stddef.h>

typedef struct
{
    uint32_t magic;          /**< 原参数魔数，保持 32 位。 */
    uint16_t version;        /**< 持久化版本，v1/v2 仅解码，v3 写入。 */
    uint16_t crc16;          /**< 从字节 8 开始的原 CCITT 校验。 */
    uint8_t page;            /**< 保存的菜单页面编号；加载校验后应用。 */
    uint8_t arrow;           /**< 保存的光标，加载只接受 1..5。 */
    uint8_t reserved[2];     /**< 原两字节保留区，布局和 CRC 覆盖保持。 */
    float kp_v;              /**< 两模式共用速度比例增益。 */
    float ki_v;              /**< 两模式共用速度积分增益。 */
    float kp_x_low;          /**< 电磁低档方向比例增益。 */
    float k2p_x_low;         /**< 电磁低档方向非线性增益。 */
    float kd_x_low;          /**< 电磁低档方向差分增益。 */
    int16_t base_speed_low;  /**< 电磁低档基础速度，脉冲/2 ms。 */
    int16_t fan_duty_low;    /**< 低档运行风扇万分比，0..10000。 */
    float kp_x_high;         /**< 电磁高档方向比例增益。 */
    float k2p_x_high;        /**< 电磁高档方向非线性增益。 */
    float kd_x_high;         /**< 电磁高档方向差分增益。 */
    int16_t base_speed_high; /**< 电磁高档基础速度，脉冲/2 ms。 */
    int16_t fan_duty_high;   /**< 高档运行风扇万分比，0..10000。 */
} param_image_v1_t;

typedef struct
{
    uint32_t magic;             /**< 原参数魔数，保持 32 位。 */
    uint16_t version;           /**< 持久化版本，v1/v2 仅解码，v3 写入。 */
    uint16_t crc16;             /**< 从字节 8 开始的原 CCITT 校验。 */
    uint8_t page;               /**< 保存的菜单页面编号；加载校验后应用。 */
    uint8_t arrow;              /**< 保存的光标，加载只接受 1..5。 */
    uint8_t reserved[2];        /**< 原两字节保留区，布局和 CRC 覆盖保持。 */
    float kp_v;                 /**< 两模式共用速度比例增益。 */
    float ki_v;                 /**< 两模式共用速度积分增益。 */
    float kp_x_low;             /**< 电磁低档方向比例增益。 */
    float k2p_x_low;            /**< 电磁低档方向非线性增益。 */
    float kd_x_low;             /**< 电磁低档方向差分增益。 */
    int16_t base_speed_low;     /**< 电磁低档基础速度，脉冲/2 ms。 */
    int16_t fan_duty_low;       /**< 低档运行风扇万分比，0..10000。 */
    float kp_x_high;            /**< 电磁高档方向比例增益。 */
    float k2p_x_high;           /**< 电磁高档方向非线性增益。 */
    float kd_x_high;            /**< 电磁高档方向差分增益。 */
    int16_t base_speed_high;    /**< 电磁高档基础速度，脉冲/2 ms。 */
    int16_t fan_duty_high;      /**< 高档运行风扇万分比，0..10000。 */
    int16_t fan_duty_idle_low;  /**< 低档空闲风扇万分比，v2 起保存。 */
    int16_t fan_duty_idle_high; /**< 高档空闲风扇万分比，v2 起保存。 */
} param_image_v2_t;

typedef struct
{
    uint32_t magic;                 /**< 原参数魔数，保持 32 位。 */
    uint16_t version;               /**< 持久化版本，v1/v2 仅解码，v3 写入。 */
    uint16_t crc16;                 /**< 从字节 8 开始的原 CCITT 校验。 */
    uint8_t page;                   /**< 保存的菜单页面编号；加载校验后应用。 */
    uint8_t arrow;                  /**< 保存的光标，加载只接受 1..5。 */
    uint8_t reserved[2];            /**< 原两字节保留区，布局和 CRC 覆盖保持。 */
    float kp_v;                     /**< 两模式共用速度比例增益。 */
    float ki_v;                     /**< 两模式共用速度积分增益。 */
    float kp_x_low;                 /**< 电磁低档方向比例增益。 */
    float k2p_x_low;                /**< 电磁低档方向非线性增益。 */
    float kd_x_low;                 /**< 电磁低档方向差分增益。 */
    int16_t base_speed_low;         /**< 电磁低档基础速度，脉冲/2 ms。 */
    int16_t fan_duty_low;           /**< 低档运行风扇万分比，0..10000。 */
    float kp_x_high;                /**< 电磁高档方向比例增益。 */
    float k2p_x_high;               /**< 电磁高档方向非线性增益。 */
    float kd_x_high;                /**< 电磁高档方向差分增益。 */
    int16_t base_speed_high;        /**< 电磁高档基础速度，脉冲/2 ms。 */
    int16_t fan_duty_high;          /**< 高档运行风扇万分比，0..10000。 */
    int16_t fan_duty_idle_low;      /**< 低档空闲风扇万分比，v2 起保存。 */
    int16_t fan_duty_idle_high;     /**< 高档空闲风扇万分比，v2 起保存。 */
    float optical_kp_low;           /**< 光电低档方向比例增益，v3 新增。 */
    float optical_k2p_low;          /**< 光电低档方向非线性增益，v3 新增。 */
    float optical_kd_low;           /**< 光电低档方向差分增益，v3 新增。 */
    int16_t optical_speed_low;      /**< 光电低档基础速度，脉冲/2 ms。 */
    uint16_t optical_reserved_low;  /**< 光电低档两字节保留区，写零。 */
    float optical_kp_high;          /**< 光电高档方向比例增益，v3 新增。 */
    float optical_k2p_high;         /**< 光电高档方向非线性增益，v3 新增。 */
    float optical_kd_high;          /**< 光电高档方向差分增益，v3 新增。 */
    int16_t optical_speed_high;     /**< 光电高档基础速度，脉冲/2 ms。 */
    uint16_t optical_reserved_high; /**< 光电高档两字节保留区，写零。 */
} param_image_v3_t;

typedef char param_v1_size_assert_t[(sizeof(param_image_v1_t) == 52) ? 1 : -1];
typedef char param_v2_size_assert_t[(sizeof(param_image_v2_t) == 56) ? 1 : -1];
typedef char param_v3_size_assert_t[(sizeof(param_image_v3_t) == 88) ? 1 : -1];
typedef char param_v3_offset_assert_t[(offsetof(param_image_v3_t, optical_kp_low) == 56
                                          && offsetof(param_image_v3_t, optical_kp_high) == 72)
        ? 1
        : -1];
#define PARAM_IMAGE_DEFAULT_PAGE 0u
#define PARAM_IMAGE_DEFAULT_ARROW 1u

typedef struct
{
    float kp_v; /**< 本次保存采集的活动速度比例增益。 */
    float ki_v; /**< 本次保存采集的活动速度积分增益。 */
    uint8_t page; /**< 保存时的菜单页面原字节。 */
    uint8_t arrow; /**< 保存时的菜单光标原字节。 */
} param_save_snapshot_t;

#endif /* SMARTCAR_APP_SERVICES_PARAMETER_IMAGE_H */
