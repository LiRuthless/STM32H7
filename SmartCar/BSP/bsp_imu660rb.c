/**
  ******************************************************************************
  * @file    bsp_imu660rb.c
  * @brief   IMU660RB（LSM6DSR 类）驱动：硬件 SPI4 + CS(PD3)，
  *          寄存器配置序列移植自逐飞 zf_device_imu660rb.c。
  *          陀螺 ±2000dps（原始值/14.3 = °/s），加速度 ±8g（原始值/4098 = g）。
  ******************************************************************************
  */

/* 包含头文件 ------------------------------------------------------------------*/
#include "bsp_imu660rb.h"
#include "spi.h"

/* 私有宏定义 ------------------------------------------------------------------*/
#define IMU_SPI_TIMEOUT_MS      (10u)       /* SPI 传输超时 */

#define IMU660RB_SPI_R          (0x80u)     /* 读寄存器地址掩码 */
#define IMU660RB_TIMEOUT_COUNT  (0x00FFu)   /* 自检超时计数（同 zf） */

#define IMU660RB_CHIP_ID        (0x0Fu)     /* WHO_AM_I，读回应为 0x6B */
#define IMU660RB_CHIP_ID_VALUE  (0x6Bu)

#define IMU660RB_INT1_CTRL      (0x0Du)
#define IMU660RB_CTRL1_XL       (0x10u)
#define IMU660RB_CTRL2_G        (0x11u)
#define IMU660RB_CTRL3_C        (0x12u)
#define IMU660RB_CTRL4_C        (0x13u)
#define IMU660RB_CTRL5_C        (0x14u)
#define IMU660RB_CTRL6_C        (0x15u)
#define IMU660RB_CTRL7_G        (0x16u)
#define IMU660RB_CTRL9_XL       (0x18u)

#define IMU660RB_ACC_ADDRESS    (0x28u)     /* 加速度数据起始寄存器 */
#define IMU660RB_GYRO_ADDRESS   (0x22u)     /* 陀螺仪数据起始寄存器 */

#define IMU660RB_ACC_SAMPLE     (0x8Cu)     /* ±8G，ODR 1.66kHz（采集提速：1ms 采样每拍取到新数据） */
#define IMU660RB_GYR_SAMPLE     (0x8Cu)     /* ±2000dps，ODR 1.66kHz */

/* 私有函数定义 -------------------------------------------*/

/* 片选拉低/拉高 */
static void imu_cs(uint8_t level)
{
  HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin,
                    level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* 写单个寄存器 */
static void imu_write_register(uint8_t reg, uint8_t dat)
{
  uint8_t buf[2] = { reg, dat };
  imu_cs(0);
  (void)HAL_SPI_Transmit(&hspi4, buf, 2, IMU_SPI_TIMEOUT_MS);
  imu_cs(1);
}

/* 读单个寄存器 */
static uint8_t imu_read_register(uint8_t reg)
{
  uint8_t addr = (uint8_t)(reg | IMU660RB_SPI_R);
  uint8_t dat  = 0;
  imu_cs(0);
  (void)HAL_SPI_Transmit(&hspi4, &addr, 1, IMU_SPI_TIMEOUT_MS);
  (void)HAL_SPI_Receive(&hspi4, &dat, 1, IMU_SPI_TIMEOUT_MS);
  imu_cs(1);
  return dat;
}

/* 连续读多个寄存器 */
static void imu_read_registers(uint8_t reg, uint8_t *dat, uint16_t len)
{
  uint8_t addr = (uint8_t)(reg | IMU660RB_SPI_R);
  imu_cs(0);
  (void)HAL_SPI_Transmit(&hspi4, &addr, 1, IMU_SPI_TIMEOUT_MS);
  (void)HAL_SPI_Receive(&hspi4, dat, len, IMU_SPI_TIMEOUT_MS);
  imu_cs(1);
}

/* 自检：反复读 WHO_AM_I，等于 0x6B 则通过（同 zf 流程） */
static uint8_t imu_self_check(void)
{
  uint8_t  dat = 0;
  uint16_t timeout_count = 0;
  do
  {
    if (timeout_count++ > IMU660RB_TIMEOUT_COUNT)
    {
      return 1;
    }
    dat = imu_read_register(IMU660RB_CHIP_ID);
    HAL_Delay(1);
  } while (IMU660RB_CHIP_ID_VALUE != dat);
  return 0;
}

/* 导出函数定义 -------------------------------------------*/

/**
  * @brief  初始化 IMU660RB（外设 SPI4 由 CubeMX 配置，此处只配寄存器）
  * @retval 0=成功，1=失败
  */
uint8_t BSP_IMU660RB_Init(void)
{
  HAL_Delay(20);                                        /* 等待设备上电 */

  if (imu_self_check())                                 /* 自检失败 */
  {
    return 1;
  }

  imu_write_register(IMU660RB_INT1_CTRL, 0x03);         /* 开启陀螺仪 加速度数据就绪中断 */
  imu_write_register(IMU660RB_CTRL1_XL, IMU660RB_ACC_SAMPLE);   /* 加速度 ±8G 1.66kHz，第一级滤波输出 */
  imu_write_register(IMU660RB_CTRL2_G,  IMU660RB_GYR_SAMPLE);   /* 陀螺仪 ±2000dps 1.66kHz */
  imu_write_register(IMU660RB_CTRL3_C,  0x44);          /* 使能陀螺仪数字低通滤波器 */
  imu_write_register(IMU660RB_CTRL4_C,  0x02);          /* 使能数字低通滤波器 */
  imu_write_register(IMU660RB_CTRL5_C,  0x00);          /* 加速度计与陀螺仪四舍五入 */
  imu_write_register(IMU660RB_CTRL6_C,  0x00);          /* 加速度高性能模式，陀螺低通 133Hz */
  imu_write_register(IMU660RB_CTRL7_G,  0x00);          /* 陀螺仪高性能模式，关闭高通滤波 */
  imu_write_register(IMU660RB_CTRL9_XL, 0x01);          /* 关闭 I3C 接口 */
  return 0;
}

/**
  * @brief  读取三轴加速度计原始值（符号约定与 zf 一致）
  */
void BSP_IMU660RB_GetAcc(int16_t *x, int16_t *y, int16_t *z)
{
  uint8_t dat[6];
  imu_read_registers(IMU660RB_ACC_ADDRESS, dat, 6);
  *x = (int16_t)(((uint16_t)dat[1] << 8) | dat[0]);
  *y = (int16_t)(((uint16_t)dat[3] << 8) | dat[2]);
  *z = (int16_t)(((uint16_t)dat[5] << 8) | dat[4]);
}

/**
  * @brief  读取三轴陀螺仪原始值（符号约定与 zf 一致）
  */
void BSP_IMU660RB_GetGyro(int16_t *x, int16_t *y, int16_t *z)
{
  uint8_t dat[6];
  imu_read_registers(IMU660RB_GYRO_ADDRESS, dat, 6);
  *x = (int16_t)(((uint16_t)dat[1] << 8) | dat[0]);
  *y = (int16_t)(((uint16_t)dat[3] << 8) | dat[2]);
  *z = (int16_t)(((uint16_t)dat[5] << 8) | dat[4]);
}
