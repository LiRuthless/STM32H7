/**
 * @file param.c
 * @brief App 参数 bank 和 v1/v2/v3 镜像兼容；Flash 只由前台访问。
 */
/* Phase14: v3 并列参数；v1/v2只读解码，方向保存只从显式bank取值。 */
#include "bsp.h"
#include "app_config.h"
#include "param.h"
#include "menu.h"
#include "pid.h"
#include "motor.h"
#include "control.h"
#include <stddef.h>
#include <string.h>
#define PARAM_MAGIC 0xA55A3C3Cu
#define PARAM_VERSION 3u

typedef struct
{
    uint32_t magic;          /**< 原参数魔数，保持 32 位。 */
    uint16_t version;        /**< 持久化版本，v1/v2 仅解码，v3 写入。 */
    uint16_t crc16;          /**< 从字节 8 开始的原 CCITT 校验。 */
    uint8_t page;            /**< 保存的菜单页面编号；加载校验后应用。 */
    uint8_t arrow;           /**< 保存的光标，加载只接受 1..5。 */
    uint8_t reserved[2];     /**< 原两字节保留区，布局和 CRC 覆盖保持。 */
    float kp_v;              /**< 两模式共用速度比例增益。 */
    float ki_v;              /**< 两模式共用速度积分增益。 */
    float kp_x_low;          /**< 电磁低档方向比例增益。 */
    float k2p_x_low;         /**< 电磁低档方向非线性增益。 */
    float kd_x_low;          /**< 电磁低档方向差分增益。 */
    int16_t base_speed_low;  /**< 电磁低档基础速度，脉冲/2 ms。 */
    int16_t fan_duty_low;    /**< 低档运行风扇万分比，0..10000。 */
    float kp_x_high;         /**< 电磁高档方向比例增益。 */
    float k2p_x_high;        /**< 电磁高档方向非线性增益。 */
    float kd_x_high;         /**< 电磁高档方向差分增益。 */
    int16_t base_speed_high; /**< 电磁高档基础速度，脉冲/2 ms。 */
    int16_t fan_duty_high;   /**< 高档运行风扇万分比，0..10000。 */
} param_store_v1_t;

typedef struct
{
    uint32_t magic;             /**< 原参数魔数，保持 32 位。 */
    uint16_t version;           /**< 持久化版本，v1/v2 仅解码，v3 写入。 */
    uint16_t crc16;             /**< 从字节 8 开始的原 CCITT 校验。 */
    uint8_t page;               /**< 保存的菜单页面编号；加载校验后应用。 */
    uint8_t arrow;              /**< 保存的光标，加载只接受 1..5。 */
    uint8_t reserved[2];        /**< 原两字节保留区，布局和 CRC 覆盖保持。 */
    float kp_v;                 /**< 两模式共用速度比例增益。 */
    float ki_v;                 /**< 两模式共用速度积分增益。 */
    float kp_x_low;             /**< 电磁低档方向比例增益。 */
    float k2p_x_low;            /**< 电磁低档方向非线性增益。 */
    float kd_x_low;             /**< 电磁低档方向差分增益。 */
    int16_t base_speed_low;     /**< 电磁低档基础速度，脉冲/2 ms。 */
    int16_t fan_duty_low;       /**< 低档运行风扇万分比，0..10000。 */
    float kp_x_high;            /**< 电磁高档方向比例增益。 */
    float k2p_x_high;           /**< 电磁高档方向非线性增益。 */
    float kd_x_high;            /**< 电磁高档方向差分增益。 */
    int16_t base_speed_high;    /**< 电磁高档基础速度，脉冲/2 ms。 */
    int16_t fan_duty_high;      /**< 高档运行风扇万分比，0..10000。 */
    int16_t fan_duty_idle_low;  /**< 低档空闲风扇万分比，v2 起保存。 */
    int16_t fan_duty_idle_high; /**< 高档空闲风扇万分比，v2 起保存。 */
} param_store_v2_t;

