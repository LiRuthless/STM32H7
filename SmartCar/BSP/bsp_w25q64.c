/**
  ******************************************************************************
  * @file    bsp_w25q64.c
  * @brief   板载 SPI Flash W25Q64（8MB）驱动。
  *          SPI1：SCK=PB3(AF5)、MISO=PB4(AF5)、MOSI=PD7(AF5)、CS=PD6(软件)。
  *          CubeMX 未配置 SPI1，此处纯代码初始化（同 bsp_sampler 的 TIM15 做法）。
  *          SPI123 内核时钟默认 PLL1Q=480MHz，16 分频 = 30MHz（W25Q64 上限 104MHz）。
  *          注意 PB3 复位后为 JTDO，本工程用 SWD 调试（PA13/PA14），无冲突。
  ******************************************************************************
  */

/* 包含头文件 ------------------------------------------------------------------*/
#include "bsp_w25q64.h"

/* W25Q 指令集 */
#define W25Q_CMD_WRITE_ENABLE   0x06u
#define W25Q_CMD_READ_STATUS1   0x05u
#define W25Q_CMD_READ_DATA      0x03u
#define W25Q_CMD_PAGE_PROGRAM   0x02u
#define W25Q_CMD_ERASE_SECTOR   0x20u   /* 4KB */
#define W25Q_CMD_ERASE_BLOCK64  0xD8u   /* 64KB */
#define W25Q_CMD_ERASE_CHIP     0xC7u
#define W25Q_CMD_JEDEC_ID       0x9Fu
#define W25Q_CMD_RELEASE_PD     0xABu

#define W25Q_STATUS_WIP         0x01u
#define W25Q_JEDEC_ID           0x00EF4017u

/* 引脚（板载焊死，见核心板 V12 原理图第 5 页） */
#define W25Q_CS_GPIO_PORT       GPIOD
#define W25Q_CS_PIN             GPIO_PIN_6

#define W25Q_SPI_TIMEOUT        100u    /* ms */
#define W25Q_PAGE_TIMEOUT_MS    10u
#define W25Q_SECTOR_TIMEOUT_MS  500u
#define W25Q_BLOCK_TIMEOUT_MS   2500u
#define W25Q_CHIP_TIMEOUT_MS    110000u

/* 私有变量 ---------------------------------------------------------*/
static SPI_HandleTypeDef s_hspi1;

/* 私有函数原型 ---------------------------------------------*/
static void w25q_cs_low(void);
static void w25q_cs_high(void);
static uint8_t w25q_read_status1(uint8_t *status);
static uint8_t w25q_wait_idle(uint32_t timeout_ms);
static uint8_t w25q_write_enable(void);

/* 私有函数定义 -------------------------------------------*/

static void w25q_cs_low(void)
{
  HAL_GPIO_WritePin(W25Q_CS_GPIO_PORT, W25Q_CS_PIN, GPIO_PIN_RESET);
}

static void w25q_cs_high(void)
{
  HAL_GPIO_WritePin(W25Q_CS_GPIO_PORT, W25Q_CS_PIN, GPIO_PIN_SET);
}

static uint8_t w25q_read_status1(uint8_t *status)
{
  uint8_t cmd = W25Q_CMD_READ_STATUS1;
  uint8_t val = 0;
  HAL_StatusTypeDef result;
  w25q_cs_low();
  result = HAL_SPI_Transmit(&s_hspi1, &cmd, 1, W25Q_SPI_TIMEOUT);
  if (result == HAL_OK)
  {
    result = HAL_SPI_Receive(&s_hspi1, &val, 1, W25Q_SPI_TIMEOUT);
  }
  w25q_cs_high();
  if (result != HAL_OK)
  {
    return 1u;
  }
  *status = val;
  return 0u;
}

static uint8_t w25q_wait_idle(uint32_t timeout_ms)
{
  uint32_t start = HAL_GetTick();
  uint8_t status;

  for (;;)
  {
    if (w25q_read_status1(&status) != 0u)
    {
      return 1u;
    }
    if ((status & W25Q_STATUS_WIP) == 0u)
    {
      return 0u;
    }
    if ((uint32_t)(HAL_GetTick() - start) >= timeout_ms)
    {
      return 1u;
    }
  }
}

