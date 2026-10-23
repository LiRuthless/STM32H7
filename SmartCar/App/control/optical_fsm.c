/**
 * @file optical_fsm.c
 * @brief App 光电影子状态机；TIM6 每 2 ms 观察，唯一执行口仍在 control。
 * @details 根据灰度质心与有效性推进光电普通循迹状态，并生成控制剖面和失线保护结果。
 */
#include "control/optical_fsm.h"
#include "app_config.h"
/* TIM6 独占本模式观察和独立方向历史；起跑复位不改 base_speed。 */
static optical_state_t s_optical;
static int32_t s_history;
static uint8_t s_calculate; /**< 本拍方向计算许可，消费后清零。 */

/**
 * @brief 读取模块持有的只读运行状态。
 * @return 非空只读指针，不转移所有权。
 * @note 静态对象寿命为整个程序；控制拍可更新，前台一致性读取须遵守既有停车约定。
 */
const optical_state_t *OpticalFSM_GetState(void)
{
    return &s_optical;
}

/**
 * @brief 清本模式阶段、偏差、输出与独立历史。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @note 停车起跑准备调用；保留 base_speed，不清其他模式。
 */
void OpticalFSM_Reset(void)
{
    s_optical.state = OPTICAL_WAIT;
    s_optical.error = 0;
    s_optical.output = 0;
    s_history = 0;
}

/**
 * @brief 更新光电影子状态并固定本拍方向计算许可。
 * @details 保留失线锁存、停车和等待判断；只发布许可，方向历史由Calculate消费一次。
 * @param[in] in in 为调用方提供的只读输入，必须指向满足接口容量要求的有效对象。
 * @note TIM6 每 2 ms 调用；LOST 锁存，1000 拍仍等待，1001 拍双信号有效才进入 TRACKING。
 */
void OpticalFSM_Select(const optical_inputs_t *in)
{
    /* 先刷新误差、输出和剖面；未运行、已有停车原因或双传感器无效时分别进入等待或失效状态，只有有效运行路径保持 TRACKING。 */
    s_calculate = 0;
    s_optical.error = in->error;
    s_optical.base_speed = in->base_speed;
    if (s_optical.state == OPTICAL_LOST)
    {
        return;
    }
    if ((in->running || in->stop_reason) && !in->gray_valid)
    {
        s_optical.state = OPTICAL_LOST;
        s_optical.output = 0;
        return;
    }
    if (in->stop_reason || (!in->running && s_optical.state == OPTICAL_TRACKING))
    {
        s_optical.state = OPTICAL_STOPPED;
        s_optical.output = 0;
        return;
    }
    if (in->running && in->time > APP_RUN_DELAY_COUNT && in->gray_valid && in->em_valid)
    {
        s_optical.state = OPTICAL_TRACKING;
        s_calculate = 1;
    }
}

/**
 * @brief 消费本拍许可并以最终增益计算一次光电方向。
 * @details 不推进FSM；空增益采用原输入，基础速度由调用方的最终快照给出。
 * @param[in] in 非空同一拍输入。
 * @param[in] gains 空指针采用in->gains，否则借用本拍最终方向增益。
 * @param[in] base_speed 最终基础速度，脉冲/2ms。
 * @note TIM6调用，消费后重复调用不改变历史或输出。
 */
void OpticalFSM_Calculate(const optical_inputs_t *in,
    const pid_track_gains_t *gains, int16_t base_speed)
{
    if (!s_calculate)
    {
        return;
    }
    s_calculate = 0;
    s_optical.base_speed = base_speed;
    s_optical.output = PID_DirectionStep(in->error, in->accel,
        gains ? gains : &in->gains, &s_history);
}

/**
 * @brief 按原输入完成一次光电状态选择和方向计算。
 * @details Select及Calculate各调用一次，不改变原等待/失线锁存。
 * @param[in] in 非空本拍输入。
 * @note 兼容旧入口，不接入调试覆盖。
 */
void OpticalFSM_Update(const optical_inputs_t *in)
{
    OpticalFSM_Select(in);
    OpticalFSM_Calculate(in, 0, in->base_speed);
}
