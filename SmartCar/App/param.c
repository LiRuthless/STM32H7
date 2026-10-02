// ============================================================
// 文件名: param.c
// 功能说明: 参数存储模块，使用 W25Q64 参数区及 CRC16 镜像。
// ============================================================

#include "bsp.h"
#include "app_config.h"
#include "param.h"
#include "menu.h"
#include "pid.h"
#include "motor.h"
#include <stddef.h>
#include <string.h>

#define PARAM_MAGIC         0xA55A3C3Cu
#define PARAM_VERSION       2u
#define PARAM_VERSION_OLD   1u
#define PARAM_FLASH_OFFSET  0u

/* v1 结构必须与旧镜像字节布局一致，供一次性迁移使用。 */
typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t crc16;
    uint8_t page;
    uint8_t arrow;
    uint8_t reserved[2];
    float KP_v;
    float KI_v;
    float KP_x_low;
    float K2P_x_low;
    float KD_x_low;
    int16_t base_speed_low;
    int16_t fan_duty_low;
    float KP_x_high;
    float K2P_x_high;
    float KD_x_high;
    int16_t base_speed_high;
    int16_t fan_duty_high;
} param_store_v1_t;

/* v2 保留 v1 全部字段和偏移，仅在镜像尾部追加两档空闲风扇值。 */
typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t crc16;
    uint8_t page;
    uint8_t arrow;
    uint8_t reserved[2];
    float KP_v;
    float KI_v;
    float KP_x_low;
    float K2P_x_low;
    float KD_x_low;
    int16_t base_speed_low;
    int16_t fan_duty_low;       /* 运行风扇值 */
    float KP_x_high;
    float K2P_x_high;
    float KD_x_high;
    int16_t base_speed_high;
    int16_t fan_duty_high;      /* 运行风扇值 */
    int16_t fan_duty_idle_low;
    int16_t fan_duty_idle_high;
} param_store_t;

typedef char param_v1_layout_size_check[(sizeof(param_store_v1_t) == 52u) ? 1 : -1];
typedef char param_v2_layout_size_check[(sizeof(param_store_t) == 56u) ? 1 : -1];
typedef char param_v1_payload_offset_check[(offsetof(param_store_v1_t, page) == 8u) ? 1 : -1];
typedef char param_v2_payload_offset_check[(offsetof(param_store_t, page) == 8u) ? 1 : -1];
typedef char param_v2_legacy_tail_check[(offsetof(param_store_t, fan_duty_high) == 50u) ? 1 : -1];
typedef char param_v2_idle_offset_check[(offsetof(param_store_t, fan_duty_idle_low) == 52u) ? 1 : -1];

static param_store_t s_param;
static uint8_t s_gear = PARAM_GEAR_LOW;
static uint8_t s_dirty;
static uint8_t s_flash_usable;
static uint32_t s_dirty_tick;

