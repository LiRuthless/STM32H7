/**
 * @file bsp_imu660rb.c
 * @brief IMU660RB（LSM6DSR 类）驱动：硬件 SPI4 + CS(PD3)， 寄存器配置序列移植自逐飞 zf_device_imu660rb.c。
 * @details 通过 SPI4 配置 IMU660RB 并阻塞读取三轴原始数据；供启动校准和 TIM15 采样使用。
 */
#include "bsp_imu660rb.h"
#include "spi.h"
#define BSP_IMU660RB_SPI_TIMEOUT_MS (10u) /* SPI 传输超时 */

#define BSP_IMU660RB_SPI_R (0x80u)           /* 读寄存器地址掩码 */
#define BSP_IMU660RB_TIMEOUT_COUNT (0x00FFu) /* 自检超时计数（同 zf） */

#define BSP_IMU660RB_CHIP_ID (0x0Fu) /* WHO_AM_I，读回应为 0x6B */
#define BSP_IMU660RB_CHIP_ID_VALUE (0x6Bu)

#define BSP_IMU660RB_INT1_CTRL (0x0Du)
#define BSP_IMU660RB_CTRL1_XL (0x10u)
#define BSP_IMU660RB_CTRL2_G (0x11u)
#define BSP_IMU660RB_CTRL3_C (0x12u)
#define BSP_IMU660RB_CTRL4_C (0x13u)
#define BSP_IMU660RB_CTRL5_C (0x14u)
#define BSP_IMU660RB_CTRL6_C (0x15u)
#define BSP_IMU660RB_CTRL7_G (0x16u)
#define BSP_IMU660RB_CTRL9_XL (0x18u)

#define BSP_IMU660RB_ACC_ADDRESS (0x28u)  /* 加速度数据起始寄存器 */
#define BSP_IMU660RB_GYRO_ADDRESS (0x22u) /* 陀螺仪数据起始寄存器 */

#define BSP_IMU660RB_ACC_SAMPLE (0x8Cu) /* ±8G，ODR 1.66kHz（采集提速：1ms 采样每拍取到新数据） */
#define BSP_IMU660RB_GYR_SAMPLE (0x8Cu) /* ±2000dps，ODR 1.66kHz */
/* 片选拉低/拉高 */
/**
 * @brief 按给定电平设置 IMU 片选。
 * @details 按现有调用顺序使用 HAL_GPIO_WritePin 完成本函数处理。
 * @param[in] level 片选电平，0 拉低，非零拉高。
 * @note 保持原调度入口与阻塞传输顺序。
 */
