/**
 * @file bsp_dl1b.c
 * @brief DL1B 激光测距（VL53L1X 内核）：硬件 I2C2，7 位地址 0x29， XS 使能脚 PE8。
 * @details 前台初始化通过 XS 复位及 I2C2 配置测距器；TIM6 控制链阻塞读取状态和距离。
 *     模块缓存毫米值或 BSP_DL1B_INVALID，读取缓存不发起总线访问。
 */
#include "bsp_dl1b.h"
#include "i2c.h"
#define BSP_DL1B_I2C_ADDR (0x29u << 1) /* HAL 地址需左移，0x52 */
#define BSP_DL1B_I2C_TIMEOUT_MS (10u)
#define BSP_DL1B_TIMEOUT_COUNT (1000u) /* 初始化轮询超时（同 zf） */

#define BSP_DL1B_I2C_SLAVE_DEVICE_ADDRESS (0x0001u)
#define BSP_DL1B_GPIO_TIO_HV_STATUS (0x0031u)
#define BSP_DL1B_SYSTEM_INTERRUPT_CLEAR (0x0086u)
#define BSP_DL1B_RESULT_RANGE_STATUS (0x0089u)
#define BSP_DL1B_RESULT_FINAL_CROSSTALK_CORRECTED_RANGE_MM_SD0 (0x0096u)
#define BSP_DL1B_FIRMWARE_SYSTEM_STATUS (0x00E5u)
static uint8_t s_dl1b_init_flag = 0;
static uint16_t s_dl1b_distance_mm = BSP_DL1B_INVALID;

/* VL53L1X 默认配置（135 字节，从 0x0001 开始连续写入），数据同 zf 库 */
static const uint8_t s_dl1b_default_configuration[135] =
{
    0x29, 0x02, 0x10, 0x00, 0x25, 0xBC, 0x51,
    0x81, 0x80, 0x07, 0x93, 0x00, 0xFF, 0xFF, 0x75, 0xFF, 0xFE, 0x0D, 0x00, 0x17, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x17, 0x0A, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10,
    0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0B,
    0x00, 0x00, 0x02, 0xFF, 0x21, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x8C, 0x00, 0x00, 0x38,
    0xFF, 0x01, 0x00, 0x27, 0x00, 0x34, 0x00, 0x65, 0x07, 0x00, 0x87, 0x05, 0x01, 0x68, 0x00, 0xC0,
    0x08, 0x38, 0x00, 0x00, 0x00, 0x00, 0x99, 0xC8, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    0x07, 0x05, 0x06, 0x06, 0x03, 0x00, 0x02, 0xC7, 0xFF, 0xDB, 0x02, 0x00, 0x00, 0x01, 0x01, 0x21
};
/* 读 16 位地址寄存器 */
/**
 * @brief 读取 DL1B 寄存器到缓冲。
 * @param[in] reg 16 位寄存器地址。
 * @param[out] buf 数据缓冲，必须非空且至少具有 len 字节容量。
 * @param[in] len 传输字节数，由模块内调用方提供。
 * @return 原 HAL I2C 状态；失败可能已部分写入输出。
 * @note 使用阻塞 HAL I2C 接口，传入 10 ms 超时；实际退出依赖 HAL 时基推进。
 */
static HAL_StatusTypeDef BSP_DL1B_ReadRegister(uint16_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(
        &hi2c2, BSP_DL1B_I2C_ADDR, reg, I2C_MEMADD_SIZE_16BIT, buf, len, BSP_DL1B_I2C_TIMEOUT_MS);
}

/* 写 16 位地址寄存器 */
/**
 * @brief 写入 DL1B 寄存器。
 * @param[in] reg 16 位寄存器地址。
 * @param[in] buf 数据缓冲，必须非空且至少具有 len 字节容量。
 * @param[in] len 传输字节数，由模块内调用方提供。
 * @return 原 HAL I2C 状态。
 * @note 使用阻塞 HAL I2C 接口，传入 10 ms 超时；实际退出依赖 HAL 时基推进。
 */
static HAL_StatusTypeDef BSP_DL1B_WriteRegister(uint16_t reg, const uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Write(&hi2c2, BSP_DL1B_I2C_ADDR, reg, I2C_MEMADD_SIZE_16BIT, (uint8_t *)buf,
        len, BSP_DL1B_I2C_TIMEOUT_MS);
}
/**
 * @brief 复位 DL1B、校验固件状态并写入原配置。
 * @details XS 高 50 ms、低 10 ms、高 50 ms 后检查固件状态 bit0，写入 135 字节配置，
 *     轮询状态 bit0 清零；I2C 失败或超过轮询计数返回失败。成功才置初始化标志并清缓存。
 * @return 0 初始化成功；1 I2C 失败、状态不符或轮询超时。
 * @note 启动主循环阻塞调用；失败不主动清除原有初始化标志。
 */
