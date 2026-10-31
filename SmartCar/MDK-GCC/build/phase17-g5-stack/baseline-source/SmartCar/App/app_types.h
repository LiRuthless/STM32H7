/**
 * @file app_types.h
 * @brief 应用跨模块运行、参数及调参业务类型。
 * @details 共享停车/模式枚举保持数值，参数及调参字段保持位宽与顺序；不包含通信类型。
 */
#ifndef SMARTCAR_APP_APP_TYPES_H
#define SMARTCAR_APP_APP_TYPES_H

#include <stdint.h>

typedef enum
{
    APP_STOP_NONE = 0, /**< 未锁存停车原因；系统仍可处于等待或运行状态。 */
    APP_STOP_OFF_TRACK, /**< 任一所选模式下电感保护判定失线，请求停止执行口。 */
    APP_STOP_LOW_BATTERY, /**< 电池滤波值低于保护阈值，请求停止执行口。 */
    APP_STOP_GRAY_LOST, /**< 任一所选模式下灰度线无效，请求停止执行口。 */
    APP_STOP_GRAY_ACQUIRE, /**< 灰度采集返回参数、超时或 DMA 错误。 */
    APP_STOP_CONTROL_OVERRUN, /**< 灰度或控制执行时间超过预算。 */
    APP_STOP_LOG_OVERFLOW, /**< 日志生产环形队列无空位。 */
    APP_STOP_LOG_WRITE, /**< 日志数据页写入 Flash 失败。 */
    APP_STOP_LOG_FULL /**< 日志区域达到固定容量上限。 */
} app_stop_reason_t;

typedef struct
{
    float kp; /**< 方向比例增益，保留原标度。 */
    float k2p; /**< 方向非线性比例增益。 */
    float kd; /**< 方向拍间差分增益。 */
    int16_t base_speed; /**< 基础速度，脉冲/2 ms。 */
} param_direction_t;

typedef struct
{
    float kp; /**< 方向比例增益。 */
    float k2p; /**< 方向非线性增益。 */
    float kd; /**< 方向拍间差分增益。 */
    float kp_v; /**< 两轮共享速度比例增益。 */
    float ki_v; /**< 两轮共享速度积分增益。 */
    int32_t base_speed; /**< 基础速度，脉冲/2ms，校验后才窄化。 */
    int32_t fan_run; /**< 运行风扇万分比，0..10000。 */
    int32_t fan_idle; /**< 空闲风扇万分比，0..10000。 */
} tuning_values_t;

typedef enum
{
    TRACKING_MODE_EM = 0, /**< 电磁循迹，执行电感影子状态机输出。 */
    TRACKING_MODE_OPTICAL = 1 /**< 光电循迹，执行灰度影子状态机输出。 */
} tracking_mode_t;

typedef enum
{
    TRACKING_OK = 0, /**< 请求完成，运行态与持久化状态均符合要求。 */
    TRACKING_BUSY, /**< 车辆正在运行，不允许切换模式或档位。 */
    TRACKING_SAVE_FAILED, /**< 模式尚未写入，参数持久化门禁失败。 */
    TRACKING_ARG /**< 模式或档位参数超出 0..1。 */
} tracking_result_t;

#endif
