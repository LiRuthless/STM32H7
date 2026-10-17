/**
 * @file electromagnetic_fsm.h
 * @brief App 电磁私有状态机；TIM6 观察十字和右环岛，不在观察层写硬件。
 * @details 声明电磁状态机输入、状态及观察接口；完整转移规则位于 electromagnetic_fsm.c。
 */
#ifndef SMARTCAR_APP_ELECTROMAGNETIC_FSM_H
#define SMARTCAR_APP_ELECTROMAGNETIC_FSM_H
#include "control.h"

typedef struct
{
    const int16_t *adc; /**< 非空四路滤波 ADC 输入，顺序横左/竖左/竖右/横右。 */
    uint16_t dl1b_mm;   /**< 测距毫米值，沿用 BSP 无效哨兵 8192。 */
    float accel;        /**< 共享原加速度值，保持方向阻尼使用的原标度。 */
    float angle_delta;  /**< 本控制拍共享 X 轴角度增量，单位度。 */
    uint8_t enabled;    /**< 非零才推进活动元素；等待或停车时仅更新观察测量。 */
} electromagnetic_inputs_t;

typedef struct
{
    uint8_t kernel_state;            /**< 私有电磁主状态，原 CONTROL_KERNEL_* 数值。 */
    uint8_t roundabout_state;        /**< 私有环岛子状态，原 ROUNDABOUT_* 数值及右环路径。 */
    uint8_t left_flag;               /**< 原左侧预环标志，不代表启用左环岛。 */
    uint8_t right_flag;              /**< 原右侧预环标志，用于既有右入环路径。 */
    uint8_t cask_flag;               /**< 原近距障碍抑制标志，不启用路障元素。 */
    uint8_t reset_distance;          /**< 本拍请求复位共享里程，仅选中电磁执行口应用。 */
    uint8_t zero_angle;              /**< 本拍请求清共享 X 角度，仅选中电磁执行口应用。 */
    int32_t left_distance;           /**< 电磁上下文独立左轮滤波脉冲累计。 */
    int32_t right_distance;          /**< 电磁上下文独立右轮滤波脉冲累计。 */
    float angle;                     /**< 私有 X 轴角度累计，单位度，不改变共享基准。 */
    int16_t error;                   /**< 原加权电感偏差，int16_t。 */
    int16_t symmetry_x;              /**< 横电感原百分比不对称度，零分母时为零。 */
    int16_t symmetry_y;              /**< 竖电感原百分比不对称度，零分母时为零。 */
    int16_t output;                  /**< 分支处理后的方向输出，可能与先执行的 steering 不同。 */
    int16_t steering;                /**< 供唯一执行口分配速度的本拍方向值。 */
    control_profile_t configuration; /**< 私有控制配置，按位合并并保留未写组历史。 */
} electromagnetic_state_t;

/**
 * @brief 清空电磁私有上下文和方向历史，恢复原初始权重。
 * @note 起跑复位调用；不写 Motor、PID 增益或共享 IMU 角度。 完整契约见 electromagnetic_fsm.c::ElectromagneticFSM_Reset。
 */
void ElectromagneticFSM_Reset(void);

/**
 * @brief 推进原活动十字、预环岛和右环岛观察路径。
 * @note inputs 非空输入对象；adc 必须非空且至少四路，字段取值沿用既有控制契约。 TIM6 观察路径调用；不启用左环岛、跷跷板或路障，重置标志只由选中模式执行。 完整契约见
 *     electromagnetic_fsm.c::ElectromagneticFSM_Update。
 */
void ElectromagneticFSM_Update(const electromagnetic_inputs_t *inputs);

/**
 * @brief 分别累计左右轮滤波脉冲。
 * @note 左右分别写入私有 int32_t 里程，不提前改为平均值累计。 完整契约见
 *     electromagnetic_fsm.c::ElectromagneticFSM_Accumulate。
 */
void ElectromagneticFSM_Accumulate(int16_t left, int16_t right);

/**
 * @brief 读取电磁观察状态机状态。
 * @note 状态由既有控制/停车路径更新；前台要求一致快照时须在对应更新停止后读取。 完整契约见
 *     electromagnetic_fsm.c::ElectromagneticFSM_GetState。
 */
const electromagnetic_state_t *ElectromagneticFSM_GetState(void);

#endif
