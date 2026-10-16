/**
 * @file electromagnetic_fsm.c
 * @brief App 电磁私有状态机；TIM6 观察十字和右环岛，不在观察层写硬件。
 */
/* 仅移入原 whole_test 的活动十字/预环岛/右环岛路径。
 * 本上下文独占所有判断历史；无 Motor/PID_Set/IMU_Zero 或硬件写入。 */
#include "electromagnetic_fsm.h"
#include "roundabout.h"
#include <stdlib.h>
#include <string.h>
/* TIM6 独占电磁观察历史；起跑显式复位，左右里程和方向历史均与光电隔离。 */
static electromagnetic_state_t s_em;
static int32_t s_history;

static int32_t ElectromagneticFSM_GetDistance(void);
static void ElectromagneticFSM_ClearDistance(void);
static void ElectromagneticFSM_ClearAngle(void);
static void ElectromagneticFSM_Measure(const electromagnetic_inputs_t *in);
static void ElectromagneticFSM_UpdateDirection(const electromagnetic_inputs_t *in);

const electromagnetic_state_t *ElectromagneticFSM_GetState(void)
{
    return &s_em;
}

void ElectromagneticFSM_Reset(void)
{
    memset(&s_em, 0, sizeof(s_em));
    s_history = 0;
    s_em.configuration.weights = (track_weights_t){ 15, 20, 22, 10 };
}

void ElectromagneticFSM_Accumulate(int16_t left, int16_t right)
{
    s_em.left_distance += left;
    s_em.right_distance += right;
}

/**
 * @brief 按原顺序求左右私有里程的整数平均。
 * @return 左右 int32_t 脉冲相加后除二的值，不改为增量平均。
 */
static int32_t ElectromagneticFSM_GetDistance(void)
{
    return (s_em.left_distance + s_em.right_distance) / 2;
}

/**
 * @brief 清私有双轮里程并置本拍共享里程复位请求。
 * @note 仍在原分支位置执行，后续距离判断重新读取清零后的值。
 */
static void ElectromagneticFSM_ClearDistance(void)
{
    s_em.left_distance = s_em.right_distance = 0;
    s_em.reset_distance = 1;
}

/**
 * @brief 清私有角度并置本拍共享角度复位请求。
 * @note 不在观察层直接清共享 IMU 角度。
 */
static void ElectromagneticFSM_ClearAngle(void)
{
    s_em.angle = 0;
    s_em.zero_angle = 1;
}

/**
 * @brief 计算本拍私有对称度与电感偏差。
 * @param[in] in 非空输入；adc 必须非空且至少四项，正常采集值 0～4095。
 * @note 保留加减乘除顺序与零分母返回，不修改硬件或共享执行状态。
 */
static void ElectromagneticFSM_Measure(const electromagnetic_inputs_t *in)
{
    int32_t sx = in->adc[0] + in->adc[3];
    int32_t sy = in->adc[1] + in->adc[2];
    s_em.symmetry_x = sx ? abs(in->adc[0] - in->adc[3]) * 100 / sx : 0;
    s_em.symmetry_y = sy ? abs(in->adc[1] - in->adc[2]) * 100 / sy : 0;
    s_em.error = Track_CalculateError(in->adc, &s_em.configuration.weights);
}

/**
 * @brief 使用电磁独立历史更新方向及执行转向值。
 * @param[in] in 非空本拍输入；adc、参数与单位满足电磁观察契约。
 * @note 先测量后计算，不与光电方向历史共享存储。
 */
static void ElectromagneticFSM_UpdateDirection(const electromagnetic_inputs_t *in)
{
    ElectromagneticFSM_Measure(in);
    s_em.output = PID_DirectionStep(s_em.error, in->accel, &s_em.configuration.track, &s_history);
    s_em.steering = s_em.output;
}