static uint8_t w25q_write_enable(void)
{
  uint8_t cmd = W25Q_CMD_WRITE_ENABLE;
  HAL_StatusTypeDef result;
  w25q_cs_low();
  result = HAL_SPI_Transmit(&s_hspi1, &cmd, 1, W25Q_SPI_TIMEOUT);
  w25q_cs_high();
  return (result == HAL_OK) ? 0u : 1u;
}

/* 导出函数定义 -------------------------------------------*/

/**
  * @brief  初始化 SPI1 与 W25Q64，校验 JEDEC ID
  * @retval 0=成功，1=失败（读不到芯片）
  */
uint8_t BSP_W25Q64_Init(void)
{
  GPIO_InitTypeDef gpio = {0};
  uint8_t cmd = W25Q_CMD_JEDEC_ID;
  uint8_t id[3] = {0};
  uint32_t jedec;
  HAL_StatusTypeDef result;

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_SPI1_CLK_ENABLE();

  /* CS 先拉高（板上另有 R40 100K 上拉） */
  gpio.Pin   = W25Q_CS_PIN;
  gpio.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio.Pull  = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(W25Q_CS_GPIO_PORT, &gpio);
  w25q_cs_high();

  /* SPI1 复用脚：PB3=SCK、PB4=MISO、PD7=MOSI（AF5） */
  gpio.Mode      = GPIO_MODE_AF_PP;
  gpio.Pull      = GPIO_NOPULL;
  gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF5_SPI1;
  gpio.Pin = GPIO_PIN_3 | GPIO_PIN_4;
  HAL_GPIO_Init(GPIOB, &gpio);
  gpio.Pin = GPIO_PIN_7;
  HAL_GPIO_Init(GPIOD, &gpio);

  s_hspi1.Instance               = SPI1;
  s_hspi1.Init.Mode              = SPI_MODE_MASTER;
  s_hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
  s_hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
  s_hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
  s_hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
  s_hspi1.Init.NSS               = SPI_NSS_SOFT;
  s_hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;  /* 480M/16 = 30MHz */
  s_hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  s_hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
  s_hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  s_hspi1.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&s_hspi1) != HAL_OK)
  {
    return 1;
  }

  /* 退出掉电模式 + 读 JEDEC ID 自检 */
  cmd = W25Q_CMD_RELEASE_PD;
  w25q_cs_low();
  result = HAL_SPI_Transmit(&s_hspi1, &cmd, 1, W25Q_SPI_TIMEOUT);
  w25q_cs_high();
  if (result != HAL_OK)
  {
    return 1u;
  }
  HAL_Delay(1);

  cmd = W25Q_CMD_JEDEC_ID;
  w25q_cs_low();
  result = HAL_SPI_Transmit(&s_hspi1, &cmd, 1, W25Q_SPI_TIMEOUT);
  if (result == HAL_OK)
  {
    result = HAL_SPI_Receive(&s_hspi1, id, 3, W25Q_SPI_TIMEOUT);
  }
  w25q_cs_high();
  if (result != HAL_OK)
  {
    return 1u;
  }

  jedec = ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
  return (jedec == W25Q_JEDEC_ID) ? 0u : 1u;
}

/**
  * @brief  读数据（标准读 0x03，24 位地址）
  * @retval 0=成功
  */
uint8_t BSP_W25Q64_Read(uint32_t addr, uint8_t *buf, uint32_t len)
{
  uint8_t header[4];
  HAL_StatusTypeDef result;

  if ((buf == 0) || (len == 0u) || (addr >= W25Q64_TOTAL_SIZE) ||
      (len > (W25Q64_TOTAL_SIZE - addr)))
  {
    return 1;
  }

  header[0] = W25Q_CMD_READ_DATA;
  header[1] = (uint8_t)(addr >> 16);
  header[2] = (uint8_t)(addr >> 8);
  header[3] = (uint8_t)(addr);

  w25q_cs_low();
  result = HAL_SPI_Transmit(&s_hspi1, header, 4, W25Q_SPI_TIMEOUT);
  if (result == HAL_OK)
  {
    result = HAL_SPI_Receive(&s_hspi1, buf, (uint16_t)len, W25Q_SPI_TIMEOUT);
  }
  w25q_cs_high();
  return (result == HAL_OK) ? 0u : 1u;
}

