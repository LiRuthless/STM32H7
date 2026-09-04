// ============================================================
// 文件名: param.c
// 功能说明: 参数存储模块（替代源 eeprom.c）
// 将菜单状态、速度环/方向环 PID 参数、两档速度参数整体打包，
// 带 magic/version/crc16 校验存入板载 W25Q64；float 按 4 字节完整存取
// （源工程 extern_iap_write_buff 只写 2 字节的 bug 不复刻）。
// ============================================================

#include "bsp.h"
#include "param.h"
#include "menu.h"
#include "pid.h"
#include "motor.h"
#include <stddef.h>
#include <string.h>

#define PARAM_MAGIC         0xA55A3C3Cu     /* 参数区魔数 */
#define PARAM_VERSION       1u              /* 结构体版本 */
#define PARAM_FLASH_OFFSET  0u              /* 扇区内逻辑偏移 */

// ==================== 参数结构体（落盘镜像） ====================
typedef struct
{
    uint32_t magic;             /* 校验魔数 */
    uint16_t version;           /* 结构体版本 */
    uint16_t crc16;             /* 对 page 起至末尾的数据区 CRC16-CCITT */

    uint8_t  page;              /* 菜单页面状态（源 write_menu） */
    uint8_t  arrow;             /* 菜单光标位置 */
    uint8_t  reserved[2];

    float    KP_v;              /* 速度环比例（源 write_pid_v） */
    float    KI_v;              /* 速度环积分 */

    float    KP_x_low;          /* 慢速档：方向环比例（源 write_speed_low） */
    float    K2P_x_low;         /* 慢速档：非线性二次比例 */
    float    KD_x_low;          /* 慢速档：方向环微分 */
    int16_t  base_speed_low;    /* 慢速档：基础速度 */
    int16_t  fan_duty_low;      /* 慢速档：负压风扇占空比 */

    float    KP_x_high;         /* 快速档：方向环比例（源 write_speed_high） */
    float    K2P_x_high;        /* 快速档：非线性二次比例 */
    float    KD_x_high;         /* 快速档：方向环微分 */
    int16_t  base_speed_high;   /* 快速档：基础速度 */
    int16_t  fan_duty_high;     /* 快速档：负压风扇占空比 */
} param_store_t;

static param_store_t s_param;               /* RAM 镜像 */
static uint8_t       s_gear = PARAM_GEAR_LOW;   /* 当前活动档位 */

// ==================== CRC16-CCITT（poly 0x1021，初值 0xFFFF） ====================
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

static uint16_t param_crc(const param_store_t *s)
{
    return crc16_ccitt((const uint8_t *)s + offsetof(param_store_t, page),
                       sizeof(param_store_t) - offsetof(param_store_t, page));
}

// ==================== 默认值 / 分发 / 收集 ====================

// 默认值以源工程初始值为准：KP_v=20.0、KI_v=0.75（源 main.c），
// 方向环与速度/风扇参数为 0（源 pid.c/motor.c 初始值），经菜单或无线调参设定。
static void param_set_defaults(param_store_t *s)
{
    memset(s, 0, sizeof(*s));
    s->magic   = PARAM_MAGIC;
    s->version = PARAM_VERSION;
    s->page    = PAGE_HOME;
    s->arrow   = 1;
    s->KP_v    = 20.0f;
    s->KI_v    = 0.75f;
}

// 把指定档位参数分发到全局（对应源 read_speed_low/high）
static void gear_to_global(uint8_t gear)
{
    if(gear == PARAM_GEAR_HIGH)
    {
        KP_x       = s_param.KP_x_high;
        K2P_x      = s_param.K2P_x_high;
        KD_x       = s_param.KD_x_high;
        base_speed = s_param.base_speed_high;
        fan_duty   = s_param.fan_duty_high;
    }
    else
    {
        KP_x       = s_param.KP_x_low;
        K2P_x      = s_param.K2P_x_low;
        KD_x       = s_param.KD_x_low;
        base_speed = s_param.base_speed_low;
        fan_duty   = s_param.fan_duty_low;
    }
}

// 函数名: Param_SelectGear
// 功能: 切换活动档位并把该档参数分发到全局（菜单进入调参页时调用）
void Param_SelectGear(uint8_t gear)
{
    s_gear = (gear == PARAM_GEAR_HIGH) ? PARAM_GEAR_HIGH : PARAM_GEAR_LOW;
    gear_to_global(s_gear);
}

// 函数名: Param_Load
// 功能: 上电从 Flash 加载参数；magic/crc 校验失败则载入默认值并立即回写
void Param_Load(void)
{
    BSP_Flash_Read(PARAM_FLASH_OFFSET, (uint8_t *)&s_param, sizeof(s_param));

    if(s_param.magic != PARAM_MAGIC ||
       s_param.version != PARAM_VERSION ||
       s_param.crc16 != param_crc(&s_param))
    {
        param_set_defaults(&s_param);
        s_param.crc16 = param_crc(&s_param);
        BSP_Flash_Write(PARAM_FLASH_OFFSET, (const uint8_t *)&s_param, sizeof(s_param));
    }

    /* 分发到全局：速度环直接恢复；方向环默认挂慢速档 */
    KP_v = s_param.KP_v;
    KI_v = s_param.KI_v;
    s_gear = PARAM_GEAR_LOW;
    gear_to_global(s_gear);

    /* 菜单状态（范围校验，非法则回主页） */
    if(s_param.page == PAGE_HOME    || s_param.page == PAGE_ADC_ERR ||
       s_param.page == PAGE_SPD_DIS || s_param.page == PAGE_GYRO    ||
       s_param.page == PAGE_ADJUST1 || s_param.page == PAGE_ADJUST2)
    {
        page = s_param.page;
    }
    else
    {
        page = PAGE_HOME;
    }
    arrow = (s_param.arrow >= 1 && s_param.arrow <= 5) ? s_param.arrow : 1;
}

// 函数名: Param_Save
// 功能: 从全局收集参数写入 Flash（对应源 eeprom_write_*，翻页/退出菜单时调用）
void Param_Save(void)
{
    s_param.KP_v = KP_v;
    s_param.KI_v = KI_v;

    if(s_gear == PARAM_GEAR_HIGH)
    {
        s_param.KP_x_high       = KP_x;
        s_param.K2P_x_high      = K2P_x;
        s_param.KD_x_high       = KD_x;
        s_param.base_speed_high = base_speed;
        s_param.fan_duty_high   = fan_duty;
    }
    else
    {
        s_param.KP_x_low       = KP_x;
        s_param.K2P_x_low      = K2P_x;
        s_param.KD_x_low       = KD_x;
        s_param.base_speed_low = base_speed;
        s_param.fan_duty_low   = fan_duty;
    }

    s_param.page  = page;
    s_param.arrow = arrow;

    s_param.crc16 = param_crc(&s_param);
    BSP_Flash_Write(PARAM_FLASH_OFFSET, (const uint8_t *)&s_param, sizeof(s_param));
}
