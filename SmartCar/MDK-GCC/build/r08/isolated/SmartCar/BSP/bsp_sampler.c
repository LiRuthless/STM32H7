/**
 * @file bsp_sampler.c
 * @brief 高速采样器：TIM15 1ms 中断（1kHz），采集编码器计数与 IMU 原始值。
 * @details 配置 TIM15 一毫秒采样中断，读清编码器、缓存 IMU 并向应用发布日志采样钩子。
 */
#include "bsp_sampler.h"
#include "bsp_encoder.h"
#include "bsp_imu660rb.h"

/* 采样节拍到达时调用的应用层钩子（定义在 app.c，内做数据记录） */
extern void App_SampleISR(void);
/* 1ms 节拍 */
static volatile uint32_t s_tick;
/* 最近 1ms 左轮计数 */
static volatile int16_t s_enc_l_1ms;
/* 最近 1ms 右轮计数 */
static volatile int16_t s_enc_r_1ms;
/* 左轮累计（控制环 2ms 消费） */
static volatile int16_t s_enc_l_sum;
/* 右轮累计 */
static volatile int16_t s_enc_r_sum;
/* 最近 IMU 陀螺原始值 */
static volatile int16_t s_gyro[3];
/* 最近 IMU 加速度原始值 */
static volatile int16_t s_acc[3];
/**
 * @brief 配置 TIM15 一毫秒周期与优先级零，保持未启动。
 * @details 按现有调用顺序使用 __HAL_RCC_TIM15_CLK_ENABLE、HAL_NVIC_SetPriority、HAL_NVIC_EnableIRQ 完成本函数处理。
 * @note 启动主循环调用；TIM15 与 TIM6 同级，保证 SPI4 及累计量访问不抢占。
 */
void BSP_Sampler_Init(void)
{
    __HAL_RCC_TIM15_CLK_ENABLE();

    TIM15->CR1 = 0;        /* 先停计数 */
    TIM15->PSC = 240 - 1;  /* 240MHz → 1MHz */
    TIM15->ARR = 1000 - 1; /* 1MHz / 1000 = 1kHz（1ms） */
    TIM15->CNT = 0;
    TIM15->EGR = TIM_EGR_UG;           /* 装载 PSC/ARR */
    TIM15->SR = (uint16_t)~TIM_SR_UIF; /* 清更新标志 */
    TIM15->DIER = TIM_DIER_UIE;        /* 使能更新中断 */

    HAL_NVIC_SetPriority(TIM15_IRQn, 0, 0); /* 与 TIM6 同级：互不抢占 */
    HAL_NVIC_EnableIRQ(TIM15_IRQn);
}

/**
 * @brief 清零采样拍与轮脉冲累计并启动 TIM15。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @note 起跑前调用；保留计数器、状态标志及使能的写入次序。
 */
