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
#define BSP_W25Q64_CMD_WRITE_ENABLE 0x06u
#define BSP_W25Q64_CMD_READ_STATUS1 0x05u
#define BSP_W25Q64_CMD_READ_DATA 0x03u
#define BSP_W25Q64_CMD_PAGE_PROGRAM 0x02u
#define BSP_W25Q64_CMD_ERASE_SECTOR 0x20u  /* 4KB */
#define BSP_W25Q64_CMD_ERASE_BLOCK64 0xD8u /* 64KB */
#define BSP_W25Q64_CMD_ERASE_CHIP 0xC7u
#define BSP_W25Q64_CMD_JEDEC_ID 0x9Fu
#define BSP_W25Q64_CMD_RELEASE_PD 0xABu

#define BSP_W25Q64_STATUS_WIP 0x01u
#define BSP_W25Q64_JEDEC_ID 0x00EF4017u

/* 引脚（板载焊死，见核心板 V12 原理图第 5 页） */
#define BSP_W25Q64_CS_GPIO_PORT GPIOD
#define BSP_W25Q64_CS_PIN GPIO_PIN_6

#define BSP_W25Q64_SPI_TIMEOUT 100u /* ms */
#define BSP_W25Q64_PAGE_TIMEOUT_MS 10u
#define BSP_W25Q64_SECTOR_TIMEOUT_MS 500u
#define BSP_W25Q64_BLOCK_TIMEOUT_MS 2500u
#define BSP_W25Q64_CHIP_TIMEOUT_MS 110000u

/* 私有变量 ---------------------------------------------------------*/
static SPI_HandleTypeDef s_hspi1;

/* 私有函数原型 ---------------------------------------------*/
static void BSP_W25Q64_Select(void);
static void BSP_W25Q64_Deselect(void);
static uint8_t BSP_W25Q64_ReadStatus1(uint8_t *status);
static uint8_t BSP_W25Q64_WaitIdle(uint32_t timeout_ms);
static uint8_t BSP_W25Q64_EnableWrite(void);

/* 私有函数定义 -------------------------------------------*/

/**
 * @brief 拉低 W25Q64 片选。
 * @note 前台串行调用；等待使用 HAL tick，不在 ISR 中执行。
 */
static void BSP_W25Q64_Select(void)
{
    HAL_GPIO_WritePin(BSP_W25Q64_CS_GPIO_PORT, BSP_W25Q64_CS_PIN, GPIO_PIN_RESET);
}

/**
 * @brief 拉高 W25Q64 片选，结束当前传输。
 * @note 前台串行调用；等待使用 HAL tick，不在 ISR 中执行。
 */
static void BSP_W25Q64_Deselect(void)
{
    HAL_GPIO_WritePin(BSP_W25Q64_CS_GPIO_PORT, BSP_W25Q64_CS_PIN, GPIO_PIN_SET);
}

/**
 * @brief 读取状态寄存器一，成功后才写入输出。
 * @param[in,out] status 输出状态字节，必须非空；传输成功后才写入。
 * @return 0 成功；1 SPI 传输失败。
 * @note 前台串行调用；等待使用 HAL tick，不在 ISR 中执行。
 */
static uint8_t BSP_W25Q64_ReadStatus1(uint8_t *status)
{
    uint8_t cmd = BSP_W25Q64_CMD_READ_STATUS1;
    uint8_t val = 0;
    HAL_StatusTypeDef result;
    BSP_W25Q64_Select();
    result = HAL_SPI_Transmit(&s_hspi1, &cmd, 1, BSP_W25Q64_SPI_TIMEOUT);
    if (result == HAL_OK)
    {
        result = HAL_SPI_Receive(&s_hspi1, &val, 1, BSP_W25Q64_SPI_TIMEOUT);
    }
    BSP_W25Q64_Deselect();
    if (result != HAL_OK)
    {
        return 1u;
    }
    *status = val;
    return 0u;
}

/**
 * @brief 有界轮询 WIP，使用无符号毫秒差处理计数回绕。
 * @param[in] timeout_ms WIP 等待预算，单位 ms，使用对应操作的原有常量。
 * @return 0 WIP 清零；1 状态读取失败或等待超时。
 * @note 前台串行调用；等待使用 HAL tick，不在 ISR 中执行。
 */
