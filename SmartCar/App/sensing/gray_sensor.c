/**
 * @file gray_sensor.c
 * @brief App 标定、归一化与质心；采集仍由 BSP 完整帧接口提供。
 * @details 接收 BSP 完整灰度帧，校验手工标定、归一化各通道并计算质心偏差，再原子式发布应用状态。
 */
/* 采集来源 LQ_Track.c V2.0.0，chiusir/北京龙邱科技有限公司 (C)2025-2026，
 * GPL-3.0；版权、原许可证声明及 SHA256 见 BSP/bsp_gray.c。
 * 本模块增加 Phase14 手工标定和质心，百分比采用均值*100/4095。 */
#include "sensing/gray_sensor.h"
#include "sensing/gray_calibration.h"
#include "bsp.h"
#include <stdio.h>
#include <string.h>
/* TIM6 或停车采样更新；失败不发布半帧，前台显示/日志遵守既有快照契约。 */
static gray_state_t s_gray;

/**
 * @brief 读取模块持有的只读运行状态。
 * @return 非空只读指针，不转移所有权。
 * @note 静态对象寿命为整个程序；控制拍可更新，前台一致性读取须遵守既有停车约定。
 */
const gray_state_t *Gray_GetState(void)
{
    return &s_gray;
}

/**
 * @brief 读取十六路黑底实测标定表。
 * @return 非空十六元素指针，ADC 0..4095 或未标定 -1。
 * @note 静态只读数组，ch0 最左、ch15 最右；-1 表示未填写，不提供示例值。
 */
const int16_t *Gray_GetBlack(void)
{
    return s_gray_black;
}

/**
 * @brief 读取十六路白线实测标定表。
 * @return 非空十六元素指针，ADC 0..4095 或未标定 -1。
 * @note 静态只读数组，ch0 最左、ch15 最右；-1 表示未填写，不提供示例值。
 */
const int16_t *Gray_GetWhite(void)
{
    return s_gray_white;
}

/**
 * @brief 逐路检查黑白标定范围和绝对差。
 * @details 逐路检查黑白值均在0..4095且绝对差至少200；每条失败只设置其掩码位，
 *     遍历完十六路后一次返回完整失败位图，不读取模块状态。
 * @param[in] black 非空十六元素黑底数组，ADC 0..4095。
 * @param[in] white 非空十六元素白线数组，ADC 0..4095。
 * @return 位 i 为 1 表示该路无效，0 表示全部有效。
 * @note 纯计算，无阻塞；差绝对值必须 >=200。
 */
uint16_t Gray_ValidateCalibration(const int16_t *black, const int16_t *white)
{
    /* 逐通道检查黑白值均在 12 位范围且绝对差至少为 200；每个失败通道设置对应掩码位。 */
    uint16_t mask = 0;
    uint8_t i;
    for (i = 0; i < 16; ++i)
    {
        int32_t delta = (int32_t)white[i] - black[i];
        if (black[i] < 0 || black[i] > 4095 || white[i] < 0 || white[i] > 4095
            || (delta > -200 && delta < 200))
        {
            mask |= (uint16_t)(1u << i);
        }
    }
    return mask;
}

/**
 * @brief 更新全部标定有效标志，可逐路报告错误。
 * @details 先由静态黑白表求失败位图并更新总有效标志；report非零才按通道顺序输出
 *     每条错误，报告不影响返回值，不修改标定表。
 * @param[in] report 0 静默，非零通过 UART 报告每条无效通道。
 * @return 全部有效为 1，否则为 0。
 * @note 报告路径为启动前前台阻塞输出；控制拍仅传 0。
 */
uint8_t Gray_CheckCalibration(uint8_t report)
{
    /* 先计算十六位失败掩码并发布总有效标志；report 非零时再逐位输出每个无效通道。 */
    uint16_t mask = Gray_ValidateCalibration(s_gray_black, s_gray_white);
    uint8_t i;
    s_gray.calibration_valid = (mask == 0);
    if (report)
    {
        for (i = 0; i < 16; ++i)
        {
            if (mask & (1u << i))
            {
                char text[48];
                (void)snprintf(text, sizeof(text), "gray calibration invalid: ch%u\r\n", i);
                BSP_UART_WriteString(text);
            }
        }
    }
    return s_gray.calibration_valid;
}

/**
 * @brief 按原整数顺序将原始值归一化。
 * @details 先拒绝范围无效或对比度不足的标定，再以1000*(raw-black)/(white-black)
 *     做有符号整数归一化；低于50清零、高于1000截顶，其余返回千分强度，不写模块状态。
 * @param[in] raw ADC 原始均值，0..4095。
 * @param[in] black 本路黑底实测 ADC 值。
 * @param[in] white 本路白线实测 ADC 值。
 * @return 强度 0..1000。
 * @note 标定无效返回 0；q<50 清零，q>1000 截顶，保留极性和整数截断。
 */
