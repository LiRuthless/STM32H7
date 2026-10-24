/**
 * @file track_sensor.c
 * @brief App 四路电感滤波与误差；TIM6 内存读取，原八次去极值平均不变。
 * @details 读取四路电感 ADC，完成八次去极值均值、误差和对称度计算，并发布观察状态。
 * @note 移植来源：Sirius20260718/project/code/adc.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include <stdlib.h>
#include "sensing/track_sensor.h"

/* TIM6 持有采样滤波中间量和观察结果；前台按原停车约定读取。 */
static track_state_t s_track = { .weights = { 15, 20, 22, 10 } };
static uint16_t s_adc_sum[4];
static uint16_t s_max[4];
static uint16_t s_min[4];

/* 通道映射：[0]横左 [1]竖左 [2]竖右 [3]横右 */
static const bsp_adc_channel_t s_adc_channel_map[4]
    = { BSP_ADC_IND_H_L, BSP_ADC_IND_V_L, BSP_ADC_IND_V_R, BSP_ADC_IND_H_R };

/**
 * @brief 读取模块持有的只读运行状态。
 * @return 非空只读指针，不转移所有权。
 * @note 静态对象寿命为整个程序；控制拍可更新，前台一致性读取须遵守既有停车约定。
 */
const track_state_t *Track_GetState(void)
{
    return &s_track;
}

/**
 * @brief 按原加权差分整数式计算电磁偏差。
 * @details 以横差和竖差的加权和为分子，横电感和与竖差绝对值的加权和为分母；
 *     分母为零直接返回0，否则先乘100、再整数除法并窄化至int16_t，不修改传感器状态。
 * @param[in] a 非空四路滤波电感，顺序为横左、竖左、竖右、横右。
 * @param[in] w 非空四项误差权重；不在此入口校验系数范围。
 * @return 分母为零返回 0，否则返回原 int16_t 截断偏差。
 * @note 纯计算，无阻塞，保持横差/竖差与分母乘加顺序。
 */
int16_t Track_CalculateError(const int16_t *a, const track_weights_t *w)
{
    int32_t dx = a[0] - a[3];
    int32_t dy = a[1] - a[2];
    int32_t denominator = w->xx * (a[0] + a[3]) + w->abs * labs(dy);
    return denominator ? (int16_t)(100 * (w->x * dx + w->y * dy) / denominator) : 0;
}

/**
 * @brief 按原顺序发布电磁观察量。
 * @details 先复制本拍权重，再写偏差和横、竖不对称百分比；原始值及滤波值均保持。
 * @param[in] weights 非空本拍权重。
 * @param[in] error 原归一化 int16_t 偏差。
 * @param[in] symmetry_x 横不对称百分比。
 * @param[in] symmetry_y 竖不对称百分比。
 * @note 控制拍调用，仅发布，不修改原始 ADC 或硬件。
 */
void Track_PublishObservation(
    const track_weights_t *weights, int16_t error, int16_t symmetry_x, int16_t symmetry_y)
{
    s_track.weights = *weights;
    s_track.error = error;
    s_track.symmetry_x = symmetry_x;
    s_track.symmetry_y = symmetry_y;
}

/**
 * @brief 按 x、xx、y、abs 顺序设置原权重。
 * @details 逐字段更新四个电感误差权重；不重算当前误差，新的权重从下一次误差更新开始生效。
 * @param[in] weights 非空四项 int8_t 权重，无量纲，不新增范围检查。
 * @note TIM6 剖面应用或参数加载路径调用；写 s_track.weights，不访问 ADC。
 */
void Track_SetWeights(const track_weights_t *weights)
{
    s_track.weights.x = weights->x;
    s_track.weights.xx = weights->xx;
    s_track.weights.y = weights->y;
    s_track.weights.abs = weights->abs;
}

/**
 * @brief 计算并发布旧电磁偏差及对称度。
 * @details 先计算横、竖电感差和两组对称度，再用权重构造归一化分母；分母为零时发布零偏差，否则按既有整数顺序计算并写回状态。
 * @return 原 int16_t 偏差，分母零返回 0。
 * @note 保留旧入口；分母零时发布零，不新增限幅或新滤波。
 */