static uint8_t BSP_W25Q64_WaitIdle(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t status;

    for (;;)
    {
        if (BSP_W25Q64_ReadStatus1(&status) != 0u)
        {
            return 1u;
        }
        if ((status & BSP_W25Q64_STATUS_WIP) == 0u)
        {
            return 0u;
        }
        if ((uint32_t)(HAL_GetTick() - start) >= timeout_ms)
        {
            return 1u;
        }
    }
}

/**
 * @brief 发送写使能指令并释放片选。
 * @return 0 成功；1 SPI 失败。
 * @note 前台串行调用；等待使用 HAL tick，不在 ISR 中执行。
 */
static uint8_t BSP_W25Q64_EnableWrite(void)
{
    uint8_t cmd = BSP_W25Q64_CMD_WRITE_ENABLE;
    HAL_StatusTypeDef result;
    BSP_W25Q64_Select();
    result = HAL_SPI_Transmit(&s_hspi1, &cmd, 1, BSP_W25Q64_SPI_TIMEOUT);
    BSP_W25Q64_Deselect();
    return (result == HAL_OK) ? 0u : 1u;
}

/* 导出函数定义 -------------------------------------------*/

uint8_t BSP_W25Q64_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    uint8_t cmd = BSP_W25Q64_CMD_JEDEC_ID;
    uint8_t id[3] = {0};
    uint32_t jedec;
    HAL_StatusTypeDef result;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    /* CS 先拉高（板上另有 R40 100K 上拉） */
    gpio.Pin = BSP_W25Q64_CS_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(BSP_W25Q64_CS_GPIO_PORT, &gpio);
    BSP_W25Q64_Deselect();

    /* SPI1 复用脚：PB3=SCK、PB4=MISO、PD7=MOSI（AF5） */
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF5_SPI1;
    gpio.Pin = GPIO_PIN_3 | GPIO_PIN_4;
    HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_7;
    HAL_GPIO_Init(GPIOD, &gpio);

    s_hspi1.Instance = SPI1;
    s_hspi1.Init.Mode = SPI_MODE_MASTER;
    s_hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    s_hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    s_hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    s_hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    s_hspi1.Init.NSS = SPI_NSS_SOFT;
    s_hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; /* 480M/16 = 30MHz */
    s_hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    s_hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    s_hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    s_hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    if (HAL_SPI_Init(&s_hspi1) != HAL_OK)
    {
        return 1;
    }

    /* 退出掉电模式 + 读 JEDEC ID 自检 */
    cmd = BSP_W25Q64_CMD_RELEASE_PD;
    BSP_W25Q64_Select();
    result = HAL_SPI_Transmit(&s_hspi1, &cmd, 1, BSP_W25Q64_SPI_TIMEOUT);
    BSP_W25Q64_Deselect();
    if (result != HAL_OK)
    {
        return 1u;
    }
    HAL_Delay(1);

    cmd = BSP_W25Q64_CMD_JEDEC_ID;
    BSP_W25Q64_Select();
    result = HAL_SPI_Transmit(&s_hspi1, &cmd, 1, BSP_W25Q64_SPI_TIMEOUT);
    if (result == HAL_OK)
    {
        result = HAL_SPI_Receive(&s_hspi1, id, 3, BSP_W25Q64_SPI_TIMEOUT);
    }
    BSP_W25Q64_Deselect();
    if (result != HAL_OK)
    {
        return 1u;
    }

    jedec = ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
    return (jedec == BSP_W25Q64_JEDEC_ID) ? 0u : 1u;
}

uint8_t BSP_W25Q64_Read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t header[4];
    HAL_StatusTypeDef result;

    if ((buf == 0) || (len == 0u) || (addr >= BSP_W25Q64_TOTAL_SIZE)
        || (len > (BSP_W25Q64_TOTAL_SIZE - addr)))
    {
        return 1;
    }

    header[0] = BSP_W25Q64_CMD_READ_DATA;
    header[1] = (uint8_t)(addr >> 16);
    header[2] = (uint8_t)(addr >> 8);
    header[3] = (uint8_t)(addr);

    BSP_W25Q64_Select();
    result = HAL_SPI_Transmit(&s_hspi1, header, 4, BSP_W25Q64_SPI_TIMEOUT);
    if (result == HAL_OK)
    {
        result = HAL_SPI_Receive(&s_hspi1, buf, (uint16_t)len, BSP_W25Q64_SPI_TIMEOUT);
    }
    BSP_W25Q64_Deselect();
    return (result == HAL_OK) ? 0u : 1u;
}