void ElectromagneticFSM_Update(const electromagnetic_inputs_t *in)
{
    s_em.reset_distance = s_em.zero_angle = 0;
    s_em.angle += in->angle_delta;
    if (!in->enabled)
    {
        ElectromagneticFSM_Measure(in);
        return;
    }
    Control_MergeProfile(&s_em.configuration, Control_GetKernelProfile(s_em.kernel_state));
    switch (s_em.kernel_state)
    {
        case CONTROL_KERNEL_TRACKING:
            ElectromagneticFSM_UpdateDirection(in);
            if (in->adc[1] + in->adc[2] > 2800 && s_em.symmetry_y < 25)
            {
                s_em.kernel_state = CONTROL_KERNEL_CROSSROADS;
            }
            /* 保留同一旧分支中预环岛覆盖十字的调用顺序。 */
            if (in->adc[0] + in->adc[3] > 2800)
            {
                if (in->adc[0] > in->adc[3])
                {
                    s_em.left_flag = 1;
                }
                else
                {
                    s_em.right_flag = 1;
                }
                s_em.cask_flag = 0;
                ElectromagneticFSM_ClearAngle();
                ElectromagneticFSM_ClearDistance();
                s_em.kernel_state = CONTROL_KERNEL_REISLAND;
            }
            if (in->dl1b_mm < 100)
            {
                s_em.cask_flag = 1;
            }
            break;
        case CONTROL_KERNEL_CROSSROADS:
            ElectromagneticFSM_UpdateDirection(in);
            if (s_em.output > 10)
            {
                s_em.output = 10;
            }
            if (s_em.output < -10)
            {
                s_em.output = -10;
            }
            s_em.steering = s_em.output;
            if (in->adc[1] + in->adc[2] < 2800)
            {
                s_em.kernel_state = CONTROL_KERNEL_TRACKING;
            }
            break;
        case CONTROL_KERNEL_REISLAND:
            ElectromagneticFSM_UpdateDirection(in);
            if (in->adc[1] + in->adc[2] < 300 && s_em.right_flag && !s_em.cask_flag)
            {
                s_em.roundabout_state = ROUNDABOUT_ISLAND_LPREENTER;
                s_em.kernel_state = CONTROL_KERNEL_ISLAND_R;
                s_em.output = 0;
                ElectromagneticFSM_ClearAngle();
                ElectromagneticFSM_ClearDistance();
            }
            /* 入环清零后再判断超时，不能提前缓存 distance。 */
            if (ElectromagneticFSM_GetDistance() > 5000)
            {
                s_em.kernel_state = CONTROL_KERNEL_TRACKING;
                s_em.left_flag = s_em.right_flag = 0;
                s_em.output = 0;
                ElectromagneticFSM_ClearAngle();
            }
            break;
        case CONTROL_KERNEL_ISLAND_R:
            Control_MergeProfile(&s_em.configuration, Roundabout_GetProfile(s_em.roundabout_state));
            switch (s_em.roundabout_state)
            {
                case ROUNDABOUT_ISLAND_LPREENTER:
                    ElectromagneticFSM_UpdateDirection(in);
                    if (ElectromagneticFSM_GetDistance() >= g_roundabout_enter_distance1)
                    {
                        s_em.roundabout_state = ROUNDABOUT_ISLAND_TURN_LEFT;
                        ElectromagneticFSM_ClearDistance();
                    }
                    break;
                case ROUNDABOUT_ISLAND_TURN_LEFT:
                    s_em.steering = -30;
                    if (ElectromagneticFSM_GetDistance() > 13000)
                    {
                        s_em.roundabout_state = ROUNDABOUT_ISLAND_IN;
                        ElectromagneticFSM_ClearDistance();
                    }
                    break;
                case ROUNDABOUT_ISLAND_IN:
                    ElectromagneticFSM_UpdateDirection(in);
                    if (ElectromagneticFSM_GetDistance() > 32000)
                    {
                        s_em.roundabout_state = ROUNDABOUT_ISLAND_OUT;
                        ElectromagneticFSM_ClearDistance();
                    }
                    break;
                case ROUNDABOUT_ISLAND_OUT:
                    ElectromagneticFSM_UpdateDirection(in);
                    if (ElectromagneticFSM_GetDistance() > g_roundabout_out_distance1)
                    {
                        s_em.left_flag = s_em.right_flag = 0;
                        s_em.roundabout_state = ROUNDABOUT_STATE_NORMAL;
                        s_em.kernel_state = CONTROL_KERNEL_TRACKING;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break; /* 无休眠元素入口 */
    }
}