static uint16_t crc16_ccitt(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFFu;
    while(len--)
    {
        crc ^= (uint16_t)(*data++) << 8;
        for(uint8_t i = 0; i < 8; i++)
        {
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static uint16_t param_crc_v1(const param_store_v1_t *s)
{
    return crc16_ccitt((const uint8_t *)s + offsetof(param_store_v1_t, page),
                       sizeof(*s) - offsetof(param_store_v1_t, page));
}

static uint16_t param_crc(const param_store_t *s)
{
    return crc16_ccitt((const uint8_t *)s + offsetof(param_store_t, page),
                       sizeof(*s) - offsetof(param_store_t, page));
}

static int16_t clamp_fan_duty(int16_t duty)
{
    if(duty < 0) return 0;
    if(duty > 10000) return 10000;
    return duty;
}

static void param_set_defaults(param_store_t *s)
{
    memset(s, 0, sizeof(*s));
    s->magic = PARAM_MAGIC;
    s->version = PARAM_VERSION;
    s->page = PAGE_HOME;
    s->arrow = 1;
    s->KP_v = 20.0f;
    s->KI_v = 0.75f;
    s->fan_duty_low = FAN_DUTY_RUN;
    s->fan_duty_high = FAN_DUTY_RUN;
    s->fan_duty_idle_low = FAN_DUTY_IDLE;
    s->fan_duty_idle_high = FAN_DUTY_IDLE;
}

static uint8_t param_write_mirror(void)
{
    s_param.magic = PARAM_MAGIC;
    s_param.version = PARAM_VERSION;
    s_param.crc16 = param_crc(&s_param);
    if(BSP_Flash_Write(PARAM_FLASH_OFFSET, (const uint8_t *)&s_param, sizeof(s_param)) != 0u)
    {
        BSP_UART_WriteString("parameter save fail\r\n");
        return 0u;
    }
    return 1u;
}

static void param_migrate_v1(const param_store_v1_t *old)
{
    memset(&s_param, 0, sizeof(s_param));
    s_param.magic = PARAM_MAGIC;
    s_param.version = PARAM_VERSION;
    s_param.page = old->page;
    s_param.arrow = old->arrow;
    memcpy(s_param.reserved, old->reserved, sizeof(s_param.reserved));
    s_param.KP_v = old->KP_v;
    s_param.KI_v = old->KI_v;
    s_param.KP_x_low = old->KP_x_low;
    s_param.K2P_x_low = old->K2P_x_low;
    s_param.KD_x_low = old->KD_x_low;
    s_param.base_speed_low = old->base_speed_low;
    s_param.fan_duty_low = old->fan_duty_low;
    s_param.KP_x_high = old->KP_x_high;
    s_param.K2P_x_high = old->K2P_x_high;
    s_param.KD_x_high = old->KD_x_high;
    s_param.base_speed_high = old->base_speed_high;
    s_param.fan_duty_high = old->fan_duty_high;
    s_param.fan_duty_idle_low = FAN_DUTY_IDLE;
    s_param.fan_duty_idle_high = FAN_DUTY_IDLE;
}

static void gear_to_global(uint8_t gear)
{
    if(gear == PARAM_GEAR_HIGH)
    {
        PID_SetGain(PID_GAIN_KP_X, s_param.KP_x_high);
        PID_SetGain(PID_GAIN_K2P_X, s_param.K2P_x_high);
        PID_SetGain(PID_GAIN_KD_X, s_param.KD_x_high);
        Motor_SetBaseSpeed(s_param.base_speed_high);
        Motor_SetFanDuty(s_param.fan_duty_high);
        Motor_SetFanDutyIdle(s_param.fan_duty_idle_high);
    }
    else
    {
        PID_SetGain(PID_GAIN_KP_X, s_param.KP_x_low);
        PID_SetGain(PID_GAIN_K2P_X, s_param.K2P_x_low);
        PID_SetGain(PID_GAIN_KD_X, s_param.KD_x_low);
        Motor_SetBaseSpeed(s_param.base_speed_low);
        Motor_SetFanDuty(s_param.fan_duty_low);
        Motor_SetFanDutyIdle(s_param.fan_duty_idle_low);
    }
}

void Param_SelectGear(uint8_t gear)
{
    s_gear = (gear == PARAM_GEAR_HIGH) ? PARAM_GEAR_HIGH : PARAM_GEAR_LOW;
    gear_to_global(s_gear);
}

uint8_t Param_Load(void)
{
    uint8_t header[8];
    uint8_t flash_ok = 1u;
    uint8_t rewrite = 0u;

    s_dirty = 0u;
    if(BSP_Flash_Read(PARAM_FLASH_OFFSET, header, sizeof(header)) != 0u)
    {
        BSP_UART_WriteString("parameter read fail; defaults\r\n");
        param_set_defaults(&s_param);
        rewrite = 1u;
    }
    else if(((uint32_t)header[0] | ((uint32_t)header[1] << 8) |
             ((uint32_t)header[2] << 16) | ((uint32_t)header[3] << 24)) != PARAM_MAGIC)
    {
        BSP_UART_WriteString("parameter invalid; defaults\r\n");
        param_set_defaults(&s_param);
        rewrite = 1u;
    }
    else
    {
        uint16_t version = (uint16_t)(header[4] | ((uint16_t)header[5] << 8));
        if(version == PARAM_VERSION_OLD)
        {
            param_store_v1_t old;
            if(BSP_Flash_Read(PARAM_FLASH_OFFSET, (uint8_t *)&old, sizeof(old)) != 0u ||
               old.magic != PARAM_MAGIC || old.version != PARAM_VERSION_OLD ||
               old.crc16 != param_crc_v1(&old))
            {
                BSP_UART_WriteString("parameter invalid; defaults\r\n");
                param_set_defaults(&s_param);
                rewrite = 1u;
            }
            else
            {
                param_migrate_v1(&old);
                rewrite = 1u;
            }
        }
        else if(version == PARAM_VERSION)
        {
            if(BSP_Flash_Read(PARAM_FLASH_OFFSET, (uint8_t *)&s_param, sizeof(s_param)) != 0u ||
               s_param.magic != PARAM_MAGIC || s_param.version != PARAM_VERSION ||
               s_param.crc16 != param_crc(&s_param))
            {
                BSP_UART_WriteString("parameter invalid; defaults\r\n");
                param_set_defaults(&s_param);
                rewrite = 1u;
            }
        }
        else
        {
            BSP_UART_WriteString("parameter version unsupported; defaults\r\n");
            param_set_defaults(&s_param);
            rewrite = 1u;
        }
    }

    if(s_param.fan_duty_low < 0 || s_param.fan_duty_low > 10000 ||
       s_param.fan_duty_high < 0 || s_param.fan_duty_high > 10000 ||
       s_param.fan_duty_idle_low < 0 || s_param.fan_duty_idle_low > 10000 ||
       s_param.fan_duty_idle_high < 0 || s_param.fan_duty_idle_high > 10000)
    {
        s_param.fan_duty_low = clamp_fan_duty(s_param.fan_duty_low);
        s_param.fan_duty_high = clamp_fan_duty(s_param.fan_duty_high);
        s_param.fan_duty_idle_low = clamp_fan_duty(s_param.fan_duty_idle_low);
        s_param.fan_duty_idle_high = clamp_fan_duty(s_param.fan_duty_idle_high);
        rewrite = 1u;
    }

    if(rewrite && !param_write_mirror())
    {
        flash_ok = 0u;
    }
    s_flash_usable = flash_ok;

    PID_SetGain(PID_GAIN_KP_V, s_param.KP_v);
    PID_SetGain(PID_GAIN_KI_V, s_param.KI_v);
    s_gear = PARAM_GEAR_LOW;
    gear_to_global(s_gear);

    if(s_param.page == PAGE_HOME || s_param.page == PAGE_ADC_ERR ||
       s_param.page == PAGE_SPD_DIS || s_param.page == PAGE_GYRO ||
       s_param.page == PAGE_ADJUST1 || s_param.page == PAGE_ADJUST2)
    {
        page = s_param.page;
    }
    else
    {
        page = PAGE_HOME;
    }
    arrow = (s_param.arrow >= 1 && s_param.arrow <= 5) ? s_param.arrow : 1;
    return flash_ok;
}

void Param_Save(void)
{
    if(!s_flash_usable)
    {
        BSP_UART_WriteString("parameter save fail\r\n");
        s_dirty = 0u;
        return;
    }

    s_param.KP_v = PID_GetState()->KP_v;
    s_param.KI_v = PID_GetState()->KI_v;
    if(s_gear == PARAM_GEAR_HIGH)
    {
        s_param.KP_x_high = PID_GetState()->KP_x;
        s_param.K2P_x_high = PID_GetState()->K2P_x;
        s_param.KD_x_high = PID_GetState()->KD_x;
        s_param.base_speed_high = Motor_GetState()->base_speed;
        s_param.fan_duty_high = clamp_fan_duty(Motor_GetState()->fan_duty);
        s_param.fan_duty_idle_high = clamp_fan_duty(Motor_GetState()->fan_duty_idle);
    }
    else
    {
        s_param.KP_x_low = PID_GetState()->KP_x;
        s_param.K2P_x_low = PID_GetState()->K2P_x;
        s_param.KD_x_low = PID_GetState()->KD_x;
        s_param.base_speed_low = Motor_GetState()->base_speed;
        s_param.fan_duty_low = clamp_fan_duty(Motor_GetState()->fan_duty);
        s_param.fan_duty_idle_low = clamp_fan_duty(Motor_GetState()->fan_duty_idle);
    }
    s_param.page = page;
    s_param.arrow = arrow;
    if(!param_write_mirror())
    {
        s_flash_usable = 0u;
    }
    s_dirty = 0u;
}

void Param_MarkDirty(void)
{
    s_dirty = 1u;
    s_dirty_tick = HAL_GetTick();
}

void Param_ServiceSave(void)
{
    if(s_dirty && (uint32_t)(HAL_GetTick() - s_dirty_tick) >= 3000u)
    {
        Param_Save();
    }
}

void Param_FlushPending(void)
{
    if(s_dirty)
    {
        Param_Save();
    }
}
