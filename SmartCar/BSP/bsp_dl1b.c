/**
  ******************************************************************************
  * @file    bsp_dl1b.c
  * @brief   DL1B 激光测距（VL53L1X 内核）：硬件 I2C2，7 位地址 0x29，
  *          XS 使能脚 PE8。寄存器流程移植自逐飞 zf_device_dl1b.c，
  *          135 字节默认配置数组提取自 zf_device_config.lib（源工程链接 HEX
  *          中符号 dl1b_default_configuration @ 0xFF021E，与 lib 内嵌数据一致）。
  ******************************************************************************
  */

/* 包含头文件 ------------------------------------------------------------------*/
#include "bsp_dl1b.h"
#include "i2c.h"

/* 私有宏定义 ------------------------------------------------------------------*/
#define DL1B_I2C_ADDR           (0x29u << 1)    /* HAL 地址需左移，0x52 */
#define DL1B_I2C_TIMEOUT_MS     (10u)
#define DL1B_TIMEOUT_COUNT      (1000u)         /* 初始化轮询超时（同 zf） */

#define DL1B_I2C_SLAVE__DEVICE_ADDRESS                      (0x0001u)
#define DL1B_GPIO__TIO_HV_STATUS                            (0x0031u)
#define DL1B_SYSTEM__INTERRUPT_CLEAR                        (0x0086u)
#define DL1B_RESULT__RANGE_STATUS                           (0x0089u)
#define DL1B_RESULT__FINAL_CROSSTALK_CORRECTED_RANGE_MM_SD0 (0x0096u)
#define DL1B_FIRMWARE__SYSTEM_STATUS                        (0x00E5u)

/* 私有变量 ------------------------------------------------------------------*/
static uint8_t  s_dl1b_init_flag = 0;
static uint16_t s_dl1b_distance_mm = BSP_DL1B_INVALID;

/* VL53L1X 默认配置（135 字节，从 0x0001 开始连续写入），数据同 zf 库 */
static const uint8_t s_dl1b_default_configuration[135] =
{
  0x29, 0x02, 0x10, 0x00, 0x25, 0xBC, 0x51, 0x81,
  0x80, 0x07, 0x93, 0x00, 0xFF, 0xFF, 0x75, 0xFF,
  0xFE, 0x0D, 0x00, 0x17, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x30, 0x00, 0x17, 0x0A, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
  0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10, 0x01,
  0x01, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0B, 0x00,
  0x00, 0x02, 0xFF, 0x21, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x8C, 0x00, 0x00, 0x38, 0xFF,
  0x01, 0x00, 0x27, 0x00, 0x34, 0x00, 0x65, 0x07,
  0x00, 0x87, 0x05, 0x01, 0x68, 0x00, 0xC0, 0x08,
  0x38, 0x00, 0x00, 0x00, 0x00, 0x99, 0xC8, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x07,
  0x05, 0x06, 0x06, 0x03, 0x00, 0x02, 0xC7, 0xFF,
  0xDB, 0x02, 0x00, 0x00, 0x01, 0x01, 0x21
};

/* 私有函数定义 -------------------------------------------*/

/* 读 16 位地址寄存器 */
static HAL_StatusTypeDef dl1b_read(uint16_t reg, uint8_t *buf, uint16_t len)
{
  return HAL_I2C_Mem_Read(&hi2c2, DL1B_I2C_ADDR, reg,
                          I2C_MEMADD_SIZE_16BIT, buf, len, DL1B_I2C_TIMEOUT_MS);
}

/* 写 16 位地址寄存器 */
static HAL_StatusTypeDef dl1b_write(uint16_t reg, const uint8_t *buf, uint16_t len)
{
  return HAL_I2C_Mem_Write(&hi2c2, DL1B_I2C_ADDR, reg,
                           I2C_MEMADD_SIZE_16BIT, (uint8_t *)buf, len, DL1B_I2C_TIMEOUT_MS);
}

/* 导出函数定义 -------------------------------------------*/