uint16_t Gray_Normalize(uint16_t raw, int16_t black, int16_t white)
{
    /* 先拒绝越界或对比度不足的标定，再按标定极性计算千分强度；低于 50 清零，高于 1000 截顶。 */
    int32_t delta = (int32_t)white - black;
    int32_t q;
    if (black < 0 || black > 4095 || white < 0 || white > 4095 || (delta > -200 && delta < 200))
    {
        return 0;
    }
    q = 1000 * ((int32_t)raw - black) / delta;
    if (q < 50)
    {
        return 0;
    }
    if (q > 1000)
    {
        return 1000;
    }
    return (uint16_t)q;
}

/**
 * @brief 将 16 路归一化强度转换为带有效标志的循迹偏差。
 * @details 通道 0 对应最左探头，通道 15 对应最右探头，位置权重为 15-2*i。
 *     对强度做质心加权后归一化为有符号百分标度：左侧信号偏强为正，右侧偏强为负。
 *     有线判断单独使用最大强度与总和，防止无信号时返回的零偏差被误认为居中。
 * @param[in] strength 非空 16 元素数组，每路为 0..1000 的归一化强度，无物理量单位。
 * @param[out] line_valid 非空指针，每次调用写入；最大强度 >=200 且总和非零时为 1，否则 0。
 * @return 偏差 -100..100；总和为零返回 0。结果必须与 line_valid 一起使用。
 * @pre 调用方提供有效数组和输出指针；本函数不校验空指针或额外钳位越界强度。
 * @note 只做本地计算并写 line_valid，不修改模块状态、不阻塞；归一化流程的主循环或
 *     TIM6 调用路径均可使用。200 是既有判定门限，本轮不声称它来自新的实测整定。
 */
int16_t Gray_ComputeError(const uint16_t *strength, uint8_t *line_valid)
{
    int32_t sum = 0;
    int32_t weighted = 0;
    uint16_t maximum = 0;
    uint8_t i;
    /* 同时累计总强度、位置加权和及最大单路强度，后者独立决定是否达到有线门限。 */
    for (i = 0; i < 16; ++i)
    {
        sum += strength[i];
        weighted += (15 - 2 * (int32_t)i) * strength[i];
        if (strength[i] > maximum)
        {
            maximum = strength[i];
        }
    }
    /* 最大值达到 200 且总强度非零才判有线；偏差为零本身不能区分居中与完全无信号。 */
    *line_valid = (maximum >= 200 && sum != 0);

    /* 权重从左 +15 到右 -15；除以 15 倍总强度归一化到 ±100，整数除法向零截断。 */
    return sum ? (int16_t)(100 * weighted / (15 * sum)) : 0;
}

/**
 * @brief 复位 BSP 和 App 完整帧序号。
 * @details 先复位BSP帧序号，再清App已发布序号；原始值、耗时、偏差和有效标志均保留。
 * @note 停车起跑准备调用，不清其他帧字段。
 */
void Gray_ResetSequence(void)
{
    BSP_Gray_ResetSequence();
    s_gray.sequence = 0;
}

/**
 * @brief 清模块状态，初始化 BSP 并报告标定。
 * @details 先清唯一灰度状态，再初始化完整帧采集BSP，最后执行带UART报告的标定校验；
 *     初始化不会自动填写或修正手工黑白表。
 * @note 启动前前台调用；保留 memset、BSP 初始化、标定报告顺序。
 */
void Gray_Init(void)
{
    memset(&s_gray, 0, sizeof(s_gray));
    BSP_Gray_Init();
    Gray_CheckCalibration(1);
}

/**
 * @brief 读取 BSP 整帧并发布标定和质心。
 * @details 先读取一个 BSP 完整帧并刷新标定状态；采集失败时只发布失败码，成功时才复制
 *     原始帧、序号与耗时，再逐路归一化并计算质心。标定无效会在计算后撤销有效线标志。
 * @note TIM6 调用；失败仅更新采集状态和标定标志，保留上一整帧及有效线标志。
 */
void Gray_Update(void)
{
    bsp_gray_frame_t frame;
    uint8_t i;
    /* 先取得完整帧并刷新标定门禁；采集失败不覆盖上一帧的原始值、序号或质心。 */
    s_gray.acquisition_status = (uint8_t)BSP_Gray_ReadFrame(&frame);
    Gray_CheckCalibration(0);
    if (s_gray.acquisition_status != BSP_GRAY_OK)
    {
        return;
    }
    /* 采集成功后才发布 BSP 帧元数据，并逐路生成显示百分比和标定强度。 */
    memcpy(s_gray.raw, frame.raw, sizeof(frame.raw));
    s_gray.sequence = frame.sequence;
    s_gray.scan_cycles = frame.scan_cycles;
    for (i = 0; i < 16; ++i)
    {
        s_gray.percent[i] = (uint16_t)((uint32_t)frame.raw[i] * 100u / 4095u);
        s_gray.strength[i] = Gray_Normalize(frame.raw[i], s_gray_black[i], s_gray_white[i]);
    }
    /* 最后发布质心与有效线标志；未完成真实标定时强制禁止起跑和运行。 */
    s_gray.error = Gray_ComputeError(s_gray.strength, &s_gray.line_valid);
    if (!s_gray.calibration_valid)
    {
        s_gray.line_valid = 0;
    }
}
