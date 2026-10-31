/**
 * @file parameter_store.c
 * @brief 参数镜像兼容与Flash保存。
 * @details 前台拥有镜像及脏标志；加载四bank，实际保存时消费瞬时快照，不访问控制或菜单。
 */
#include "bsp.h"
#include "app_config.h"
#include "parameter_store.h"
#include "parameter_bank.h"
#include <string.h>
#define PARAM_MAGIC 0xA55A3C3Cu
#define PARAM_VERSION 3u

/* 参数镜像与保存可用性仅由前台更新。 */
static param_image_v3_t s_param;
static uint8_t s_dirty;
static uint8_t s_flash_usable;
static uint32_t s_dirty_tick;

/**
 * @brief 按原 CCITT 位序计算 CRC16。
 * @details 逐字节高位先异或，再执行8次条件移位；初值ffff、多项式1021，返回16位结果。
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
 * @details 从字节8开始只覆盖44字节，排除magic、version及crc16字段。
 * @param[in] s 非空 param_image_v1_t 镜像，从字节 8 开始覆盖 44 字节。
 * @return 原 CCITT CRC16。
 * @note 不覆盖 magic/version/crc16，不改变填充字节和布局。
 */
static uint16_t Param_CalculateCrcV1(const param_image_v1_t *s)
{
    return Param_CalculateCrc16((const uint8_t *)s + 8, 44);
}

/**
 * @brief 计算 v2 镜像既定覆盖区 CRC。
 * @details 从字节8开始只覆盖48字节，包含v2新增的空闲风扇字段。
 * @param[in] s 非空 param_image_v2_t 镜像，从字节 8 开始覆盖 48 字节。
 * @return 原 CCITT CRC16。
 * @note 不覆盖 magic/version/crc16，不改变填充字节和布局。
 */
static uint16_t Param_CalculateCrcV2(const param_image_v2_t *s)
{
    return Param_CalculateCrc16((const uint8_t *)s + 8, 48);
}

/**
 * @brief 计算 v3 镜像既定覆盖区 CRC。
 * @details 从字节8开始只覆盖80字节，包含两组光电参数及保留字段。
 * @param[in] s 非空 param_image_v3_t 镜像，从字节 8 开始覆盖 80 字节。
 * @return 原 CCITT CRC16。
 * @note 不覆盖 magic/version/crc16，不改变填充字节和布局。
 */
static uint16_t Param_CalculateCrcV3(const param_image_v3_t *s)
{
    return Param_CalculateCrc16((const uint8_t *)s + 8, 80);
}

/**
 * @brief 按原三元式钳位风扇万分比。
 * @details 输入小于0返回0，大于10000返回10000，否则返回原int16_t值；不修改镜像。
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
 * @details 两档依次设置kp=2、k2p=0.008、kd=15、速度100，并清两个保留字；
 *     仅默认加载和旧版本迁移使用，不初始化电磁方向字段。
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
 * @details 清整份88字节镜像，依次装魔数、版本、schema页面/光标、共享PI和两档风扇默认值，
 *     最后补光电默认；清零后的电磁方向字段不猜测可跑值。
 * @note 仅加载路径调用；不凭空填写电磁方向值或真实灰度标定。
 */
static void Param_SetDefaults(void)
{
    memset(&s_param, 0, sizeof(s_param));
    s_param.magic = PARAM_MAGIC;
    s_param.version = 3;
    s_param.page = PARAM_IMAGE_DEFAULT_PAGE;
    s_param.arrow = PARAM_IMAGE_DEFAULT_ARROW;
    s_param.kp_v = 20.0f;
    s_param.ki_v = 0.75f;
    s_param.fan_duty_low = s_param.fan_duty_high = MOTOR_FAN_DUTY_RUN;
    s_param.fan_duty_idle_low = s_param.fan_duty_idle_high = MOTOR_FAN_DUTY_IDLE;
    Param_SetOpticalDefaults();
}

/**
 * @brief 整理 bank 并计算 CRC 后写原 v3 镜像。
 * @details 从四bank装方向字段，依次写魔数、版本、CRC后向地址0写88字节；
 *     写错只打印原错误文本并返回0，不回滚已整理的RAM镜像。
 * @return 写成功 1，BSP 写错打印后返回 0。
 * @note 前台调用，可能阻塞；保持写入地址、88 B 长度与错误打印。
 */
