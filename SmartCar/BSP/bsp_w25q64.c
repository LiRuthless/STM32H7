/**
 * @file bsp_w25q64.c
 * @brief 板载 SPI Flash W25Q64（8MB）驱动。
 * @details 通过 SPI1 实现 W25Q64 识别、读写、等待空闲和扇区、块、整片擦除原语。
 */
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
static SPI_HandleTypeDef s_hspi1;
static void BSP_W25Q64_Select(void);
static void BSP_W25Q64_Deselect(void);
static uint8_t BSP_W25Q64_ReadStatus1(uint8_t *status);
static uint8_t BSP_W25Q64_WaitIdle(uint32_t timeout_ms);
static uint8_t BSP_W25Q64_EnableWrite(void);
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
 * @details 按现有调用顺序使用 BSP_W25Q64_Select、HAL_SPI_Transmit、HAL_SPI_Receive、BSP_W25Q64_Deselect
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in,out] status 输出状态字节，必须非空；传输成功后才写入。
 * @return 0 成功；1 SPI 传输失败。
 * @note 前台串行调用；等待使用 HAL tick，不在 ISR 中执行。
 */
static uint8_t BSP_W25Q64_ReadStatus1(uint8_t *status)
{
    /* 选中器件后发送状态寄存器命令并接收一字节；任一步失败仍释放片选，只有两步均成功才写调用方输出。 */
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
 * @details 按现有调用顺序使用 HAL_GetTick、BSP_W25Q64_ReadStatus1 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] timeout_ms WIP 等待预算，单位 ms，使用对应操作的原有常量。
 * @return 0 WIP 清零；1 状态读取失败或等待超时。
 * @note 前台串行调用；等待使用 HAL tick，不在 ISR 中执行。
 */
static uint8_t BSP_W25Q64_WaitIdle(uint32_t timeout_ms)
{
    /* 以 HAL tick 为起点轮询状态寄存器；读失败或超过给定毫秒上限返回错误，WIP 清零返回成功。 */
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
 * @details 按现有调用顺序使用 BSP_W25Q64_Select、HAL_SPI_Transmit、BSP_W25Q64_Deselect 完成本函数处理。
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
/**
 * @brief 初始化 SPI1 并释放掉电模式、检查 JEDEC ID。
 * @details 按现有调用顺序使用
 *     __HAL_RCC_GPIOB_CLK_ENABLE、__HAL_RCC_GPIOD_CLK_ENABLE、__HAL_RCC_SPI1_CLK_ENABLE、HAL_GPIO_Init
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @return 0 标识为 0xEF4017；1 初始化、传输失败或标识不符。
 * @note 启动主循环阻塞调用；含 HAL_Delay，保持原有 GPIO/SPI 配置。
 */
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

/**
 * @brief 读取W25Q64 数据。
 * @details 验证地址、缓冲和长度后发送 24 位读地址，再阻塞接收数据；无论传输成功与否都在返回前释放片选。
 * @param[in] addr W25Q64 字节地址，整个读取范围必须小于 8 MiB。
 * @param[out] buf 接收缓冲，非空且至少可写 len 字节。
 * @param[in] len 非零读取字节数；传给 HAL 时保持现有 uint16_t 转换。
 * @return 0 成功；1 参数不合法、后端未就绪或 SPI/等待失败。
 * @note 长度仍按原逻辑转为 uint16_t 传给 HAL，不新增长度限制。
 */
uint8_t BSP_W25Q64_Read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    /* 先校验完整地址范围，再发送 24 位读命令并接收数据；所有传输路径在返回前释放片选。 */
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

/**
 * @brief 写入W25Q64 数据。
 * @details 检查空指针、长度和页边界后等待芯片空闲、置写使能并发送页编程命令；传输或等待失败立即撤销片选并返回错误。
 * @param[in] addr 页内起始字节地址，必须小于 8 MiB。
 * @param[in] buf 待写缓冲，非空且至少可读 len 字节。
 * @param[in] len 1..256 字节，且 addr 到末字节不能跨页。
 * @return 0 成功；1 参数不合法、后端未就绪或 SPI/等待失败。
 * @pre 长度最多 256 字节且不能跨页；写后 WIP 等待上限 10 ms。
 * @note 主循环 Flash 服务调用，SPI1 阻塞；失败可能发生在命令或数据已部分发送之后，不承诺回滚。
 */
uint8_t BSP_W25Q64_WritePage(uint32_t addr, const uint8_t *buf, uint16_t len)
{
    /* 校验单页范围后先置写使能，再发送页编程头和数据；释放片选后检查传输结果，最后等待 WIP 清零。 */
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

/**
 * @brief 擦除4 KiB 扇区并等待 WIP 清零。
 * @details 检查地址后执行写使能，发送 4 KiB 扇区擦除命令并释放片选；命令成功后以 500 ms 上限等待 WIP 清零。
 * @param[in] addr 目标扇区内字节地址，必须小于 8 MiB。
 * @return 0 完成；1 地址不合法、SPI 失败或 WIP 轮询超时。
 * @note 仅前台阻塞调用，WIP 等待上限 500 ms。
 */
uint8_t BSP_W25Q64_EraseSector(uint32_t addr)
{
    /* 校验地址并置写使能，发送扇区擦除命令后释放片选；命令成功才进入 500 ms WIP 等待。 */
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

/**
 * @brief 擦除64 KiB 块并等待 WIP 清零。
 * @details 检查地址后执行写使能，发送 64 KiB 块擦除命令并释放片选；命令成功后以 2500 ms 上限等待 WIP 清零。
 * @param[in] addr 目标 64 KiB 块内字节地址，必须小于 8 MiB。
 * @return 0 完成；1 地址不合法、SPI 失败或 WIP 轮询超时。
 * @note 仅前台阻塞调用，WIP 等待上限 2500 ms。
 */
uint8_t BSP_W25Q64_EraseBlock64K(uint32_t addr)
{
    /* 校验地址并置写使能，发送块擦除命令后释放片选；命令成功才进入 2500 ms WIP 等待。 */
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

/**
 * @brief 擦除整片并等待 WIP 清零。
 * @details 按现有调用顺序使用 BSP_W25Q64_EnableWrite、BSP_W25Q64_Select、HAL_SPI_Transmit、BSP_W25Q64_Deselect
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @return 0 完成；1 地址不合法、SPI 失败或 WIP 轮询超时。
 * @note 仅前台阻塞调用，WIP 等待上限 110 s。仅看门狗启动前维护使用，当前运行链不调用。
 */
uint8_t BSP_W25Q64_EraseChip(void)
{
    /* 先置写使能并发送整片擦除命令，释放片选后检查 HAL 结果；命令成功才执行最长 110 秒的 WIP 等待。 */
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