void BSP_Sampler_Start(void)
{
    s_tick = 0;
    s_enc_l_sum = 0;
    s_enc_r_sum = 0;
    TIM15->CNT = 0;
    TIM15->SR = (uint16_t)~TIM_SR_UIF;
    TIM15->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief 关闭 TIM15 计数使采样停止。
 * @note 安全停车路径可调用，不改已有采样值。
 */
void BSP_Sampler_Stop(void)
{
    TIM15->CR1 &= (uint16_t)~TIM_CR1_CEN;
}

/**
 * @brief 读取最近采样节拍。
 * @details 读取 TIM15 自最近一次启动以来累计的一毫秒节拍；uint32_t 溢出按自然回绕处理。
 * @return 自启动以来一毫秒拍数，uint32_t 自然回绕。
 * @note TIM15 写、其他上下文读；单次 32 位读取，不重置计数。
 */
uint32_t BSP_Sampler_GetTick(void)
{
    return s_tick;
}

/**
 * @brief 读取左轮最近一毫秒脉冲。
 * @return 已按编码器方向配置处理的 int16_t 脉冲计数。
 * @note TIM15 写入；TIM6 同级读取，前进为正。
 */
int16_t BSP_Sampler_GetEncL_1ms(void)
{
    return s_enc_l_1ms;
}

/**
 * @brief 读取右轮最近一毫秒脉冲。
 * @return 已按编码器方向配置处理的 int16_t 脉冲计数。
 * @note TIM15 写入；TIM6 同级读取，前进为正。
 */
int16_t BSP_Sampler_GetEncR_1ms(void)
{
    return s_enc_r_1ms;
}

/**
 * @brief 读取三轴陀螺仪原始值。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @param[out] x X 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] y Y 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] z Z 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @note TIM15 发布、TIM6 同级读取；前台应在采样停止后读取一致快照。
 */
void BSP_Sampler_GetGyroRaw(int16_t *x, int16_t *y, int16_t *z)
{
    *x = s_gyro[0];
    *y = s_gyro[1];
    *z = s_gyro[2];
}

/**
 * @brief 读取三轴加速度原始值。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @param[out] x X 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] y Y 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] z Z 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @note TIM15 发布、TIM6 同级读取；前台应在采样停止后读取一致快照。
 */
void BSP_Sampler_GetAccRaw(int16_t *x, int16_t *y, int16_t *z)
{
    *x = s_acc[0];
    *y = s_acc[1];
    *z = s_acc[2];
}

/**
 * @brief 取出并清零左轮累计脉冲。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @return 自上次消费起累计的 int16_t 脉冲计数，前进为正。
 * @note TIM6 控制拍调用；与 TIM15 同级保证读清过程不被采样打断。
 */
int16_t BSP_Sampler_ConsumeEncL(void)
{
    int16_t sum = s_enc_l_sum; /* 与 1ms 采样同优先级，不会被抢占 */
    s_enc_l_sum = 0;
    return sum;
}

/**
 * @brief 取出并清零右轮累计脉冲。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @return 自上次消费起累计的 int16_t 脉冲计数，前进为正。
 * @note TIM6 控制拍调用；与 TIM15 同级保证读清过程不被采样打断。
 */
int16_t BSP_Sampler_ConsumeEncR(void)
{
    int16_t sum = s_enc_r_sum;
    s_enc_r_sum = 0;
    return sum;
}
/**
 * @brief TIM15 一毫秒采样中断入口。
 * @details 先清更新标志并推进一毫秒节拍，再读清两路编码器、累计到控制窗口，随后顺序读取陀螺和加速度，最后调用应用采样钩子，使日志看到同一拍已发布的传感器缓存。
 * @note 与 TIM6 同优先级；先更新编码器和 IMU 缓存，再调用 App_SampleISR 发布日志。
 */
void TIM15_IRQHandler(void)
{
    int16_t enc_l;
    int16_t enc_r;
    int16_t raw[3];

    TIM15->SR = (uint16_t)~TIM_SR_UIF; /* 清更新标志 */
    s_tick++;

    /* 编码器：读即清零（BSP_Encoder 已按 INVERT 宏处理符号），并做 2ms 累计 */
    enc_l = BSP_Encoder_GetLeft();
    enc_r = BSP_Encoder_GetRight();
    s_enc_l_1ms = enc_l;
    s_enc_r_1ms = enc_r;
    s_enc_l_sum += enc_l;
    s_enc_r_sum += enc_r;

    /* IMU：陀螺+加速度原始值（SPI4 10MHz，约 30µs） */
    BSP_IMU660RB_GetGyro(&raw[0], &raw[1], &raw[2]);
    s_gyro[0] = raw[0];
    s_gyro[1] = raw[1];
    s_gyro[2] = raw[2];
    BSP_IMU660RB_GetAcc(&raw[0], &raw[1], &raw[2]);
    s_acc[0] = raw[0];
    s_acc[1] = raw[1];
    s_acc[2] = raw[2];

    App_SampleISR(); /* 应用层钩子：数据记录 */
}