int16_t Track_UpdateError(void)
{
    int16_t diff = 0;   /* 循迹偏差（带符号） */
    int32_t diff_x = 0; /* 横电感差（带符号） */
    int32_t diff_y = 0; /* 竖电感差（带符号） */
    int32_t denominator = 0;
    int32_t sum_x;
    int32_t sum_y;

    diff_x = (s_track.filtered[0] - s_track.filtered[3]); /* 横电感差（带符号） */
    diff_y = (s_track.filtered[1] - s_track.filtered[2]); /* 竖电感差（带符号） */

    sum_x = s_track.filtered[0] + s_track.filtered[3];
    sum_y = s_track.filtered[1] + s_track.filtered[2];
    s_track.symmetry_x = (sum_x == 0) ? 0 : (abs(diff_x) * 100) / sum_x;
    s_track.symmetry_y = (sum_y == 0) ? 0 : (abs(diff_y) * 100) / sum_y;

    denominator = s_track.weights.xx * (s_track.filtered[0] + s_track.filtered[3])
        + s_track.weights.abs * labs(s_track.filtered[1] - s_track.filtered[2]);

    if (denominator == 0)
    {
        s_track.error = 0;
        return 0;
    }

    diff = 100 * (s_track.weights.x * diff_x + s_track.weights.y * diff_y) / denominator;
    s_track.error = diff;

    return diff;
}

/**
 * @brief 计算并发布横竖不对称度。
 * @details 分别用横、竖两侧差的绝对值乘100除以对应和；各自和为零时发布0，不改变偏差。
 * @note 控制拍调用，电感和为零返回百分比 0，保持原整数顺序。
 */
void Track_UpdateSymmetry(void)
{
    int32_t minus_x = 0; /* 横电感差（带符号） */
    int32_t minus_y = 0; /* 竖电感差（带符号） */
    int32_t sum_x;
    int32_t sum_y;

    minus_x = (s_track.filtered[0] - s_track.filtered[3]); /* 横电感差（带符号） */
    minus_y = (s_track.filtered[1] - s_track.filtered[2]); /* 竖电感差（带符号） */

    sum_x = s_track.filtered[0] + s_track.filtered[3];
    sum_y = s_track.filtered[1] + s_track.filtered[2];
    s_track.symmetry_x = (sum_x == 0) ? 0 : (abs(minus_x) * 100) / sum_x;
    s_track.symmetry_y = (sum_y == 0) ? 0 : (abs(minus_y) * 100) / sum_y;
}

/**
 * @brief 对四路 DMA 内存各读八次并去极值平均。
 * @details 每轮按横左、竖左、竖右、横右读取DMA当前值，共八轮；各通道独立累计、
 *     去一次最大和最小后除以6，整数结果写filtered，raw保留最后一轮，内部不等待新转换。
 * @note TIM6 调用；不是八次新转换，去一最大一最小后六次整数平均，不改 BSP 采集。
 */
void Track_ReadAdc(void)
{
    uint8_t i;
    uint8_t j;

    /* 初始化每组的最大值、最小值和累加和 */
    for (i = 0; i < 4; i++)
    {
        s_max[i] = 0;              /* 最大值初始化为0 */
        s_min[i] = APP_ADC_FULL_SCALE; /* 最小值初始化为ADC最大值4095 */
        s_adc_sum[i] = 0;          /* 累加和清零 */
    }

    /* 循环采样8次 */
    for (i = 0; i < 8; i++)
    {
        /* 依次读取四路ADC通道的原始值 */
        s_track.raw[0] = BSP_ADC_Read(s_adc_channel_map[0]);
        s_track.raw[1] = BSP_ADC_Read(s_adc_channel_map[1]);
        s_track.raw[2] = BSP_ADC_Read(s_adc_channel_map[2]);
        s_track.raw[3] = BSP_ADC_Read(s_adc_channel_map[3]);

        /* 对四路数据分别更新累加和、最大值、最小值 */
        for (j = 0; j < 4; j++)
        {
            s_adc_sum[j] += s_track.raw[j]; /* 累加当前采样值 */
            if (s_max[j] < s_track.raw[j])
            {
                s_max[j] = s_track.raw[j];
            }
            if (s_min[j] > s_track.raw[j])
            {
                s_min[j] = s_track.raw[j];
            }
        }
    }

    /* 去掉每组的最大值和最小值后，剩余6次求平均 */
    for (i = 0; i < 4; i++)
    {
        s_adc_sum[i] -= s_max[i];
        s_adc_sum[i] -= s_min[i];
        s_track.filtered[i] = s_adc_sum[i] / 6;
    }
}

/**
 * @brief 保留 ADC 初始化薄封装。
 * @note 上电前台调用；校准与 DMA 启动仍由 BSP 原实现负责。
 */
void Track_InitAdc(void)
{
    BSP_ADC_Init();
}