uint8_t BSP_W25Q64_WritePage(uint32_t addr, const uint8_t *buf, uint16_t len)
{
    uint8_t header[4];
    HAL_StatusTypeDef result;

    if ((buf == 0) || (len == 0u) || (len > BSP_W25Q64_PAGE_SIZE) || (addr >= BSP_W25Q64_TOTAL_SIZE)
        || ((addr / BSP_W25Q64_PAGE_SIZE) != ((addr + len - 1u) / BSP_W25Q64_PAGE_SIZE)))
    {
        return 1;
    }

    header[0] = BSP_W25Q64_CMD_PAGE_PROGRAM;
    header[1] = (uint8_t)(addr >> 16);
    header[2] = (uint8_t)(addr >> 8);
    header[3] = (uint8_t)(addr);

    if (BSP_W25Q64_EnableWrite() != 0u)
    {
        return 1u;
    }
    BSP_W25Q64_Select();
    result = HAL_SPI_Transmit(&s_hspi1, header, 4, BSP_W25Q64_SPI_TIMEOUT);
    if (result == HAL_OK)
    {
        result = HAL_SPI_Transmit(&s_hspi1, (uint8_t *)buf, len, BSP_W25Q64_SPI_TIMEOUT);
    }
    BSP_W25Q64_Deselect();
    if (result != HAL_OK)
    {
        return 1u;
    }
    return BSP_W25Q64_WaitIdle(BSP_W25Q64_PAGE_TIMEOUT_MS);
}

uint8_t BSP_W25Q64_EraseSector(uint32_t addr)
{
    uint8_t header[4];
    HAL_StatusTypeDef result;

    if (addr >= BSP_W25Q64_TOTAL_SIZE)
    {
        return 1;
    }

    header[0] = BSP_W25Q64_CMD_ERASE_SECTOR;
    header[1] = (uint8_t)(addr >> 16);
    header[2] = (uint8_t)(addr >> 8);
    header[3] = (uint8_t)(addr);

    if (BSP_W25Q64_EnableWrite() != 0u)
    {
        return 1u;
    }
    BSP_W25Q64_Select();
    result = HAL_SPI_Transmit(&s_hspi1, header, 4, BSP_W25Q64_SPI_TIMEOUT);
    BSP_W25Q64_Deselect();
    if (result != HAL_OK)
    {
        return 1u;
    }
    return BSP_W25Q64_WaitIdle(BSP_W25Q64_SECTOR_TIMEOUT_MS);
}

uint8_t BSP_W25Q64_EraseBlock64K(uint32_t addr)
{
    uint8_t header[4];
    HAL_StatusTypeDef result;

    if (addr >= BSP_W25Q64_TOTAL_SIZE)
    {
        return 1;
    }

    header[0] = BSP_W25Q64_CMD_ERASE_BLOCK64;
    header[1] = (uint8_t)(addr >> 16);
    header[2] = (uint8_t)(addr >> 8);
    header[3] = (uint8_t)(addr);

    if (BSP_W25Q64_EnableWrite() != 0u)
    {
        return 1u;
    }
    BSP_W25Q64_Select();
    result = HAL_SPI_Transmit(&s_hspi1, header, 4, BSP_W25Q64_SPI_TIMEOUT);
    BSP_W25Q64_Deselect();
    if (result != HAL_OK)
    {
        return 1u;
    }
    return BSP_W25Q64_WaitIdle(BSP_W25Q64_BLOCK_TIMEOUT_MS);
}

uint8_t BSP_W25Q64_EraseChip(void)
{
    uint8_t cmd = BSP_W25Q64_CMD_ERASE_CHIP;
    HAL_StatusTypeDef result;

    if (BSP_W25Q64_EnableWrite() != 0u)
    {
        return 1u;
    }
    BSP_W25Q64_Select();
    result = HAL_SPI_Transmit(&s_hspi1, &cmd, 1, BSP_W25Q64_SPI_TIMEOUT);
    BSP_W25Q64_Deselect();
    if (result != HAL_OK)
    {
        return 1u;
    }
    return BSP_W25Q64_WaitIdle(BSP_W25Q64_CHIP_TIMEOUT_MS);
}
