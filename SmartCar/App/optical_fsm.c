/**
 * @file optical_fsm.c
 * @brief App 光电影子状态机；TIM6 每 2 ms 观察，唯一执行口仍在 control。
 */
#include "optical_fsm.h"
#include "app_config.h"
/* TIM6 独占本模式观察和独立方向历史；起跑复位不改 base_speed。 */
static optical_state_t s_optical;
static int32_t s_history;

const optical_state_t *OpticalFSM_GetState(void)
{
    return &s_optical;
}

void OpticalFSM_Reset(void)
{
    s_optical.state = OPTICAL_WAIT;
    s_optical.error = 0;
    s_optical.output = 0;
    s_history = 0;
}

void OpticalFSM_Update(const optical_inputs_t *in)
{
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
    if (in->running && in->time > RUN_DELAY_COUNT && in->gray_valid && in->em_valid)
    {
        s_optical.state = OPTICAL_TRACKING;
        s_optical.output = PID_DirectionStep(in->error, in->accel, &in->gains, &s_history);
    }
}