/**
  * @brief  页编程（0x02）。len ≤ 256 且不得跨页（调用方保证）
  * @retval 0=成功
  */
uint8_t BSP_W25Q64_WritePage(uint32_t addr, const uint8_t *buf, uint16_t len)
{
  uint8_t header[4];
  HAL_StatusTypeDef result;

  if ((buf == 0) || (len == 0u) || (len > W25Q64_PAGE_SIZE) ||
      (addr >= W25Q64_TOTAL_SIZE) ||
      ((addr / W25Q64_PAGE_SIZE) != ((addr + len - 1u) / W25Q64_PAGE_SIZE)))
  {
    return 1;
  }

  header[0] = W25Q_CMD_PAGE_PROGRAM;
  header[1] = (uint8_t)(addr >> 16);
  header[2] = (uint8_t)(addr >> 8);
  header[3] = (uint8_t)(addr);

  if (w25q_write_enable() != 0u)
  {
    return 1u;
  }
  w25q_cs_low();
  result = HAL_SPI_Transmit(&s_hspi1, header, 4, W25Q_SPI_TIMEOUT);
  if (result == HAL_OK)
  {
    result = HAL_SPI_Transmit(&s_hspi1, (uint8_t *)buf, len, W25Q_SPI_TIMEOUT);
  }
  w25q_cs_high();
  if (result != HAL_OK)
  {
    return 1u;
  }
  return w25q_wait_idle(W25Q_PAGE_TIMEOUT_MS);
}

/**
  * @brief  4KB 扇区擦除（含等待，典型几十 ms）
  * @retval 0=成功
  */
uint8_t BSP_W25Q64_EraseSector(uint32_t addr)
{
  uint8_t header[4];
  HAL_StatusTypeDef result;

  if (addr >= W25Q64_TOTAL_SIZE)
  {
    return 1;
  }

  header[0] = W25Q_CMD_ERASE_SECTOR;
  header[1] = (uint8_t)(addr >> 16);
  header[2] = (uint8_t)(addr >> 8);
  header[3] = (uint8_t)(addr);

  if (w25q_write_enable() != 0u)
  {
    return 1u;
  }
  w25q_cs_low();
  result = HAL_SPI_Transmit(&s_hspi1, header, 4, W25Q_SPI_TIMEOUT);
  w25q_cs_high();
  if (result != HAL_OK)
  {
    return 1u;
  }
  return w25q_wait_idle(W25Q_SECTOR_TIMEOUT_MS);
}

/**
  * @brief  64KB 块擦除（含等待）
  * @retval 0=成功
  */
uint8_t BSP_W25Q64_EraseBlock64K(uint32_t addr)
{
  uint8_t header[4];
  HAL_StatusTypeDef result;

  if (addr >= W25Q64_TOTAL_SIZE)
  {
    return 1;
  }

  header[0] = W25Q_CMD_ERASE_BLOCK64;
  header[1] = (uint8_t)(addr >> 16);
  header[2] = (uint8_t)(addr >> 8);
  header[3] = (uint8_t)(addr);

  if (w25q_write_enable() != 0u)
  {
    return 1u;
  }
  w25q_cs_low();
  result = HAL_SPI_Transmit(&s_hspi1, header, 4, W25Q_SPI_TIMEOUT);
  w25q_cs_high();
  if (result != HAL_OK)
  {
    return 1u;
  }
  return w25q_wait_idle(W25Q_BLOCK_TIMEOUT_MS);
}

/**
  * @brief  整片擦除（8MB 典型 20s、最长 100s；仅看门狗启动前维护使用）
  * @retval 0=成功
  */
uint8_t BSP_W25Q64_EraseChip(void)
{
  uint8_t cmd = W25Q_CMD_ERASE_CHIP;
  HAL_StatusTypeDef result;

  if (w25q_write_enable() != 0u)
  {
    return 1u;
  }
  w25q_cs_low();
  result = HAL_SPI_Transmit(&s_hspi1, &cmd, 1, W25Q_SPI_TIMEOUT);
  w25q_cs_high();
  if (result != HAL_OK)
  {
    return 1u;
  }
  return w25q_wait_idle(W25Q_CHIP_TIMEOUT_MS);
}
