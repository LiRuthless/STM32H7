/**
 * @file optical_fsm.h
 * @brief App 光电影子状态机；TIM6 每 2 ms 观察，唯一执行口仍在 control。
 */
#ifndef SMARTCAR_APP_OPTICAL_FSM_H
#define SMARTCAR_APP_OPTICAL_FSM_H
#include "pid.h"

typedef enum
{
    OPTICAL_WAIT = 0,
    OPTICAL_TRACKING = 1,
    OPTICAL_LOST = 2,
    OPTICAL_STOPPED = 3
} optical_phase_t;

typedef struct
{
    uint8_t running;         /**< 运行标志，非零表示已起跑。 */
    uint8_t gray_valid;      /**< 光电有效标志，非零有效。 */
    uint8_t em_valid;        /**< 电磁有效标志，非零有效。 */
    uint8_t stop_reason;     /**< 停车原因标志，非零要求停车。 */
    int32_t time;            /**< 起跑后 2 ms 控制拍计数，保留 int32_t。 */
    int16_t error;           /**< 光电归一化偏差，通常 -100..100。 */
    int16_t base_speed;      /**< 当前模式基础速度，每控制拍脉冲。 */
    float accel;             /**< 方向阻尼输入，保留源 IMU 标度。 */
    pid_track_gains_t gains; /**< 本拍方向增益副本，不改其他模式历史。 */
} optical_inputs_t;

typedef struct
{
    optical_phase_t state; /**< OPTICAL_* 影子阶段；LOST 在复位前锁存。 */
    int16_t error;         /**< 最近观察偏差，状态拒绝更新前也发布。 */
    int16_t output;        /**< 本模式方向输出，正常为 -1000..1000。 */
    int16_t base_speed;    /**< 最近观察的基础速度；Reset 不改此字段。 */
} optical_state_t;

/**
 * @brief 清本模式阶段、偏差、输出与独立历史。
 * @note 停车起跑准备调用；保留 base_speed，不清其他模式。
 */
void OpticalFSM_Reset(void);

/**
 * @brief 更新光电影子状态及方向计算。
 * @param[in] inputs 非空本拍输入快照。
 * @note TIM6 每 2 ms 调用；LOST 锁存，1000 拍仍等待，1001 拍双信号有效才进入 TRACKING。
 */
void OpticalFSM_Update(const optical_inputs_t *inputs);

/**
 * @brief 读取模块持有的只读运行状态。
 * @return 非空只读指针，不转移所有权。
 * @note 静态对象寿命为整个程序；控制拍可更新，前台一致性读取须遵守既有停车约定。
 */
const optical_state_t *OpticalFSM_GetState(void);

#endif