typedef struct
{
    uint32_t magic;                 /**< 原参数魔数，保持 32 位。 */
    uint16_t version;               /**< 持久化版本，v1/v2 仅解码，v3 写入。 */
    uint16_t crc16;                 /**< 从字节 8 开始的原 CCITT 校验。 */
    uint8_t page;                   /**< 保存的菜单页面编号；加载校验后应用。 */
    uint8_t arrow;                  /**< 保存的光标，加载只接受 1..5。 */
    uint8_t reserved[2];            /**< 原两字节保留区，布局和 CRC 覆盖保持。 */
    float kp_v;                     /**< 两模式共用速度比例增益。 */
    float ki_v;                     /**< 两模式共用速度积分增益。 */
    float kp_x_low;                 /**< 电磁低档方向比例增益。 */
    float k2p_x_low;                /**< 电磁低档方向非线性增益。 */
    float kd_x_low;                 /**< 电磁低档方向差分增益。 */
    int16_t base_speed_low;         /**< 电磁低档基础速度，脉冲/2 ms。 */
    int16_t fan_duty_low;           /**< 低档运行风扇万分比，0..10000。 */
    float kp_x_high;                /**< 电磁高档方向比例增益。 */
    float k2p_x_high;               /**< 电磁高档方向非线性增益。 */
    float kd_x_high;                /**< 电磁高档方向差分增益。 */
    int16_t base_speed_high;        /**< 电磁高档基础速度，脉冲/2 ms。 */
    int16_t fan_duty_high;          /**< 高档运行风扇万分比，0..10000。 */
    int16_t fan_duty_idle_low;      /**< 低档空闲风扇万分比，v2 起保存。 */
    int16_t fan_duty_idle_high;     /**< 高档空闲风扇万分比，v2 起保存。 */
    float optical_kp_low;           /**< 光电低档方向比例增益，v3 新增。 */
    float optical_k2p_low;          /**< 光电低档方向非线性增益，v3 新增。 */
    float optical_kd_low;           /**< 光电低档方向差分增益，v3 新增。 */
    int16_t optical_speed_low;      /**< 光电低档基础速度，脉冲/2 ms。 */
    uint16_t optical_reserved_low;  /**< 光电低档两字节保留区，写零。 */
    float optical_kp_high;          /**< 光电高档方向比例增益，v3 新增。 */
    float optical_k2p_high;         /**< 光电高档方向非线性增益，v3 新增。 */
    float optical_kd_high;          /**< 光电高档方向差分增益，v3 新增。 */
    int16_t optical_speed_high;     /**< 光电高档基础速度，脉冲/2 ms。 */
    uint16_t optical_reserved_high; /**< 光电高档两字节保留区，写零。 */
} param_store_t;

typedef char param_v1_size_assert_t[(sizeof(param_store_v1_t) == 52) ? 1 : -1];
typedef char param_v2_size_assert_t[(sizeof(param_store_v2_t) == 56) ? 1 : -1];
typedef char param_v3_size_assert_t[(sizeof(param_store_t) == 88) ? 1 : -1];
typedef char param_v3_offset_assert_t[(offsetof(param_store_t, optical_kp_low) == 56
                                          && offsetof(param_store_t, optical_kp_high) == 72)
        ? 1
        : -1];
/* 前台持有镜像、四方向 bank 及保存状态；控制拍仅按现有调用读取，不新增并发写入。 */
static param_store_t s_param;
static param_direction_t s_direction[2][2];
static uint8_t s_dirty;
static uint8_t s_flash_usable;
static uint32_t s_dirty_tick;

static uint16_t Param_CalculateCrc16(const uint8_t *data, uint32_t len);
static uint16_t Param_CalculateCrcV1(const param_store_v1_t *s);
static uint16_t Param_CalculateCrcV2(const param_store_v2_t *s);
static uint16_t Param_CalculateCrcV3(const param_store_t *s);
static int16_t Param_ClampFanDuty(int16_t x);
static void Param_SetOpticalDefaults(void);
static void Param_SetDefaults(void);
static void Param_LoadBanks(void);
static void Param_StoreBanks(void);
static uint8_t Param_WriteMirror(void);

/**
 * @brief 按原 CCITT 位序计算 CRC16。
 * @param[in] data 非空字节数组，至少 len 字节。
 * @param[in] len 覆盖字节数，可为 0。
 * @return 原 16 位 CRC；len=0 返回初值。
 * @note 初值 0xFFFF、多项式 0x1021，保持移位与转换顺序。
 */
