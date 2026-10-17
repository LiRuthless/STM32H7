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

static int32_t ElectromagneticFSM_GetDistance(void);
static void ElectromagneticFSM_ClearDistance(void);
static void ElectromagneticFSM_ClearAngle(void);
static void ElectromagneticFSM_Measure(const electromagnetic_inputs_t *in);
static void ElectromagneticFSM_UpdateDirection(const electromagnetic_inputs_t *in);

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
 * @brief 使用电磁独立历史更新方向及执行转向值。
 * @details 按现有调用顺序使用 ElectromagneticFSM_Measure、PID_DirectionStep 完成本函数处理。
 * @param[in] in 非空本拍输入；adc、参数与单位满足电磁观察契约。
 * @note 先测量后计算，不与光电方向历史共享存储。
 */
static void ElectromagneticFSM_UpdateDirection(const electromagnetic_inputs_t *in)
{
    ElectromagneticFSM_Measure(in);
    s_em.output = PID_DirectionStep(s_em.error, in->accel, &s_em.configuration.track, &s_history);
    s_em.steering = s_em.output;
}

/**
 * @brief 推进原活动十字、预环岛和右环岛观察路径。
 * @details 按现有调用顺序使用
 *     ElectromagneticFSM_Measure、Control_MergeProfile、Control_GetKernelProfile、
 *     ElectromagneticFSM_UpdateDirection
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] in in 为调用方提供的只读输入，必须指向满足接口容量要求的有效对象。
 * @note TIM6 观察路径调用；不启用左环岛、跷跷板或路障，重置标志只由选中模式执行。
 */
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