static uint8_t Param_WriteMirror(void)
{
    ParamBank_StoreBanks(&s_param);
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
 * @brief 读取两档共用风扇参数。
 * @details 档位越界返回0；有效档位按idle零/非零选择运行或空闲镜像槽，只读取不应用。
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
 * @brief 标记参数待写并更新静默起点。
 * @details 先置脏，再读取一次BSP毫秒tick作为3000ms静默保存起点；不写Flash。
 * @note 停车调参路径调用，BSP毫秒tick；不立即写 Flash。
 */
void Param_MarkDirty(void)
{
    s_dirty = 1;
    s_dirty_tick = BSP_Time_GetTick();
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
 * @brief 读取 v1/v2/v3 镜像并迁移到原 88 B v3。
 * @details 先清脏并装默认RAM，再读8字节头并按v1/v2/v3完整读取和CRC判断；
 *     有效旧版补缺失字段，无效数据保留默认并要求重写，读取错误不重写。
 *     随后依次钳位四风扇、装四bank和锁存可用性；必要重写失败锁存不可用，RAM仍可读取。
 * @return Flash 可继续服务返回 1，读错或必要重写失败返回 0；RAM 状态仍按原流程装载。
 * @note 启动前台调用，可阻塞；保持错误回显、CRC、默认/迁移值、风扇钳位和重写失败路径。
 */
uint8_t ParamStore_Load(void)
{
    uint8_t header[8];
    uint8_t rewrite = 0;
    uint8_t io_error = 0;
    uint8_t invalid = 0;
    uint16_t version = 0;
    /* 先建立完整默认RAM；读错时不重写，旧版本只补其尚未定义的字段。 */
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
            param_image_v1_t old;
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
            param_image_v2_t old;
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
            param_image_v3_t current;
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
    /* bank先从已迁移/钳位镜像发布；必要重写失败只锁存Flash不可用，不撤销RAM。 */
    ParamBank_LoadBanks(&s_param);
    s_flash_usable = (uint8_t)!io_error;
    if (rewrite && s_flash_usable && !Param_WriteMirror())
    {
        s_flash_usable = 0;
    }
    return s_flash_usable;
}

/**
 * @brief 只更新指定档位的运行或空闲风扇镜像。
 * @details 档位越界无操作；有效值先钳位到0..10000，再按档位及idle选择唯一镜像槽写入。
 * @param[in] gear 0 低档、1 高档；其他值不操作。
 * @param[in] idle 0 运行值，非零空闲值。
 * @param[in] value 风扇占空比，单位为万分比；写入前钳位到 0..10000。
 * @note 仅前台镜像写入，不应用活动风扇、不标脏、不直接写PWM。
 */
void ParamStore_SetFan(uint8_t gear, uint8_t idle, int16_t value)
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
}

/**
 * @brief 只更新共享速度PI镜像。
 * @details 依次写镜像kp_v、ki_v，不改活动PID或脏标志；应用及标脏由system负责。
 * @param[in] kp 已由调用方校验的有限比例增益。
 * @param[in] ki 已由调用方校验的有限积分增益。
 * @note 仅前台写入，不调用控制器，不复位历史或访问Flash。
 */
void ParamStore_SetSpeedGains(float kp, float ki)
{
    s_param.kp_v = kp;
    s_param.ki_v = ki;
}

/**
 * @brief 将待写 bank 和共享参数同步到参数镜像。
 * @details Flash不可用立即0，无脏立即1且不读输入；需写时依次复制快照PI、页面、光标，
 *     再整理bank并写镜像。失败锁存不可用且保留脏，成功才清脏。
 * @param[in] runtime 非空本次保存快照，仅在可用且脏时读取四字段，不保存地址。
 * @return 可用且无待写或保存成功为 1，Flash 不可用或保存失败为 0。
 * @note 停车或起跑门禁前台调用，可能阻塞；写失败锁存 Flash 不可用，标脏保持。
 */
uint8_t ParamStore_TryFlushPending(const param_save_snapshot_t *runtime)
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
    s_param.kp_v = runtime->kp_v;
    s_param.ki_v = runtime->ki_v;
    s_param.page = runtime->page;
    s_param.arrow = runtime->arrow;
    if (!Param_WriteMirror())
    {
        s_flash_usable = 0;
        return 0;
    }
    s_dirty = 0;
    return 1;
}

/**
 * @brief 复制镜像的菜单位置原字节。
 * @details 不校验页面；由菜单owner执行合法性恢复。
 * @param[out] page 非空页面输出。
 * @param[out] arrow 非空光标输出。
 */
void ParamStore_GetPosition(uint8_t *page, uint8_t *arrow)
{
    *page = s_param.page;
    *arrow = s_param.arrow;
}

/**
 * @brief 读取参数Flash可用状态。
 * @return 条件成立返回1，否则0。
 */
uint8_t ParamStore_IsUsable(void)
{
    return s_flash_usable;
}

/**
 * @brief 读取参数待保存状态。
 * @return 条件成立返回1，否则0。
 */
uint8_t ParamStore_IsDirty(void)
{
    return s_dirty;
}

/**
 * @brief 标记立即保存请求。
 * @details 只置脏，不更新时间；与延迟保存静默时间分离。
 */
void ParamStore_RequestSave(void)
{
    s_dirty = 1;
}

/**
 * @brief 检查静默3000ms保存期限。
 * @details 只有脏状态才读取毫秒时间；无符号相减允许计数回绕。
 * @return 条件成立返回1，否则0。
 */
uint8_t ParamStore_IsSaveDue(void)
{
    return s_dirty && (uint32_t)(BSP_Time_GetTick() - s_dirty_tick) >= 3000u;
}
