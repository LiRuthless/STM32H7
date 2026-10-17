/**
 * @file optical_fsm.c
 * @brief App 光电影子状态机；TIM6 每 2 ms 观察，唯一执行口仍在 control。
 * @details 根据灰度质心与有效性推进光电普通循迹状态，并生成控制剖面和失线保护结果。
 */
#include "optical_fsm.h"
#include "app_config.h"
/* TIM6 独占本模式观察和独立方向历史；起跑复位不改 base_speed。 */
static optical_state_t s_optical;
static int32_t s_history;

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
 * @brief 更新光电影子状态及方向计算。
 * @details 按现有调用顺序使用 PID_DirectionStep 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] in in 为调用方提供的只读输入，必须指向满足接口容量要求的有效对象。
 * @note TIM6 每 2 ms 调用；LOST 锁存，1000 拍仍等待，1001 拍双信号有效才进入 TRACKING。
 */
void OpticalFSM_Update(const optical_inputs_t *in)
{
    /* 先刷新误差、输出和剖面；未运行、已有停车原因或双传感器无效时分别进入等待或失效状态，只有有效运行路径保持 TRACKING。 */
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
        s_optical.output = PID_DirectionStep(in->error, in->accel, &in->gains, &s_history);
    }
}
