/**
 * @file param.c
 * @brief App 参数 bank 和 v1/v2/v3 镜像兼容；Flash 只由前台访问。
 * @details 维护高低档参数镜像，完成默认值、版本迁移、CRC 校验、延迟保存和运行配置应用。
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
 * @details 按函数体既定循环顺序累计或更新数据，并在循环结束后发布结果；循环边界和整数运算顺序保持不变。
 * @param[in] data 非空字节数组，至少 len 字节。
 * @param[in] len 覆盖字节数，可为 0。
 * @return 原 16 位 CRC；len=0 返回初值。
 * @note 初值 0xFFFF、多项式 0x1021，保持移位与转换顺序。
 */
static uint16_t Param_CalculateCrc16(const uint8_t *data, uint32_t len)
{
    /* 逐字节执行与日志相同的 CCITT 高位优先计算；调用方另行决定覆盖的镜像版本和字节范围。 */
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
 * @details 按现有调用顺序使用 Param_CalculateCrc16 完成本函数处理。
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
 * @details 按现有调用顺序使用 Param_CalculateCrc16 完成本函数处理。
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
 * @details 按现有调用顺序使用 Param_CalculateCrc16 完成本函数处理。
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
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
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
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
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
 * @details 按现有调用顺序使用 memset、Param_SetOpticalDefaults 完成本函数处理。
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
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
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
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
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

/**
 * @brief 读取显式模式/档位的方向 bank。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @param[in] mode 0 电磁、1 光电；其他值无效。
 * @param[in] gear 0 低档、1 高档；其他值无效。
 * @return 有效索引返回非空只读指针，否则返回空指针。
 * @note 指针指向模块持有的 bank，直至后续加载或设置有效，不可直接修改。
 */
const param_direction_t *Param_GetDirection(uint8_t mode, uint8_t gear)
{
    return mode < 2 && gear < 2 ? &s_direction[mode][gear] : 0;
}

/**
 * @brief 把当前模式/档位的 bank 分发给控制器。
 * @details 按现有调用顺序使用 Param_GetDirection、Tracking_GetState、PID_SetTrackGains、Motor_SetBaseSpeed
 *     完成本函数处理。
 * @note 保持方向四增益后基础速度的顺序，阻尼固定 1.0f；不将运行状态表覆写值回收进 bank。
 */
void Param_ApplyCurrentDirection(void)
{
    const param_direction_t *p
        = Param_GetDirection(Tracking_GetState()->mode, Tracking_GetState()->gear);
    const pid_track_gains_t gains = {p->kp, p->k2p, p->kd, 1.0f};
    PID_SetTrackGains(&gains);
    Motor_SetBaseSpeed(p->base_speed);
}

/**
 * @brief 保存指定 bank，并按当前选择应用。
 * @details 按现有调用顺序使用 Param_MarkDirty、Tracking_GetState、Param_ApplyCurrentDirection
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] mode 0 电磁、1 光电，其他值拒绝。
 * @param[in] gear 0 低档、1 高档，其他值拒绝。
 * @param[in] p p 为调用方提供的只读输入，必须指向满足接口容量要求的有效对象。
 * @return 有效索引且指针非空返回 1，否则 0 且不写 bank。
 * @note 停车调参路径调用；写 bank 后标脏，只有当前模式及档位才更新活动增益。
 */
uint8_t Param_SetDirection(uint8_t mode, uint8_t gear, const param_direction_t *p)
{
    /* 先校验模式、档位和输入指针，再复制一组方向参数；若目标正是当前模式/档位则立即应用，最后标记延迟保存。 */
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

/**
 * @brief 读取两档共用风扇参数。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @param[in] gear 0 低档、1 高档；其他值返回 0。
 * @param[in] idle 0 运行值，非零空闲值。
 * @return 有效档位返回原 int16_t 万分比，约定 0..10000。
 * @note 只读，不按循迹模式拆分风扇 bank。
 */
int16_t Param_GetFan(uint8_t gear, uint8_t idle)
{
    if (gear >= 2)
    {
        return 0;
    }
    return gear ? (idle ? s_param.fan_duty_idle_high : s_param.fan_duty_high)
                : (idle ? s_param.fan_duty_idle_low : s_param.fan_duty_low);
}

/**
 * @brief 钳位并保存风扇参数，当前档同步活动值。
 * @details 先把输入占空比限制到 0..10000，再按档位和运行/空闲索引写入对应参数槽；其他三项风扇参数保持不变。
 * @param[in] gear 0 低档、1 高档；其他值不操作。
 * @param[in] idle 0 运行值，非零空闲值。
 * @param[in] value 风扇占空比，单位为万分比；写入前钳位到 0..10000。
 * @note 停车前台调用；有效档位会更新镜像、按当前档位应用输出并标记延迟保存。
 */
void Param_SetFan(uint8_t gear, uint8_t idle, int16_t value)
{
    /* 先校验档位并钳位万分比，再按 idle 选择运行或空闲槽；当前档位同步更新输出，最后标记延迟保存。 */
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

/**
 * @brief 切换活动档位并分发方向与共用风扇值。
 * @details 按现有调用顺序使用 Tracking_SetGear、Param_ApplyCurrentDirection、Motor_SetFanDuty、Param_GetFan
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] gear 0 低档、1 高档，其他值不操作。
 * @note 停车菜单或加载路径调用，不改变当前循迹模式。
 */
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
 * @details 按现有调用顺序使用 Param_StoreBanks、Param_CalculateCrcV3、BSP_Flash_Write、BSP_UART_WriteString
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
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

/**
 * @brief 读取 v1/v2/v3 镜像并迁移到原 88 B v3。
 * @details 按现有调用顺序使用 Param_SetDefaults、BSP_Flash_Read、Param_CalculateCrcV1、memcpy
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @return Flash 可继续服务返回 1，读错或必要重写失败返回 0；RAM 状态仍按原流程装载。
 * @note 启动前台调用，可阻塞；保持错误回显、CRC、默认/迁移值、风扇钳位和重写失败路径。
 */
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

/**
 * @brief 标记参数待写并更新静默起点。
 * @details 按现有调用顺序使用 HAL_GetTick 完成本函数处理。
 * @note 停车调参路径调用，HAL 毫秒 tick；不立即写 Flash。
 */
void Param_MarkDirty(void)
{
    s_dirty = 1;
    s_dirty_tick = HAL_GetTick();
}

/**
 * @brief 将待写 bank 和共享参数同步到参数镜像。
 * @details 按现有调用顺序使用 PID_GetState、Param_WriteMirror 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @return 可用且无待写或保存成功为 1，Flash 不可用或保存失败为 0。
 * @note 停车或起跑门禁前台调用，可能阻塞；写失败锁存 Flash 不可用，标脏保持。
 */
uint8_t Param_TryFlushPending(void)
{
    /* Flash 不可用立即失败；无待保存直接成功，否则尝试写镜像，成功后清脏标志，失败保留待保存状态。 */
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

/**
 * @brief 立即标脏并尝试保存。
 * @details 按现有调用顺序使用 Param_TryFlushPending 完成本函数处理。
 * @note 停车菜单前台调用，可阻塞；保持忽略尝试保存返回值，失败状态由模块锁存。
 */
void Param_Save(void)
{
    s_dirty = 1;
    (void)Param_TryFlushPending();
}

/**
 * @brief 静默满 3000 ms 时尝试保存待写参数。
 * @details 按现有调用顺序使用 HAL_GetTick、Param_TryFlushPending 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 仅停车主循环调用，可阻塞；保留无符号减法回绕和失败锁存。
 */
void Param_ServiceSave(void)
{
    if (s_dirty && (uint32_t)(HAL_GetTick() - s_dirty_tick) >= 3000u)
    {
        (void)Param_TryFlushPending();
    }
}

/**
 * @brief 保留立即尝试保存的无返回包装。
 * @note 前台调用，可能阻塞；保持忽略返回值，不新增错误路径。
 */
void Param_FlushPending(void)
{
    (void)Param_TryFlushPending();
}

/**
 * @brief 复制持久化镜像中的共享速度PI。
 * @param[out] kp 非空比例增益输出。
 * @param[out] ki 非空积分增益输出。
 * @note 前台停车/查询使用，不读取被运行调试临时覆盖的活动PI。
 */
void Param_GetSpeedGains(float *kp, float *ki)
{
    *kp = s_param.kp_v;
    *ki = s_param.ki_v;
}

/**
 * @brief 设置共享速度PI镜像及活动值，保持积分历史。
 * @details 先更新镜像共享值，再分发活动PI并标记原延迟保存，不调用积分复位。
 * @param[in] kp 已由调用方校验的有限比例增益。
 * @param[in] ki 已由调用方校验的有限积分增益。
 * @note 仅停车前台，标脏但不Flash；不改变88B镜像布局。
 */
void Param_SetSpeedGains(float kp, float ki)
{
    s_param.kp_v = kp;
    s_param.ki_v = ki;
    PID_SetGain(PID_GAIN_KP_V, kp);
    PID_SetGain(PID_GAIN_KI_V, ki);
    Param_MarkDirty();
}
