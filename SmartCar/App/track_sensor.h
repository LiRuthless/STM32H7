/**
 * @file track_sensor.h
 * @brief App 四路电感滤波与误差；TIM6 内存读取，原八次去极值平均不变。
 * @details 声明电感状态、权重和误差计算接口；完整采样与计算规则位于 track_sensor.c。
 * @note 移植来源：Sirius20260718/project/code/adc.h，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#ifndef SMARTCAR_APP_TRACK_SENSOR_H
#define SMARTCAR_APP_TRACK_SENSOR_H

#include <stdint.h>

typedef struct
{
    int8_t x;   /**< 横差权重，无量纲有符号系数。 */
    int8_t xx;  /**< 横和分母权重，无量纲有符号系数。 */
    int8_t y;   /**< 竖差权重，无量纲有符号系数。 */
    int8_t abs; /**< 竖差绝对值分母权重，无量纲有符号系数。 */
} track_weights_t;

typedef struct
{
    uint16_t raw[4];         /**< 本轮最后一次 ADC 内存读值，横左/竖左/竖右/横右。 */
    int16_t filtered[4];     /**< 八读去一最大一最小后六读整数均值，ADC 标度。 */
    int16_t error;           /**< 原加权归一化偏差，int16_t，分母零时为 0。 */
    int16_t symmetry_x;      /**< 横电感不对称百分比，横和为零时为 0。 */
    int16_t symmetry_y;      /**< 竖电感不对称百分比，竖和为零时为 0。 */
    track_weights_t weights; /**< 最近发布的四项权重；配置表按选定位部分覆写。 */
} track_state_t;

/**
 * @brief 读取模块持有的只读运行状态。
 * @note 静态对象寿命为整个程序；控制拍可更新，前台一致性读取须遵守既有停车约定。 完整契约见 track_sensor.c::Track_GetState。
 */
const track_state_t *Track_GetState(void);

/**
 * @brief 按 x、xx、y、abs 顺序设置原权重。
 * @note weights 非空四项 int8_t 权重，无量纲，不新增范围检查。 完整契约见 track_sensor.c::Track_SetWeights。
 */
void Track_SetWeights(const track_weights_t *weights);

/**
 * @brief 按原加权差分整数式计算电磁偏差。
 * @note filtered 非空四路滤波 ADC，横左/竖左/竖右/横右。 weights 非空四项 int8_t 权重。 纯计算，无阻塞，保持横差/竖差与分母乘加顺序。 完整契约见
 *     track_sensor.c::Track_CalculateError。
 */
int16_t Track_CalculateError(const int16_t *filtered, const track_weights_t *weights);

/**
 * @brief 按原顺序发布电磁观察量。
 * @note weights 非空本拍权重。 控制拍调用，仅发布，不修改原始 ADC 或硬件。 完整契约见 track_sensor.c::Track_PublishObservation。
 */
void Track_PublishObservation(
    const track_weights_t *weights, int16_t error, int16_t symmetry_x, int16_t symmetry_y);

/**
 * @brief 计算并发布旧电磁偏差及对称度。
 * @note 保留旧入口；分母零时发布零，不新增限幅或新滤波。 完整契约见 track_sensor.c::Track_UpdateError。
 */
int16_t Track_UpdateError(void);

/**
 * @brief 计算并发布横竖不对称度。
 * @note 控制拍调用，电感和为零返回百分比 0，保持原整数顺序。 完整契约见 track_sensor.c::Track_UpdateSymmetry。
 */
void Track_UpdateSymmetry(void);

/**
 * @brief 保留 ADC 初始化薄封装。
 * @note 上电前台调用；校准与 DMA 启动仍由 BSP 原实现负责。 完整契约见 track_sensor.c::Track_InitAdc。
 */
void Track_InitAdc(void);

/**
 * @brief 对四路 DMA 内存各读八次并去极值平均。
 * @note TIM6 调用；不是八次新转换，去一最大一最小后六次整数平均，不改 BSP 采集。 完整契约见 track_sensor.c::Track_ReadAdc。
 */
void Track_ReadAdc(void);

#endif