static void BSP_IMU660RB_SetChipSelect(uint8_t level)
{
    HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* 写单个寄存器 */
/**
 * @brief 阻塞写入一个 IMU 寄存器，保持 HAL 错误忽略语义。
 * @details 按现有调用顺序使用 BSP_IMU660RB_SetChipSelect、HAL_SPI_Transmit 完成本函数处理。
 * @param[in] reg 寄存器地址，使用调用方指定的原始字节或 16 位地址。
 * @param[in] dat 寄存器数据，0～255。
 * @note 保持原调度入口与阻塞传输顺序。
 */
static void BSP_IMU660RB_WriteRegister(uint8_t reg, uint8_t dat)
{
    uint8_t buf[2] = {reg, dat};
    BSP_IMU660RB_SetChipSelect(0);
    (void)HAL_SPI_Transmit(&hspi4, buf, 2, BSP_IMU660RB_SPI_TIMEOUT_MS);
    BSP_IMU660RB_SetChipSelect(1);
}

/* 读单个寄存器 */
/**
 * @brief 阻塞读取一个 IMU 寄存器，保持 HAL 错误忽略语义。
 * @details 按现有调用顺序使用 BSP_IMU660RB_SetChipSelect、HAL_SPI_Transmit、HAL_SPI_Receive 完成本函数处理。
 * @param[in] reg 寄存器地址，使用调用方指定的原始字节或 16 位地址。
 * @return 原始寄存器字节；接收失败时保留初始化值或 HAL 已写入值。
 * @note 保持原调度入口与阻塞传输顺序。
 */
static uint8_t BSP_IMU660RB_ReadRegister(uint8_t reg)
{
    uint8_t addr = (uint8_t)(reg | BSP_IMU660RB_SPI_R);
    uint8_t dat = 0;
    BSP_IMU660RB_SetChipSelect(0);
    (void)HAL_SPI_Transmit(&hspi4, &addr, 1, BSP_IMU660RB_SPI_TIMEOUT_MS);
    (void)HAL_SPI_Receive(&hspi4, &dat, 1, BSP_IMU660RB_SPI_TIMEOUT_MS);
    BSP_IMU660RB_SetChipSelect(1);
    return dat;
}

/* 连续读多个寄存器 */
/**
 * @brief 按原传输顺序连续读取 IMU 寄存器。
 * @details 按现有调用顺序使用 BSP_IMU660RB_SetChipSelect、HAL_SPI_Transmit、HAL_SPI_Receive 完成本函数处理。
 * @param[in] reg 寄存器地址，使用调用方指定的原始字节或 16 位地址。
 * @param[in,out] dat 数据缓冲，必须非空且具有传输所需容量；HAL 调用期间读取或写入。
 * @param[in] len 传输字节数，保持原参数类型范围。
 * @note 保持原调度入口与阻塞传输顺序。
 */
static void BSP_IMU660RB_ReadRegisters(uint8_t reg, uint8_t *dat, uint16_t len)
{
    uint8_t addr = (uint8_t)(reg | BSP_IMU660RB_SPI_R);
    BSP_IMU660RB_SetChipSelect(0);
    (void)HAL_SPI_Transmit(&hspi4, &addr, 1, BSP_IMU660RB_SPI_TIMEOUT_MS);
    (void)HAL_SPI_Receive(&hspi4, dat, len, BSP_IMU660RB_SPI_TIMEOUT_MS);
    BSP_IMU660RB_SetChipSelect(1);
}

/* 自检：反复读 WHO_AM_I，等于 0x6B 则通过（同 zf 流程） */
/**
 * @brief 轮询 IMU 标识，保持自增条件与一毫秒延时顺序。
 * @details 按现有调用顺序使用 BSP_IMU660RB_ReadRegister、HAL_Delay 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @return 0 标识相符；1 轮询超时。
 * @note 保持原调度入口与阻塞传输顺序。
 */
static uint8_t BSP_IMU660RB_CheckIdentity(void)
{
    /* 循环读取 WHO_AM_I，每次失败读后延时一毫秒；计数超过既有限值返回失败，匹配 0x6B 才成功。 */
    uint8_t dat = 0;
    uint16_t timeout_count = 0;
    do
    {
        if (timeout_count++ > BSP_IMU660RB_TIMEOUT_COUNT)
        {
            return 1;
        }
        dat = BSP_IMU660RB_ReadRegister(BSP_IMU660RB_CHIP_ID);
        HAL_Delay(1);
    } while (BSP_IMU660RB_CHIP_ID_VALUE != dat);
    return 0;
}
/**
 * @brief 检查设备标识并按原序列配置 IMU660RB。
 * @details 按现有调用顺序使用 HAL_Delay、BSP_IMU660RB_CheckIdentity、BSP_IMU660RB_WriteRegister
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @return 0 标识检查成功并完成配置；1 标识轮询超时。
 * @note 启动主循环调用，含 HAL_Delay，配置写入保持原 HAL 返回值忽略行为。
 */
uint8_t BSP_IMU660RB_Init(void)
{
    HAL_Delay(20); /* 等待设备上电 */

    if (BSP_IMU660RB_CheckIdentity()) /* 自检失败 */
    {
        return 1;
    }

    BSP_IMU660RB_WriteRegister(BSP_IMU660RB_INT1_CTRL, 0x03); /* 开启陀螺仪 加速度数据就绪中断 */
    BSP_IMU660RB_WriteRegister(
        BSP_IMU660RB_CTRL1_XL, BSP_IMU660RB_ACC_SAMPLE); /* 加速度 ±8G 1.66kHz，第一级滤波输出 */
    BSP_IMU660RB_WriteRegister(
        BSP_IMU660RB_CTRL2_G, BSP_IMU660RB_GYR_SAMPLE);      /* 陀螺仪 ±2000dps 1.66kHz */
    BSP_IMU660RB_WriteRegister(BSP_IMU660RB_CTRL3_C, 0x44);  /* 使能陀螺仪数字低通滤波器 */
    BSP_IMU660RB_WriteRegister(BSP_IMU660RB_CTRL4_C, 0x02);  /* 使能数字低通滤波器 */
    BSP_IMU660RB_WriteRegister(BSP_IMU660RB_CTRL5_C, 0x00);  /* 加速度计与陀螺仪四舍五入 */
    BSP_IMU660RB_WriteRegister(BSP_IMU660RB_CTRL6_C, 0x00);  /* 加速度高性能模式，陀螺低通 133Hz */
    BSP_IMU660RB_WriteRegister(BSP_IMU660RB_CTRL7_G, 0x00);  /* 陀螺仪高性能模式，关闭高通滤波 */
    BSP_IMU660RB_WriteRegister(BSP_IMU660RB_CTRL9_XL, 0x01); /* 关闭 I3C 接口 */
    return 0;
}

/**
 * @brief 读取三轴加速度原始值。
 * @details 按现有调用顺序使用 BSP_IMU660RB_ReadRegisters 完成本函数处理。
 * @param[out] x X 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] y Y 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] z Z 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @note SPI4 阻塞读取；TIM15 或启动校准调用，不与 LCD 并发；保持原 HAL 错误忽略行为。
 */
void BSP_IMU660RB_GetAcc(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t dat[6];
    BSP_IMU660RB_ReadRegisters(BSP_IMU660RB_ACC_ADDRESS, dat, 6);
    *x = (int16_t)(((uint16_t)dat[1] << 8) | dat[0]);
    *y = (int16_t)(((uint16_t)dat[3] << 8) | dat[2]);
    *z = (int16_t)(((uint16_t)dat[5] << 8) | dat[4]);
}

/**
 * @brief 读取三轴陀螺仪原始值。
 * @details 按现有调用顺序使用 BSP_IMU660RB_ReadRegisters 完成本函数处理。
 * @param[out] x X 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] y Y 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] z Z 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @note SPI4 阻塞读取；TIM15 或启动校准调用，不与 LCD 并发；保持原 HAL 错误忽略行为。
 */
void BSP_IMU660RB_GetGyro(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t dat[6];
    BSP_IMU660RB_ReadRegisters(BSP_IMU660RB_GYRO_ADDRESS, dat, 6);
    *x = (int16_t)(((uint16_t)dat[1] << 8) | dat[0]);
    *y = (int16_t)(((uint16_t)dat[3] << 8) | dat[2]);
    *z = (int16_t)(((uint16_t)dat[5] << 8) | dat[4]);
}