uint8_t BSP_DL1B_Init(void)
{
    uint8_t dat = 0;
    uint16_t time_out_count = 0;

    /* XS 复位时序（同 zf：高 50ms -> 低 10ms -> 高 50ms） */
    HAL_GPIO_WritePin(DL1B_XS_GPIO_Port, DL1B_XS_Pin, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(DL1B_XS_GPIO_Port, DL1B_XS_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(DL1B_XS_GPIO_Port, DL1B_XS_Pin, GPIO_PIN_SET);
    HAL_Delay(50);

    /* 检查固件状态：bit0=1 表示就绪 */
    if ((HAL_OK != BSP_DL1B_ReadRegister(BSP_DL1B_FIRMWARE_SYSTEM_STATUS, &dat, 1))
        || (0x01u != (dat & 0x01u)))
    {
        return 1;
    }

    /* 写入默认配置 */
    if (HAL_OK
        != BSP_DL1B_WriteRegister(BSP_DL1B_I2C_SLAVE_DEVICE_ADDRESS, s_dl1b_default_configuration,
            sizeof(s_dl1b_default_configuration)))
    {
        return 1;
    }

    /* 等待配置生效（TIO_HV_STATUS bit0 清 0），超时退出 */
    while (1)
    {
        if (HAL_OK != BSP_DL1B_ReadRegister(BSP_DL1B_GPIO_TIO_HV_STATUS, &dat, 1))
        {
            return 1;
        }
        if (0x00u == (dat & 0x01u))
        {
            break;
        }
        if (BSP_DL1B_TIMEOUT_COUNT < time_out_count++)
        {
            return 1;
        }
        HAL_Delay(1);
    }

    s_dl1b_init_flag = 1;
    s_dl1b_distance_mm = BSP_DL1B_INVALID;
    return 0;
}

/**
 * @brief 读取 DL1B 就绪状态并更新距离缓存。
 * @details 未初始化时保留缓存；状态字节非零时尝试清中断（忽略写入结果），再要求量程状态
 *     等于 0x89。两字节距离组合后转 int16_t，仅 0～4000 保存为毫米缓存，其余发布无效值。
 * @note App_ControlISR 的 TIM6 控制链调用，存在 I2C 阻塞；HAL 时基不推进时不能保证超时退出。
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
    if (HAL_OK != BSP_DL1B_ReadRegister(BSP_DL1B_GPIO_TIO_HV_STATUS, &dat, 1))
    {
        s_dl1b_distance_mm = BSP_DL1B_INVALID;
        return;
    }

    if (0u != dat)
    {
        const uint8_t clear = 0x01;
        (void)BSP_DL1B_WriteRegister(BSP_DL1B_SYSTEM_INTERRUPT_CLEAR, &clear, 1); /* 清中断 */

        if ((HAL_OK == BSP_DL1B_ReadRegister(BSP_DL1B_RESULT_RANGE_STATUS, &dat, 1))
            && (0x89u == dat))
        {
            if (HAL_OK
                == BSP_DL1B_ReadRegister(
                    BSP_DL1B_RESULT_FINAL_CROSSTALK_CORRECTED_RANGE_MM_SD0, range_buf, 2))
            {
                distance_temp = (int16_t)(((uint16_t)range_buf[0] << 8) | range_buf[1]);
                if (distance_temp > 4000 || distance_temp < 0)
                {
                    s_dl1b_distance_mm = BSP_DL1B_INVALID; /* 超量程 */
                }
                else
                {
                    s_dl1b_distance_mm = (uint16_t)distance_temp;
                }
                return;
            }
        }
    }

    s_dl1b_distance_mm = BSP_DL1B_INVALID; /* 数据未就绪或无效 */
}

/**
 * @brief 读取最近一次测距缓存。
 * @details 直接返回已按毫米保存的缓存；不进行单位换算，也不触发新的总线测量。
 * @return 0～4000 mm；无有效测量时返回 BSP_DL1B_INVALID。
 * @note 读取模块缓存，不阻塞；缓存由 DL1B 更新路径写入。
 */
uint16_t BSP_DL1B_GetDistanceMm(void)
{
    return s_dl1b_distance_mm;
}
