/**
 * @file datalog.c
 * @brief App 固定 v2 日志与 CSV；TIM15 生产、TIM6 发布、前台唯一 Flash 消费。
 */
/* Phase14 日志v2：固定little-endian布局，单生产者/单消费者；Flash仅前台访问。
 * TIM6/TIM15同优先级，控制快照发布不交错；其他共享标志用极短PRIMASK锁存。 */
#include "bsp.h"
#include "app_config.h"
#include "datalog.h"
#include "app.h"
#include "control.h"
#include "track_sensor.h"
#include "gray_sensor.h"
#include "optical_fsm.h"
#include "electromagnetic_fsm.h"
#include "param.h"
#include "pid.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define DATALOG_MAGIC 0x324C4353u
#define DATALOG_BATCH_RECORDS 8u
#define DATALOG_ASSERT_LAYOUT(name, expr) typedef char name[(expr) ? 1 : -1]
DATALOG_ASSERT_LAYOUT(datalog_record_size_assert_t, sizeof(datalog_record_t) == 96);
DATALOG_ASSERT_LAYOUT(datalog_header_size_assert_t, sizeof(datalog_header_t) == 256);
DATALOG_ASSERT_LAYOUT(datalog_record_offsets_assert_t,
    offsetof(datalog_record_t, gray_raw) == 32 && offsetof(datalog_record_t, gray_sequence) == 64
        && offsetof(datalog_record_t, protection_flags) == 76
        && offsetof(datalog_record_t, reserved) == 82);
DATALOG_ASSERT_LAYOUT(datalog_header_offsets_assert_t,
    offsetof(datalog_header_t, black) == 28 && offsetof(datalog_header_t, weights) == 92
        && offsetof(datalog_header_t, em) == 108 && offsetof(datalog_header_t, kp_v) == 140
        && offsetof(datalog_header_t, reserved_tail) == 172);
DATALOG_ASSERT_LAYOUT(
    datalog_log_capacity_assert_t, (BSP_FLASH_LOG_SIZE - 256) / 96 == DATALOG_CAPACITY);
/* TIM15 单生产者写槽和 head，前台单消费者写 tail/batch；屏障位置不能移动。 */
static datalog_record_t s_buf[DATALOG_BUFFER_RECORDS];
/* TIM6 发布、TIM15 取用的完整控制快照，同级中断不交错。 */
static datalog_record_t s_snapshot;
/* 前台持有会话头，原 256 B；所有保留区清零后计算 CRC。 */
static datalog_header_t s_header;
static uint8_t s_batch[DATALOG_BATCH_RECORDS * DATALOG_RECORD_SIZE];
static volatile uint16_t s_head;
static volatile uint16_t s_tail;
static datalog_status_t s_status;
static uint8_t s_header_valid; /* 前台建立会话成功后置位，清除操作不伪造新会话。 */
static uint8_t s_write_failed; /* 首次写失败后锁存，active 清零也不能触发重试。 */

static uint16_t Datalog_CalculateCrc16(const uint8_t *data, uint32_t len);
static void Datalog_LatchFault(uint8_t fault);
static void Datalog_RequestFaultStop(uint8_t fault, app_stop_reason_t reason, uint16_t flag);
static void Datalog_SetProfile(datalog_profile_t *dst, const param_direction_t *src);
static uint8_t Datalog_IsHeaderValid(const datalog_header_t *h);

/**
 * @brief 按原 CCITT 位序计算日志校验。
 * @param[in] data 非空数组，至少 len 字节。
 * @param[in] len 覆盖长度，可为 0。
 * @return 原 16 位校验，空长度返回初值。
 * @note 初值 0xffff，多项式 0x1021，不改原常量类型或转换。
 */