static uint16_t Param_CalculateCrc16(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFFu;
    while (len--)
    {
        crc ^= (uint16_t)(*data++) << 8;
        for (uint8_t i = 0; i < 8; i++)
        {
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

/**
 * @brief 计算 v1 镜像既定覆盖区 CRC。
 * @param[in] s 非空 param_store_v1_t 镜像，从字节 8 开始覆盖 44 字节。
 * @return 原 CCITT CRC16。
 * @note 不覆盖 magic/version/crc16，不改变填充字节和布局。
 */
static uint16_t Param_CalculateCrcV1(const param_store_v1_t *s)
{
    return Param_CalculateCrc16((const uint8_t *)s + 8, 44);
}

/**
 * @brief 计算 v2 镜像既定覆盖区 CRC。
 * @param[in] s 非空 param_store_v2_t 镜像，从字节 8 开始覆盖 48 字节。
 * @return 原 CCITT CRC16。
 * @note 不覆盖 magic/version/crc16，不改变填充字节和布局。
 */
static uint16_t Param_CalculateCrcV2(const param_store_v2_t *s)
{
    return Param_CalculateCrc16((const uint8_t *)s + 8, 48);
}

/**
 * @brief 计算 v3 镜像既定覆盖区 CRC。
 * @param[in] s 非空 param_store_t 镜像，从字节 8 开始覆盖 80 字节。
 * @return 原 CCITT CRC16。
 * @note 不覆盖 magic/version/crc16，不改变填充字节和布局。
 */
static uint16_t Param_CalculateCrcV3(const param_store_t *s)
{
    return Param_CalculateCrc16((const uint8_t *)s + 8, 80);
}

/**
 * @brief 按原三元式钳位风扇万分比。
 * @param[in] x int16_t 原风扇值。
 * @return 小于 0 返回 0，大于 10000 返回 10000，否则 x。
 * @note 不改变方向或速度参数。
 */
static int16_t Param_ClampFanDuty(int16_t x)
{
    return x < 0 ? 0 : (x > 10000 ? 10000 : x);
}

/**
 * @brief 设置原两档光电默认增益与基础速度。
 * @note 仅默认初始化/旧镜像迁移调用；保留连锁赋值、保留字段清零和全部数值。
 */
static void Param_SetOpticalDefaults(void)
{
    s_param.optical_kp_low = s_param.optical_kp_high = 2.0f;
    s_param.optical_k2p_low = s_param.optical_k2p_high = 0.008f;
    s_param.optical_kd_low = s_param.optical_kd_high = 15.0f;
    s_param.optical_speed_low = s_param.optical_speed_high = 100;
    s_param.optical_reserved_low = s_param.optical_reserved_high = 0;
}

/**
 * @brief 清镜像并设置原公共默认值。
 * @note 仅加载路径调用；不凭空填写电磁方向值或真实灰度标定。
 */
static void Param_SetDefaults(void)
{
    memset(&s_param, 0, sizeof(s_param));
    s_param.magic = PARAM_MAGIC;
    s_param.version = 3;
    s_param.page = MENU_PAGE_HOME;
    s_param.arrow = 1;
    s_param.kp_v = 20.0f;
    s_param.ki_v = 0.75f;
    s_param.fan_duty_low = s_param.fan_duty_high = MOTOR_FAN_DUTY_RUN;
    s_param.fan_duty_idle_low = s_param.fan_duty_idle_high = MOTOR_FAN_DUTY_IDLE;
    Param_SetOpticalDefaults();
}

/**
 * @brief 按显式模式/档位装载四个方向 bank。
 * @note 仅参数加载路径调用，保持电磁低/高、光电低/高顺序与字段类型。
 */
static void Param_LoadBanks(void)
{
    s_direction[0][0] = (param_direction_t){
        s_param.kp_x_low, s_param.k2p_x_low, s_param.kd_x_low, s_param.base_speed_low};
    s_direction[0][1] = (param_direction_t){
        s_param.kp_x_high, s_param.k2p_x_high, s_param.kd_x_high, s_param.base_speed_high};
    s_direction[1][0] = (param_direction_t){s_param.optical_kp_low, s_param.optical_k2p_low,
        s_param.optical_kd_low, s_param.optical_speed_low};
    s_direction[1][1] = (param_direction_t){s_param.optical_kp_high, s_param.optical_k2p_high,
        s_param.optical_kd_high, s_param.optical_speed_high};
}

/**
 * @brief 把显式四 bank 写入持久化镜像。
 * @note 不从被状态表覆写的活动方向增益反推参数；保留保留字段清零顺序。
 */
static void Param_StoreBanks(void)
{
    /* 持久化来源是显式四 bank，不是每拍被状态配置部分覆写的活动 PID。 */
    const param_direction_t *p = &s_direction[0][0];
    s_param.kp_x_low = p->kp;
    s_param.k2p_x_low = p->k2p;
    s_param.kd_x_low = p->kd;
    s_param.base_speed_low = p->base_speed;
    p = &s_direction[0][1];
    s_param.kp_x_high = p->kp;
    s_param.k2p_x_high = p->k2p;
    s_param.kd_x_high = p->kd;
    s_param.base_speed_high = p->base_speed;
    p = &s_direction[1][0];
    s_param.optical_kp_low = p->kp;
    s_param.optical_k2p_low = p->k2p;
    s_param.optical_kd_low = p->kd;
    s_param.optical_speed_low = p->base_speed;
    p = &s_direction[1][1];
    s_param.optical_kp_high = p->kp;
    s_param.optical_k2p_high = p->k2p;
    s_param.optical_kd_high = p->kd;
    s_param.optical_speed_high = p->base_speed;
    s_param.optical_reserved_low = s_param.optical_reserved_high = 0;
}

const param_direction_t *Param_GetDirection(uint8_t mode, uint8_t gear)
{
    return mode < 2 && gear < 2 ? &s_direction[mode][gear] : 0;
}

void Param_ApplyCurrentDirection(void)
{
    const param_direction_t *p
        = Param_GetDirection(Tracking_GetState()->mode, Tracking_GetState()->gear);
    const pid_track_gains_t gains = {p->kp, p->k2p, p->kd, 1.0f};
    PID_SetTrackGains(&gains);
    Motor_SetBaseSpeed(p->base_speed);
}

uint8_t Param_SetDirection(uint8_t mode, uint8_t gear, const param_direction_t *p)
{
    if (mode >= 2 || gear >= 2 || !p)
    {
        return 0;
    }
    s_direction[mode][gear] = *p;
    Param_MarkDirty();
    if (mode == Tracking_GetState()->mode && gear == Tracking_GetState()->gear)
    {
        Param_ApplyCurrentDirection();
    }
    return 1;
}

int16_t Param_GetFan(uint8_t gear, uint8_t idle)
{
    if (gear >= 2)
    {
        return 0;
    }
    return gear ? (idle ? s_param.fan_duty_idle_high : s_param.fan_duty_high)
                : (idle ? s_param.fan_duty_idle_low : s_param.fan_duty_low);
}

void Param_SetFan(uint8_t gear, uint8_t idle, int16_t value)
{
    if (gear >= 2)
    {
        return;
    }
    value = Param_ClampFanDuty(value);
    if (gear)
    {
        if (idle)
        {
            s_param.fan_duty_idle_high = value;
        }
        else
        {
            s_param.fan_duty_high = value;
        }
    }
    else
    {
        if (idle)
        {
            s_param.fan_duty_idle_low = value;
        }
        else
        {
            s_param.fan_duty_low = value;
        }
    }
    if (gear == Tracking_GetState()->gear)
    {
        if (idle)
        {
            Motor_SetFanDutyIdle(value);
        }
        else
        {
            Motor_SetFanDuty(value);
        }
    }
    Param_MarkDirty();
}

void Param_SelectGear(uint8_t gear)
{
    if (gear >= 2)
    {
        return;
    }
    Tracking_SetGear(gear);
    Param_ApplyCurrentDirection();
    Motor_SetFanDuty(Param_GetFan(gear, 0));
    Motor_SetFanDutyIdle(Param_GetFan(gear, 1));
}

/**
 * @brief 整理 bank 并计算 CRC 后写原 v3 镜像。
 * @return 写成功 1，BSP 写错打印后返回 0。
 * @note 前台调用，可能阻塞；保持写入地址、88 B 长度与错误打印。
 */
static uint8_t Param_WriteMirror(void)
{
    Param_StoreBanks();
    s_param.magic = PARAM_MAGIC;
    s_param.version = 3;
    s_param.crc16 = Param_CalculateCrcV3(&s_param);
    if (BSP_Flash_Write(0, (const uint8_t *)&s_param, sizeof(s_param)))
    {
        BSP_UART_WriteString("parameter save fail\r\n");
        return 0;
    }
    return 1;
}

uint8_t Param_Load(void)
{
    uint8_t header[8];
    uint8_t rewrite = 0;
    uint8_t io_error = 0;
    uint8_t invalid = 0;
    uint16_t version = 0;
    s_dirty = 0;
    Param_SetDefaults();
    if (BSP_Flash_Read(0, header, 8))
    {
        io_error = 1;
    }
    else
    {
        uint32_t magic = (uint32_t)header[0] | ((uint32_t)header[1] << 8)
            | ((uint32_t)header[2] << 16) | ((uint32_t)header[3] << 24);
        if (magic == PARAM_MAGIC)
        {
            version = (uint16_t)(header[4] | ((uint16_t)header[5] << 8));
        }
        if (version == 1)
        {
            param_store_v1_t old;
            if (BSP_Flash_Read(0, (uint8_t *)&old, sizeof(old)))
            {
                io_error = 1;
            }
            else if (old.magic == PARAM_MAGIC && old.version == 1
                && old.crc16 == Param_CalculateCrcV1(&old))
            {
                memcpy(&s_param, &old, sizeof(old));
                s_param.fan_duty_idle_low = s_param.fan_duty_idle_high = MOTOR_FAN_DUTY_IDLE;
                Param_SetOpticalDefaults();
                rewrite = 1;
            }
            else
            {
                rewrite = invalid = 1;
            }
        }
        else if (version == 2)
        {
            param_store_v2_t old;
            if (BSP_Flash_Read(0, (uint8_t *)&old, sizeof(old)))
            {
                io_error = 1;
            }
            else if (old.magic == PARAM_MAGIC && old.version == 2
                && old.crc16 == Param_CalculateCrcV2(&old))
            {
                memcpy(&s_param, &old, sizeof(old));
                Param_SetOpticalDefaults();
                rewrite = 1;
            }
            else
            {
                rewrite = invalid = 1;
            }
        }
        else if (version == 3)
        {
            param_store_t current;
            if (BSP_Flash_Read(0, (uint8_t *)&current, sizeof(current)))
            {
                io_error = 1;
            }
            else if (current.magic == PARAM_MAGIC && current.version == 3
                && current.crc16 == Param_CalculateCrcV3(&current))
            {
                s_param = current;
            }
            else
            {
                rewrite = invalid = 1;
            }
        }
        else
        {
            rewrite = invalid = 1;
        }
    }
    if (io_error)
    {
        BSP_UART_WriteString("parameter read fail; defaults\r\n");
    }
    else if (invalid)
    {
        BSP_UART_WriteString("parameter invalid; defaults\r\n");
    }
    /* 旧兼容风扇钳位；方向/速度字段保持原值，不用默认值覆盖有效旧镜像。 */
    {
        int16_t *fans[] =
        {
            &s_param.fan_duty_low,
            &s_param.fan_duty_high,
            &s_param.fan_duty_idle_low,
            &s_param.fan_duty_idle_high
        };
        for (uint8_t i = 0; i < 4; ++i)
        {
            int16_t x = Param_ClampFanDuty(*fans[i]);
            if (x != *fans[i])
            {
                *fans[i] = x;
                rewrite = 1;
            }
        }
    }
    Param_LoadBanks();
    s_flash_usable = (uint8_t)!io_error;
    if (rewrite && s_flash_usable && !Param_WriteMirror())
    {
        s_flash_usable = 0;
    }
    PID_SetGain(PID_GAIN_KP_V, s_param.kp_v);
    PID_SetGain(PID_GAIN_KI_V, s_param.ki_v);
    Param_SelectGear(PARAM_GEAR_LOW);
    if (s_param.page == MENU_PAGE_HOME || s_param.page == MENU_PAGE_ADC_ERR
        || s_param.page == MENU_PAGE_SPD_DIS || s_param.page == MENU_PAGE_GYRO
        || s_param.page == MENU_PAGE_ADJUST1 || s_param.page == MENU_PAGE_ADJUST2)
    {
        g_menu_page = s_param.page;
    }
    else
    {
        g_menu_page = MENU_PAGE_HOME;
    }
    g_menu_arrow = (s_param.arrow >= 1 && s_param.arrow <= 5) ? s_param.arrow : 1;
    return s_flash_usable;
}

void Param_MarkDirty(void)
{
    s_dirty = 1;
    s_dirty_tick = HAL_GetTick();
}

uint8_t Param_TryFlushPending(void)
{
    if (!s_flash_usable)
    {
        return 0;
    }
    if (!s_dirty)
    {
        return 1;
    }
    s_param.kp_v = PID_GetState()->kp_v;
    s_param.ki_v = PID_GetState()->ki_v;
    s_param.page = g_menu_page;
    s_param.arrow = g_menu_arrow;
    if (!Param_WriteMirror())
    {
        s_flash_usable = 0;
        return 0;
    }
    s_dirty = 0;
    return 1;
}

void Param_Save(void)
{
    s_dirty = 1;
    (void)Param_TryFlushPending();
}

void Param_ServiceSave(void)
{
    if (s_dirty && (uint32_t)(HAL_GetTick() - s_dirty_tick) >= 3000u)
    {
        (void)Param_TryFlushPending();
    }
}

void Param_FlushPending(void)
{
    (void)Param_TryFlushPending();
}