/**
  * @brief  初始化 DL1B（XS 复位时序 + 固件状态检查 + 写入默认配置）
  * @retval 0=成功，1=失败
  */
uint8_t BSP_DL1B_Init(void)
{
  uint8_t  dat = 0;
  uint16_t time_out_count = 0;

  /* XS 复位时序（同 zf：高 50ms -> 低 10ms -> 高 50ms） */
  HAL_GPIO_WritePin(DL1B_XS_GPIO_Port, DL1B_XS_Pin, GPIO_PIN_SET);
  HAL_Delay(50);
  HAL_GPIO_WritePin(DL1B_XS_GPIO_Port, DL1B_XS_Pin, GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(DL1B_XS_GPIO_Port, DL1B_XS_Pin, GPIO_PIN_SET);
  HAL_Delay(50);

  /* 检查固件状态：bit0=1 表示就绪 */
  if ((HAL_OK != dl1b_read(DL1B_FIRMWARE__SYSTEM_STATUS, &dat, 1)) ||
      (0x01u != (dat & 0x01u)))
  {
    return 1;
  }

  /* 写入默认配置 */
  if (HAL_OK != dl1b_write(DL1B_I2C_SLAVE__DEVICE_ADDRESS,
                           s_dl1b_default_configuration,
                           sizeof(s_dl1b_default_configuration)))
  {
    return 1;
  }

  /* 等待配置生效（TIO_HV_STATUS bit0 清 0），超时退出 */
  while (1)
  {
    if (HAL_OK != dl1b_read(DL1B_GPIO__TIO_HV_STATUS, &dat, 1))
    {
      return 1;
    }
    if (0x00u == (dat & 0x01u))
    {
      break;
    }
    if (DL1B_TIMEOUT_COUNT < time_out_count++)
    {
      return 1;
    }
    HAL_Delay(1);
  }

  s_dl1b_init_flag  = 1;
  s_dl1b_distance_mm = BSP_DL1B_INVALID;
  return 0;
}

/**
  * @brief  周期调用（5ms）轮询测距结果，非阻塞（每次最多 4 次短 I2C 事务）
  *         流程同 zf dl1b_get_distance()
  */
void BSP_DL1B_Update(void)
{
  uint8_t dat = 0;
  uint8_t range_buf[2];
  int16_t distance_temp;

  if (0u == s_dl1b_init_flag)
  {
    return;
  }

  /* 查询数据就绪状态 */
  if (HAL_OK != dl1b_read(DL1B_GPIO__TIO_HV_STATUS, &dat, 1))
  {
    s_dl1b_distance_mm = BSP_DL1B_INVALID;
    return;
  }

  if (0u != dat)
  {
    const uint8_t clear = 0x01;
    (void)dl1b_write(DL1B_SYSTEM__INTERRUPT_CLEAR, &clear, 1);  /* 清中断 */

    if ((HAL_OK == dl1b_read(DL1B_RESULT__RANGE_STATUS, &dat, 1)) &&
        (0x89u == dat))
    {
      if (HAL_OK == dl1b_read(DL1B_RESULT__FINAL_CROSSTALK_CORRECTED_RANGE_MM_SD0,
                              range_buf, 2))
      {
        distance_temp = (int16_t)(((uint16_t)range_buf[0] << 8) | range_buf[1]);
        if (distance_temp > 4000 || distance_temp < 0)
        {
          s_dl1b_distance_mm = BSP_DL1B_INVALID;    /* 超量程 */
        }
        else
        {
          s_dl1b_distance_mm = (uint16_t)distance_temp;
        }
        return;
      }
    }
  }

  s_dl1b_distance_mm = BSP_DL1B_INVALID;            /* 数据未就绪或无效 */
}

/**
  * @brief  获取最近一次测距结果（mm），无效/超量程返回 8192
  */
uint16_t BSP_DL1B_GetDistanceMm(void)
{
  return s_dl1b_distance_mm;
}