static uint16_t Datalog_CalculateCrc16(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xffff;
    while (len--)
    {
        crc ^= (uint16_t)*data++ << 8;
        for (uint8_t i = 0; i < 8; ++i)
        {
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

/**
 * @brief 锁存首次故障并按原规则停生产。
 * @param[in] fault DATALOG_FAULT_* 字节值；READ 不清 active，其他值按原逻辑清零。
 * @note ISR/前台可调用，只在原短临界区更新，不进行 Flash I/O。
 */
static void Datalog_LatchFault(uint8_t fault)
{
    uint32_t mask = BSP_Time_EnterCritical();
    if (!s_status.fault)
    {
        s_status.fault = fault;
    }
    if (fault != DATALOG_FAULT_READ)
    {
        s_status.active = 0;
    }
    BSP_Time_ExitCritical(mask);
}

/**
 * @brief 按原顺序锁存日志故障、保护位和停车请求。
 * @param[in] fault 原 DATALOG_FAULT_* 故障值。
 * @param[in] reason 原 APP_STOP_* 停车原因。
 * @param[in] flag 原 TRACKING_PROTECT_* 对应位。
 * @note ISR/前台可调用，有界；不将重复停车原因覆盖首次原因。
 */
static void Datalog_RequestFaultStop(uint8_t fault, app_stop_reason_t reason, uint16_t flag)
{
    Datalog_LatchFault(fault);
    Tracking_AddProtection(flag);
    App_RequestStop(reason);
}

void Datalog_SetStopSummary(uint8_t reason, uint16_t flags)
{
    uint32_t mask = BSP_Time_EnterCritical();
    if (!s_status.reason)
    {
        s_status.reason = reason;
    }
    s_status.protection_flags |= flags;
    BSP_Time_ExitCritical(mask);
}

const datalog_status_t *Datalog_GetStatus(void)
{
    s_status.pending = s_status.accepted - s_status.written;
    return &s_status;
}

uint8_t Datalog_IsActive(void)
{
    return s_status.active;
}

uint32_t Datalog_GetCount(void)
{
    return s_status.written;
}

void Datalog_Init(void)
{
    s_head = s_tail = 0;
    memset(&s_status, 0, sizeof(s_status));
    memset(&s_snapshot, 0, sizeof(s_snapshot));
    s_header_valid = 0;
    s_write_failed = 0;
}

/**
 * @brief 将方向 bank 写入头部并清保留字。
 * @param[out] dst 非空头部配置对象，按 kp/k2p/kd/base_speed/reserved 顺序写入。
 * @param[in] src 非空显式参数 bank，输入单位保持原控制拍标度。
 * @note 仅前台构造日志头时调用，不读取状态表覆写后的活动参数。
 */
static void Datalog_SetProfile(datalog_profile_t *dst, const param_direction_t *src)
{
    dst->kp = src->kp;
    dst->k2p = src->k2p;
    dst->kd = src->kd;
    dst->base_speed = src->base_speed;
    dst->reserved = 0;
}

uint8_t Datalog_Start(void)
{
    static datalog_header_t s_check;
    const tracking_state_t *t = Tracking_GetState();
    Datalog_Init();
    if (!DATALOG_ENABLE || DATALOG_DIV != 1)
    {
        Datalog_LatchFault(DATALOG_FAULT_CONFIG);
        return 1;
    }
    if (!Gray_CheckCalibration(0))
    {
        Datalog_LatchFault(DATALOG_FAULT_HEADER);
        return 1;
    }
    memset(&s_header, 0, sizeof(s_header));
    s_header.magic = DATALOG_MAGIC;
    s_header.version = 2;
    s_header.header_size = 256;
    s_header.record_size = 96;
    s_header.sample_period_us = 1000;
    s_header.control_period_us = 2000;
    s_header.mode = t->mode;
    s_header.gear = t->gear;
    s_header.channel_count = 16;
    s_header.noise = 50;
    s_header.lost = 200;
    s_header.min_contrast = 200;
    memcpy(s_header.black, Gray_GetBlack(), 32);
    memcpy(s_header.white, Gray_GetWhite(), 32);
    for (uint8_t i = 0; i < 16; ++i)
    {
        s_header.weights[i] = (int8_t)(15 - 2 * (int)i);
    }
    Datalog_SetProfile(&s_header.em, Param_GetDirection(0, t->gear));
    Datalog_SetProfile(&s_header.optical, Param_GetDirection(1, t->gear));
    s_header.kp_v = PID_GetState()->kp_v;
    s_header.ki_v = PID_GetState()->ki_v;
    s_header.fan_run = Param_GetFan(t->gear, 0);
    s_header.fan_idle = Param_GetFan(t->gear, 1);
    s_header.gray_budget_us = 700;
    s_header.control_budget_us = 900;
    s_header.run_delay_count = 1000;
    s_header.capacity = DATALOG_CAPACITY;
    s_header.datalog_div = 1;
    s_header.crc16 = Datalog_CalculateCrc16((const uint8_t *)&s_header + 8, 248);
    if (BSP_Flash_LogErase())
    {
        Datalog_LatchFault(DATALOG_FAULT_UNAVAILABLE);
        return 1;
    }
    if (BSP_Flash_LogWrite(0, (const uint8_t *)&s_header, 256)
        || BSP_Flash_LogRead(0, (uint8_t *)&s_check, 256) || memcmp(&s_check, &s_header, 256))
    {
        Datalog_LatchFault(DATALOG_FAULT_HEADER);
        return 1;
    }
    s_header_valid = 1;
    s_status.active = 1;
    return 0;
}

uint8_t Datalog_Clear(void)
{
    s_status.active = 0;
    s_header_valid = 0;
    /* 擦除不伪造新会话；首次故障和上一会话RAM摘要保留到下次Start。 */
    return BSP_Flash_LogErase();
}

void Datalog_PublishControl(void)
{
    const track_state_t *em = Track_GetState();
    const gray_state_t *g = Gray_GetState();
    const tracking_state_t *t = Tracking_GetState();
    /* 同级 TIM6/TIM15 不互抢：先清保留区，按原位置读取各字段后用屏障发布。 */
    memset(&s_snapshot, 0, sizeof(s_snapshot));
    memcpy(s_snapshot.adc, em->filtered, 8);
    memcpy(s_snapshot.gray_raw, g->raw, 32);
    s_snapshot.track_error = t->active_error;
    s_snapshot.track_out = t->track_out;
    s_snapshot.dl1b_mm = g_app_dl1b_distance_mm;
    s_snapshot.battery = (uint16_t)g_app_battery_filtered;
    s_snapshot.kernel_state = ElectromagneticFSM_GetState()->kernel_state;
    s_snapshot.roundabout_state = ElectromagneticFSM_GetState()->roundabout_state;
    s_snapshot.gray_sequence = g->sequence;
    s_snapshot.control_sequence = t->control_sequence;
    s_snapshot.em_error = em->error;
    s_snapshot.optical_error = g->error;
    s_snapshot.protection_flags = t->protection_flags;
    s_snapshot.mode = t->mode;
    s_snapshot.optical_state = (uint8_t)OpticalFSM_GetState()->state;
    s_snapshot.gray_valid
        = (uint8_t)(g->calibration_valid && g->line_valid && !g->acquisition_status);
    s_snapshot.stop_reason = s_status.reason;
    BSP_Time_MemoryBarrier();
}

void Datalog_Push(void)
{
    uint16_t next;
    int16_t raw[3];
    datalog_record_t rec;
    if (!s_status.active || s_status.fault)
    {
        return;
    }
    if (s_status.accepted >= DATALOG_CAPACITY)
    {
        Datalog_RequestFaultStop(DATALOG_FAULT_FULL, APP_STOP_LOG_FULL, TRACKING_PROTECT_LOG_FULL);
        return;
    }
    next = (uint16_t)((s_head + 1u) % DATALOG_BUFFER_RECORDS);
    if (next == s_tail)
    {
        ++s_status.rejected;
        Datalog_RequestFaultStop(
            DATALOG_FAULT_OVERFLOW, APP_STOP_LOG_OVERFLOW, TRACKING_PROTECT_LOG_OVERFLOW);
        return;
    }
    rec = s_snapshot;
    rec.tick = BSP_Sampler_GetTick();
    rec.speed_l = BSP_Sampler_GetEncL_1ms();
    rec.speed_r = BSP_Sampler_GetEncR_1ms();
    BSP_Sampler_GetGyroRaw(&raw[0], &raw[1], &raw[2]);
    rec.gyro_x = (int16_t)((float)raw[0] / IMU_GYRO_RAW_TO_DPS);
    rec.gyro_y = (int16_t)((float)raw[1] / IMU_GYRO_RAW_TO_DPS);
    rec.gyro_z = (int16_t)((float)raw[2] / IMU_GYRO_RAW_TO_DPS);
    /* 先完成槽内容，屏障后发布 head；不能把 accepted 或 head 提前。 */
    s_buf[s_head] = rec;
    BSP_Time_MemoryBarrier();
    s_head = next;
    ++s_status.accepted;
    if (s_status.accepted == DATALOG_CAPACITY)
    {
        Datalog_RequestFaultStop(DATALOG_FAULT_FULL, APP_STOP_LOG_FULL, TRACKING_PROTECT_LOG_FULL);
    }
}

void Datalog_Service(void)
{
    uint32_t pending;
    uint32_t n;
    uint16_t tail = s_tail;
    if (s_write_failed || !s_header_valid)
    {
        return;
    }
    pending = ((uint32_t)s_head + DATALOG_BUFFER_RECORDS - tail) % DATALOG_BUFFER_RECORDS;
    if (!pending || (s_status.active && pending < DATALOG_BATCH_RECORDS))
    {
        return;
    }
    n = pending < DATALOG_BATCH_RECORDS ? pending : DATALOG_BATCH_RECORDS;
    BSP_Time_MemoryBarrier();
    for (uint32_t i = 0; i < n; ++i)
    {
        memcpy(s_batch + i * 96, &s_buf[(tail + i) % DATALOG_BUFFER_RECORDS], 96);
    }
    if (BSP_Flash_LogWrite(256 + s_status.written * 96, s_batch, n * 96))
    {
        s_write_failed = 1;
        Datalog_RequestFaultStop(
            DATALOG_FAULT_WRITE, APP_STOP_LOG_WRITE, TRACKING_PROTECT_LOG_WRITE);
        return;
    }
    /* Flash 成功才增加 written；屏障后推进 tail，写错路径不消费也不重试。 */
    s_status.written += n;
    BSP_Time_MemoryBarrier();
    s_tail = (uint16_t)((tail + n) % DATALOG_BUFFER_RECORDS);
}

void Datalog_Flush(void)
{
    Datalog_Service();
}

void Datalog_Stop(void)
{
    s_status.active = 0;
    for (uint16_t i = 0; i < 128 && s_tail != s_head && !s_write_failed; ++i)
    {
        Datalog_Service();
        BSP_WDT_Feed();
    }
}

/**
 * @brief 逐项校验原 v2 头布局、配置、CRC 和标定。
 * @param[in] h 非空已读出的 256 B 头对象。
 * @return 全部条件成立为 1，否则 0。
 * @note 停车导出前台调用，保持短路顺序、校验覆盖 8..255 字节及所有原比较。
 */
static uint8_t Datalog_IsHeaderValid(const datalog_header_t *h)
{
    return h->magic == DATALOG_MAGIC && h->version == 2 && h->header_size == 256
        && h->record_size == 96 && h->sample_period_us == 1000 && h->control_period_us == 2000
        && h->mode < 2 && h->gear < 2 && h->channel_count == 16 && h->noise == 50 && h->lost == 200
        && h->min_contrast == 200 && h->capacity == DATALOG_CAPACITY && h->datalog_div == 1
        && h->crc16 == Datalog_CalculateCrc16((const uint8_t *)h + 8, 248)
        && !Gray_ValidateCalibration(h->black, h->white);
}

void Datalog_Dump(void)
{
    /* 仅停车前台调用，不可重入；大缓冲静态分配，避开AC5的1KB栈。 */
    static datalog_header_t s_dump_header;
    static datalog_record_t s_dump_record;
    static char s_dump_line[640];
    int len;
    Datalog_Stop();
    if (BSP_Flash_LogRead(0, (uint8_t *)&s_dump_header, 256))
    {
        Datalog_LatchFault(DATALOG_FAULT_READ);
        BSP_UART_WriteString("#dump_error=read\r\n");
        goto done;
    }
    if (!s_header_valid || !Datalog_IsHeaderValid(&s_dump_header))
    {
        BSP_UART_WriteString("#dump_error=no_valid_session\r\n");
        goto done;
    }
    (void)snprintf(s_dump_line, sizeof(s_dump_line),
        "#log_version=2,record_size=96,mode=%u,gear=%u\r\n", s_dump_header.mode,
        s_dump_header.gear);
    BSP_UART_WriteString(s_dump_line);
    for (uint8_t color = 0; color < 2; ++color)
    {
        const int16_t *values = color ? s_dump_header.white : s_dump_header.black;
        len = snprintf(s_dump_line, sizeof(s_dump_line), color ? "#white" : "#black");
        for (uint8_t i = 0; i < 16; ++i)
        {
            len += snprintf(s_dump_line + len, sizeof(s_dump_line) - (size_t)len, ",%d", values[i]);
        }
        BSP_UART_WriteString(s_dump_line);
        BSP_UART_WriteString("\r\n");
    }
    BSP_UART_WriteString(
        "tick,adc0,adc1,adc2,adc3,spdL,spdR,err,tout,gx,gy,gz,dl1b,bat,kstate,rstate");
    for (uint8_t i = 0; i < 16; ++i)
    {
        snprintf(s_dump_line, sizeof(s_dump_line), ",gray%u", i);
        BSP_UART_WriteString(s_dump_line);
    }
    for (uint8_t i = 0; i < 16; ++i)
    {
        snprintf(s_dump_line, sizeof(s_dump_line), ",q%u", i);
        BSP_UART_WriteString(s_dump_line);
    }
    BSP_UART_WriteString(
        ",em_err,opt_err,mode,opt_state,gray_valid,protect,reason,gray_seq,control_seq\r\n");
    for (uint32_t i = 0; i < s_status.written; ++i)
    {
        if (BSP_Flash_LogRead(256 + i * 96, (uint8_t *)&s_dump_record, 96))
        {
            Datalog_LatchFault(DATALOG_FAULT_READ);
            BSP_UART_WriteString("#dump_error=read\r\n");
            break;
        }
        len = snprintf(s_dump_line, sizeof(s_dump_line),
            "%lu,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u", (unsigned long)s_dump_record.tick,
            s_dump_record.adc[0], s_dump_record.adc[1], s_dump_record.adc[2], s_dump_record.adc[3],
            s_dump_record.speed_l, s_dump_record.speed_r, s_dump_record.track_error,
            s_dump_record.track_out, s_dump_record.gyro_x, s_dump_record.gyro_y,
            s_dump_record.gyro_z, s_dump_record.dl1b_mm, s_dump_record.battery,
            s_dump_record.kernel_state, s_dump_record.roundabout_state);
        for (uint8_t j = 0; j < 16; ++j)
        {
            len += snprintf(s_dump_line + len, sizeof(s_dump_line) - (size_t)len, ",%u",
                s_dump_record.gray_raw[j]);
        }
        for (uint8_t j = 0; j < 16; ++j)
        {
            len += snprintf(s_dump_line + len, sizeof(s_dump_line) - (size_t)len, ",%u",
                Gray_Normalize(
                    s_dump_record.gray_raw[j], s_dump_header.black[j], s_dump_header.white[j]));
        }
        len += snprintf(s_dump_line + len, sizeof(s_dump_line) - (size_t)len,
            ",%d,%d,%u,%u,%u,%u,%u,%lu,%lu\r\n", s_dump_record.em_error,
            s_dump_record.optical_error, s_dump_record.mode, s_dump_record.optical_state,
            s_dump_record.gray_valid, s_dump_record.protection_flags, s_dump_record.stop_reason,
            (unsigned long)s_dump_record.gray_sequence,
            (unsigned long)s_dump_record.control_sequence);
        if (len > 0 && (size_t)len < sizeof(s_dump_line))
        {
            BSP_UART_Write((const uint8_t *)s_dump_line, (uint16_t)len);
        }
        BSP_WDT_Feed();
    }
done:
    {
        const datalog_status_t *s = Datalog_GetStatus();
        snprintf(s_dump_line, sizeof(s_dump_line),
            "#status,fault=%u,accepted=%lu,written=%lu,rejected=%lu,pending=%lu,reason=%u,protect=%u\r\n",
            s->fault, (unsigned long)s->accepted, (unsigned long)s->written,
            (unsigned long)s->rejected, (unsigned long)s->pending, s->reason, s->protection_flags);
        BSP_UART_WriteString(s_dump_line);
    }
    BSP_UART_WriteString("EOF\r\n");
}
