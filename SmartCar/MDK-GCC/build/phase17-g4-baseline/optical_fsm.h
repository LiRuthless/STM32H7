/**
 * @file optical_fsm.h
 * @brief App 光电影子状态机；TIM6 每 2 ms 观察，唯一执行口仍在 control。
 * @details 声明光电状态机输入、状态及观察接口；完整转移和保护规则位于 optical_fsm.c。
 */
#ifndef SMARTCAR_APP_OPTICAL_FSM_H
#define SMARTCAR_APP_OPTICAL_FSM_H
#include "control/pid.h"

typedef enum
{
    OPTICAL_WAIT = 0,     /**< 尚未起跑，仅更新观察量而不输出循迹控制。 */
    OPTICAL_TRACKING = 1, /**< 起跑且灰度有效，发布当前光电方向输出。 */
    OPTICAL_LOST = 2,     /**< 起跑后灰度无效，锁存丢线保护状态。 */
    OPTICAL_STOPPED = 3   /**< 上层已锁存停车原因，输出保持为零。 */
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
 * @note 停车起跑准备调用；保留 base_speed，不清其他模式。 完整契约见 optical_fsm.c::OpticalFSM_Reset。
 */
void OpticalFSM_Reset(void);

/**
 * @brief 更新光电影子状态及方向计算。
 * @note inputs 非空本拍输入快照。 TIM6 每 2 ms 调用；LOST 锁存，1000 拍仍等待，1001 拍双信号有效才进入 TRACKING。 完整契约见
 *     optical_fsm.c::OpticalFSM_Update。
 */
void OpticalFSM_Update(const optical_inputs_t *inputs);

/**
 * @brief 读取模块持有的只读运行状态。
 * @note 静态对象寿命为整个程序；控制拍可更新，前台一致性读取须遵守既有停车约定。 完整契约见 optical_fsm.c::OpticalFSM_GetState。
 */
const optical_state_t *OpticalFSM_GetState(void);

/**
 * @brief 选择本拍状态和原策略，不推进方向历史。
 * @note 完整契约见 optical_fsm.c::OpticalFSM_Select。
 */
void OpticalFSM_Select(const optical_inputs_t *in);
/**
 * @brief 消费本拍许可并计算一次最终方向。
 * @note 完整契约见 optical_fsm.c::OpticalFSM_Calculate。
 */
void OpticalFSM_Calculate(const optical_inputs_t *in,
    const pid_track_gains_t *gains, int16_t base_speed);

#endif
