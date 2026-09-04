/**
  ******************************************************************************
  * @file    bsp_sampler.c
  * @brief   高速采样器：TIM15 1ms 中断（1kHz），采集编码器计数与 IMU 原始值。
  *          TIM15 未被 CubeMX 占用，此处纯寄存器配置（定时器时钟 240MHz：
  *          APB2 ÷2 后定时器倍频，PSC=240-1 → 1MHz，ARR=999 → 1ms）。
  *          NVIC 优先级 0，与 TIM6 控制环同级互不抢占，避免 SPI4 访问交错。
  ******************************************************************************
  */

/* 包含头文件 ------------------------------------------------------------------*/
#include "bsp_sampler.h"
#include "bsp_encoder.h"
#include "bsp_imu660rb.h"

/* 采样节拍到达时调用的应用层钩子（定义在 app.c，内做数据记录） */
extern void App_SampleISR(void);

/* 私有变量 ---------------------------------------------------------*/
static volatile uint32_t s_tick;            /* 1ms 节拍 */
static volatile int16_t  s_enc_l_1ms;       /* 最近 1ms 左轮计数 */
static volatile int16_t  s_enc_r_1ms;       /* 最近 1ms 右轮计数 */
static volatile int16_t  s_enc_l_sum;       /* 左轮累计（控制环 2ms 消费） */
static volatile int16_t  s_enc_r_sum;       /* 右轮累计 */
static volatile int16_t  s_gyro[3];         /* 最近 IMU 陀螺原始值 */
static volatile int16_t  s_acc[3];          /* 最近 IMU 加速度原始值 */

/* 导出函数定义 -------------------------------------------*/

/**
  * @brief  配置 TIM15 为 1ms 周期中断（不启动计数）
  */
void BSP_Sampler_Init(void)
{
  __HAL_RCC_TIM15_CLK_ENABLE();

  TIM15->CR1  = 0;                      /* 先停计数 */
  TIM15->PSC  = 240 - 1;                /* 240MHz → 1MHz */
  TIM15->ARR  = 1000 - 1;               /* 1MHz / 1000 = 1kHz（1ms） */
  TIM15->CNT  = 0;
  TIM15->EGR  = TIM_EGR_UG;             /* 装载 PSC/ARR */
  TIM15->SR   = (uint16_t)~TIM_SR_UIF;  /* 清更新标志 */
  TIM15->DIER = TIM_DIER_UIE;           /* 使能更新中断 */

  HAL_NVIC_SetPriority(TIM15_IRQn, 0, 0);   /* 与 TIM6 同级：互不抢占 */
  HAL_NVIC_EnableIRQ(TIM15_IRQn);
}

void BSP_Sampler_Start(void)
{
  s_tick = 0;
  s_enc_l_sum = 0;
  s_enc_r_sum = 0;
  TIM15->CNT = 0;
  TIM15->SR  = (uint16_t)~TIM_SR_UIF;
  TIM15->CR1 |= TIM_CR1_CEN;
}

void BSP_Sampler_Stop(void)
{
  TIM15->CR1 &= (uint16_t)~TIM_CR1_CEN;
}

uint32_t BSP_Sampler_GetTick(void)
{
  return s_tick;
}

int16_t BSP_Sampler_GetEncL_1ms(void)
{
  return s_enc_l_1ms;
}

int16_t BSP_Sampler_GetEncR_1ms(void)
{
  return s_enc_r_1ms;
}

void BSP_Sampler_GetGyroRaw(int16_t *x, int16_t *y, int16_t *z)
{
  *x = s_gyro[0];
  *y = s_gyro[1];
  *z = s_gyro[2];
}

void BSP_Sampler_GetAccRaw(int16_t *x, int16_t *y, int16_t *z)
{
  *x = s_acc[0];
  *y = s_acc[1];
  *z = s_acc[2];
}

int16_t BSP_Sampler_ConsumeEncL(void)
{
  int16_t sum = s_enc_l_sum;        /* 与 1ms 采样同优先级，不会被抢占 */
  s_enc_l_sum = 0;
  return sum;
}

int16_t BSP_Sampler_ConsumeEncR(void)
{
  int16_t sum = s_enc_r_sum;
  s_enc_r_sum = 0;
  return sum;
}

/* 中断服务 ---------------------------------------------------------*/

/**
  * @brief  TIM15 1ms 采样中断：编码器读清+累计、IMU 原始值读取、应用层钩子
  */
void TIM15_IRQHandler(void)
{
  int16_t enc_l;
  int16_t enc_r;
  int16_t raw[3];

  TIM15->SR = (uint16_t)~TIM_SR_UIF;    /* 清更新标志 */
  s_tick++;

  /* 编码器：读即清零（BSP_Encoder 已按 INVERT 宏处理符号），并做 2ms 累计 */
  enc_l = BSP_Encoder_GetLeft();
  enc_r = BSP_Encoder_GetRight();
  s_enc_l_1ms  = enc_l;
  s_enc_r_1ms  = enc_r;
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

  App_SampleISR();                      /* 应用层钩子：数据记录 */
}
