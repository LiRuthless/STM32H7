/**
 * @file electromagnetic_fsm.c
 * @brief App 电磁私有状态机；TIM6 观察十字和右环岛，不在观察层写硬件。
 * @details 根据电感、IMU 与里程输入推进电磁赛道状态机，并生成控制剖面和保护观察结果。
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
/* Select记录原分支的计算及后处理许可，Calculate消费一次。 */
static uint8_t s_calculate;
static uint8_t s_clamp;
static uint8_t s_zero_output;

static int32_t ElectromagneticFSM_GetDistance(void);
static void ElectromagneticFSM_ClearDistance(void);
static void ElectromagneticFSM_ClearAngle(void);
static void ElectromagneticFSM_Measure(const electromagnetic_inputs_t *in);

/**
 * @brief 读取电磁观察状态机状态。
 * @return 非空模块静态对象的只读指针，生命周期覆盖整个运行期。
 * @note 状态由既有控制/停车路径更新；前台要求一致快照时须在对应更新停止后读取。
 */
const electromagnetic_state_t *ElectromagneticFSM_GetState(void)
{
    return &s_em;
}

/**
 * @brief 清空电磁私有上下文和方向历史，恢复原初始权重。
 * @details 按现有调用顺序使用 memset 完成本函数处理。
 * @note 起跑复位调用；不写 Motor、PID 增益或共享 IMU 角度。
 */
void ElectromagneticFSM_Reset(void)
{
    memset(&s_em, 0, sizeof(s_em));
    s_history = 0;
    s_em.configuration.weights = (track_weights_t){ 15, 20, 22, 10 };
}

/**
 * @brief 分别累计左右轮滤波脉冲。
 * @details 把本拍左右轮滤波脉冲分别累加到电磁私有 int32_t 里程，不先求平均，以保留两侧独立距离。
 * @param[in] left 左轮本拍滤波编码器计数，单位为脉冲/2 ms。
 * @param[in] right 右轮本拍滤波编码器计数，单位为脉冲/2 ms。
 * @note 左右分别写入私有 int32_t 里程，不提前改为平均值累计。
 */
void ElectromagneticFSM_Accumulate(int16_t left, int16_t right)
{
    s_em.left_distance += left;
    s_em.right_distance += right;
}

/**
 * @brief 按原顺序求左右私有里程的整数平均。
 * @details 返回当前电磁状态机累计里程，单位为编码器脉冲；值由控制拍的里程累计入口更新。
 * @return 左右 int32_t 脉冲相加后除二的值，不改为增量平均。
 * @note 仅由 TIM6 电磁观察路径调用并读取 s_em，不访问硬件或阻塞。
 */
static int32_t ElectromagneticFSM_GetDistance(void)
{
    return (s_em.left_distance + s_em.right_distance) / 2;
}

/**
 * @brief 清私有双轮里程并置本拍共享里程复位请求。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @note 仍在原分支位置执行，后续距离判断重新读取清零后的值。
 */
static void ElectromagneticFSM_ClearDistance(void)
{
    s_em.left_distance = s_em.right_distance = 0;
    s_em.reset_distance = 1;
}

/**
 * @brief 清私有角度并置本拍共享角度复位请求。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @note 不在观察层直接清共享 IMU 角度。
 */
static void ElectromagneticFSM_ClearAngle(void)
{
    s_em.angle = 0;
    s_em.zero_angle = 1;
}

/**
 * @brief 计算本拍私有对称度与电感偏差。
 * @details 按现有调用顺序使用 abs、Track_CalculateError 完成本函数处理。
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
 * @brief 选择原活动元素状态和策略，不推进方向历史。
 * @details 原分支测量后固定计算许可，状态转移仍按原条件顺序执行；方向计算移到
 *     Calculate，以便先判断切换拍是否允许调试。策略只合并原状态表，不混入调试值。
 * @param[in] in in 为调用方提供的只读输入，必须指向满足接口容量要求的有效对象。
 * @note TIM6 观察路径调用；不启用左环岛、跷跷板或路障，重置标志只由选中模式执行。
 */
