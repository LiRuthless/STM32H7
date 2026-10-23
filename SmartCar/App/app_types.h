/**
 * @file app_types.h
 * @brief 应用跨模块停车原因。
 * @details 共享停车枚举保持数值，供运行、安全、日志与控制接口使用。
 */
#ifndef SMARTCAR_APP_APP_TYPES_H
#define SMARTCAR_APP_APP_TYPES_H

typedef enum
{
    APP_STOP_NONE = 0,         /**< 未锁存停车原因；系统仍可处于等待或运行状态。 */
    APP_STOP_OFF_TRACK,        /**< 任一所选模式下电感保护判定失线，请求停止执行口。 */
    APP_STOP_LOW_BATTERY,      /**< 电池滤波值低于保护阈值，请求停止执行口。 */
    APP_STOP_GRAY_LOST,        /**< 任一所选模式下灰度线无效，请求停止执行口。 */
    APP_STOP_GRAY_ACQUIRE,     /**< 灰度采集返回参数、超时或 DMA 错误。 */
    APP_STOP_CONTROL_OVERRUN,  /**< 灰度或控制执行时间超过预算。 */
    APP_STOP_LOG_OVERFLOW,     /**< 日志生产环形队列无空位。 */
    APP_STOP_LOG_WRITE,        /**< 日志数据页写入 Flash 失败。 */
    APP_STOP_LOG_FULL          /**< 日志区域达到固定容量上限。 */
} app_stop_reason_t;

#endif