void ElectromagneticFSM_Select(const electromagnetic_inputs_t *in)
{
    s_calculate = s_clamp = s_zero_output = 0;
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
            ElectromagneticFSM_Measure(in);
            s_calculate = 1;
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
            ElectromagneticFSM_Measure(in);
            s_calculate = 1;
            s_clamp = 1;
            if (in->adc[1] + in->adc[2] < 2800)
            {
                s_em.kernel_state = CONTROL_KERNEL_TRACKING;
            }
            break;
        case CONTROL_KERNEL_REISLAND:
            ElectromagneticFSM_Measure(in);
            s_calculate = 1;
            if (in->adc[1] + in->adc[2] < 300 && s_em.right_flag && !s_em.cask_flag)
            {
                s_em.roundabout_state = ROUNDABOUT_ISLAND_LPREENTER;
                s_em.kernel_state = CONTROL_KERNEL_ISLAND_R;
                s_zero_output = 1;
                ElectromagneticFSM_ClearAngle();
                ElectromagneticFSM_ClearDistance();
            }
            /* 入环清零后再判断超时，不能提前缓存 distance。 */
            if (ElectromagneticFSM_GetDistance() > 5000)
            {
                s_em.kernel_state = CONTROL_KERNEL_TRACKING;
                s_em.left_flag = s_em.right_flag = 0;
                s_zero_output = 1;
                ElectromagneticFSM_ClearAngle();
            }
            break;
        case CONTROL_KERNEL_ISLAND_R:
            Control_MergeProfile(&s_em.configuration, Roundabout_GetProfile(s_em.roundabout_state));
            switch (s_em.roundabout_state)
            {
                case ROUNDABOUT_ISLAND_LPREENTER:
                    ElectromagneticFSM_Measure(in);
                    s_calculate = 1;
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
                    ElectromagneticFSM_Measure(in);
                    s_calculate = 1;
                    if (ElectromagneticFSM_GetDistance() > 32000)
                    {
                        s_em.roundabout_state = ROUNDABOUT_ISLAND_OUT;
                        ElectromagneticFSM_ClearDistance();
                    }
                    break;
                case ROUNDABOUT_ISLAND_OUT:
                    ElectromagneticFSM_Measure(in);
                    s_calculate = 1;
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

/**
 * @brief 按本拍许可计算一次方向，随后执行原分支后处理。
 * @details 消费Select许可；十字限幅在转向发布前，预环清零在转向发布后，保留原差异。
 * @param[in] in 非空同一拍输入，沿用Select的加速度单位。
 * @param[in] gains 空指针使用原策略，否则使用本拍已复核的选中模式增益。
 * @note TIM6调用；重复Calculate不再次推进方向历史，不修改原配置。
 */
void ElectromagneticFSM_Calculate(const electromagnetic_inputs_t *in,
    const pid_track_gains_t *gains)
{
    if (!s_calculate)
    {
        return;
    }
    s_calculate = 0;
    s_em.output = PID_DirectionStep(s_em.error, in->accel,
        gains ? gains : &s_em.configuration.track, &s_history);
    if (s_clamp)
    {
        if (s_em.output > 10)
        {
            s_em.output = 10;
        }
        if (s_em.output < -10)
        {
            s_em.output = -10;
        }
    }
    s_em.steering = s_em.output;
    if (s_zero_output)
    {
        s_em.output = 0;
    }
}

/**
 * @brief 保留原策略的一次完整观察包装。
 * @details 先Select推进状态，再Calculate推进方向，不接入调试覆盖。
 * @param[in] in 非空本拍输入，adc至少四项。
 * @note 旧入口及宿主等价验证调用，不额外执行硬件输出。
 */
void ElectromagneticFSM_Update(const electromagnetic_inputs_t *in)
{
    ElectromagneticFSM_Select(in);
    ElectromagneticFSM_Calculate(in, 0);
}
