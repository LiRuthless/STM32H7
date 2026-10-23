/**
 * @file datalog.c
 * @brief App 固定 v2 日志与 CSV；TIM15 生产、TIM6 发布、前台唯一 Flash 消费。
 * @details 由 TIM15 生产固定记录、TIM6 发布控制快照、主循环批量消费至 Flash，并提供停车收尾与 CSV 导出。
 */
/* Phase14 日志v2：固定little-endian布局，单生产者/单消费者；Flash仅前台访问。
 * TIM6/TIM15同优先级，控制快照发布不交错；其他共享标志用极短PRIMASK锁存。 */
#include "bsp.h"
#include "app_config.h"
#include "services/datalog.h"
#include "app.h"
#include "sensing/battery.h"
#include "system/lifecycle.h"
#include "control/control.h"
#include "sensing/track_sensor.h"
#include "sensing/gray_sensor.h"
#include "control/optical_fsm.h"
#include "control/electromagnetic_fsm.h"
#include "services/param.h"
#include "control/pid.h"
#include "services/telemetry.h"
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
DATALOG_ASSERT_LAYOUT(datalog_offset_tick_assert_t,
    offsetof(datalog_record_t, tick) == 0);
DATALOG_ASSERT_LAYOUT(datalog_offset_adc_assert_t,
    offsetof(datalog_record_t, adc) == 4);
DATALOG_ASSERT_LAYOUT(datalog_offset_speed_l_assert_t,
    offsetof(datalog_record_t, speed_l) == 12);
DATALOG_ASSERT_LAYOUT(datalog_offset_speed_r_assert_t,
    offsetof(datalog_record_t, speed_r) == 14);
DATALOG_ASSERT_LAYOUT(datalog_offset_track_error_assert_t,
    offsetof(datalog_record_t, track_error) == 16);
DATALOG_ASSERT_LAYOUT(datalog_offset_track_out_assert_t,
    offsetof(datalog_record_t, track_out) == 18);
DATALOG_ASSERT_LAYOUT(datalog_offset_gyro_x_assert_t,
    offsetof(datalog_record_t, gyro_x) == 20);
DATALOG_ASSERT_LAYOUT(datalog_offset_gyro_y_assert_t,
    offsetof(datalog_record_t, gyro_y) == 22);
DATALOG_ASSERT_LAYOUT(datalog_offset_gyro_z_assert_t,
    offsetof(datalog_record_t, gyro_z) == 24);
DATALOG_ASSERT_LAYOUT(datalog_offset_dl1b_mm_assert_t,
    offsetof(datalog_record_t, dl1b_mm) == 26);
DATALOG_ASSERT_LAYOUT(datalog_offset_battery_assert_t,
    offsetof(datalog_record_t, battery) == 28);
DATALOG_ASSERT_LAYOUT(datalog_offset_kernel_state_assert_t,
    offsetof(datalog_record_t, kernel_state) == 30);
DATALOG_ASSERT_LAYOUT(datalog_offset_roundabout_state_assert_t,
    offsetof(datalog_record_t, roundabout_state) == 31);
DATALOG_ASSERT_LAYOUT(datalog_offset_gray_raw_assert_t,
    offsetof(datalog_record_t, gray_raw) == 32);
DATALOG_ASSERT_LAYOUT(datalog_offset_gray_sequence_assert_t,
    offsetof(datalog_record_t, gray_sequence) == 64);
DATALOG_ASSERT_LAYOUT(datalog_offset_control_sequence_assert_t,
    offsetof(datalog_record_t, control_sequence) == 68);
DATALOG_ASSERT_LAYOUT(datalog_offset_em_error_assert_t,
    offsetof(datalog_record_t, em_error) == 72);
DATALOG_ASSERT_LAYOUT(datalog_offset_optical_error_assert_t,
    offsetof(datalog_record_t, optical_error) == 74);
DATALOG_ASSERT_LAYOUT(datalog_offset_protection_flags_assert_t,
    offsetof(datalog_record_t, protection_flags) == 76);
DATALOG_ASSERT_LAYOUT(datalog_offset_mode_assert_t,
    offsetof(datalog_record_t, mode) == 78);
DATALOG_ASSERT_LAYOUT(datalog_offset_optical_state_assert_t,
    offsetof(datalog_record_t, optical_state) == 79);
DATALOG_ASSERT_LAYOUT(datalog_offset_gray_valid_assert_t,
    offsetof(datalog_record_t, gray_valid) == 80);
DATALOG_ASSERT_LAYOUT(datalog_offset_stop_reason_assert_t,
    offsetof(datalog_record_t, stop_reason) == 81);
DATALOG_ASSERT_LAYOUT(datalog_offset_reserved_assert_t,
    offsetof(datalog_record_t, reserved) == 82);
/* TIM15 单生产者写槽和 head，前台单消费者写 tail/batch；屏障位置不能移动。 */
static datalog_record_t s_buf[DATALOG_BUFFER_RECORDS];
/* TIM6 发布、TIM15 取用的完整控制快照，同级中断不交错。 */
static datalog_record_t s_snapshot;
/* 前台持有会话头，原 256 B；所有保留区清零后计算 CRC。 */
static datalog_header_t s_header;
/* 前台批写缓冲及环形索引；head 由 TIM15 写，tail 与状态统计由前台写。 */
static uint8_t s_batch[DATALOG_BATCH_RECORDS * DATALOG_RECORD_SIZE];
static volatile uint16_t s_head;
static volatile uint16_t s_tail;
static datalog_status_t s_status;
/* 前台建立会话成功后置位，清除操作不伪造新会话。 */
static uint8_t s_header_valid;
/* 首次写失败后锁存，active 清零也不能触发重试。 */
static uint8_t s_write_failed;
/* 前台导出独占状态；窗口1，块仅在成功新读取时替换，重复不推进CRC或租期。 */
static datalog_export_status_t s_export;
static uint32_t s_export_progress_ms;
static uint32_t s_export_last_offset;
static uint16_t s_export_last_length;
static uint8_t s_export_block[968];

static uint16_t Datalog_CalculateCrc16(const uint8_t *data, uint32_t len);
static void Datalog_LatchFault(uint8_t fault);
static void Datalog_RequestFaultStop(uint8_t fault, app_stop_reason_t reason, uint16_t flag);
static void Datalog_SetProfile(datalog_profile_t *dst, const param_direction_t *src);
static uint8_t Datalog_IsHeaderValid(const datalog_header_t *h);

/**
 * @brief 按原 CCITT 位序计算日志校验。
 * @details 按函数体既定循环顺序累计或更新数据，并在循环结束后发布结果；循环边界和整数运算顺序保持不变。
 * @param[in] data 非空数组，至少 len 字节。
 * @param[in] len 覆盖长度，可为 0。
 * @return 原 16 位校验，空长度返回初值。
 * @note 初值 0xffff，多项式 0x1021，不改原常量类型或转换。
 */
static uint16_t Datalog_CalculateCrc16(const uint8_t *data, uint32_t len)
{
    /* 逐字节把输入并入 CRC 高字节，每字节再按最高位执行八次 CCITT 移位；len 为零时直接保留初值。 */
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
 * @details 按现有调用顺序使用 BSP_Time_EnterCritical、BSP_Time_ExitCritical 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] fault DATALOG_FAULT_* 字节值；READ 不清 active，其他值按原逻辑清零。
 * @note ISR/前台可调用，只在原短临界区更新，不进行 Flash I/O。
 */
static void Datalog_LatchFault(uint8_t fault)
{
    /* 在短临界区内只锁存首次故障；除读取故障外同时停止生产，退出时恢复调用前的 PRIMASK。 */
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
 * @details 按现有调用顺序使用 Datalog_LatchFault、Tracking_AddProtection、App_RequestStop 完成本函数处理。
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

/**
 * @brief 锁存首次停车原因并累加保护位。
 * @details 在短 PRIMASK 临界区内仅于当前原因尚为零时写入首次停车原因，同时按位或累加保护标志；重复调用不会覆盖首因。
 * @param[in] reason 停车原因字节；仅首次非零原因被保存。
 * @param[in] flags TRACKING_PROTECT_* 位图；与既有摘要按位或。
 * @note ISR/前台可调用，在既有 PRIMASK 临界区更新，不改变首次原因。
 */
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

/**
 * @brief 更新 pending 计数并返回状态。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @return 非空只读指针；pending=accepted-written。
 * @note 指向模块持有状态，寿命整个程序；不是原子整包快照，前台/ISR 按既有调用约定访问。
 */
const datalog_status_t *Datalog_GetStatus(void)
{
    s_status.pending = s_status.accepted - s_status.written;
    return &s_status;
}

/**
 * @brief 读取生产使能标志。
 * @return 原 active 字节，0 停止，非零允许生产。
 * @note 无阻塞，不代表 Flash 服务已完成。
 */
uint8_t Datalog_IsActive(void)
{
    return s_status.active;
}

/**
 * @brief 读取已成功写入条数。
 * @return written 计数，正常不超过 87336。
 * @note 无阻塞，不包含待刷写队列。
 */
uint32_t Datalog_GetCount(void)
{
    return s_status.written;
}

/**
 * @brief 清日志队列、摘要及会话有效状态。
 * @details 按现有调用顺序使用 memset 完成本函数处理。
 * @note 仅启动前台路径调用，不擦 Flash；保持共享对象写入顺序。
 */
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
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
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

/**
 * @brief 检查配置与标定，擦除并写回读校验原日志头。
 * @details 按现有调用顺序使用 Tracking_GetState、Datalog_Init、Datalog_LatchFault、Gray_CheckCalibration
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @return 成功 0；配置、标定、擦写或回读失败为 1 并锁存原故障。
 * @note 仅起跑门禁前台调用，可能阻塞；先初始化，失败不置 active，保留 256 B 头、CRC 和所有错误路径。
 */
uint8_t Datalog_Start(void)
{
    /* 先清会话与队列状态并检查 Flash/标定，再填充固定 256 字节头及 CRC；擦除和写头全部成功后才发布有效会话和 active。 */
    static datalog_header_t s_check;
    const tracking_state_t *t = Tracking_GetState();
    if (Datalog_IsExporting())
    {
        return 1;
    }
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

/**
 * @brief 停止入队并擦除日志首扇区、复位懒擦除前沿。
 * @details 按现有调用顺序使用 BSP_Flash_LogErase 完成本函数处理。
 * @return 原 BSP 擦除返回值，0 成功，非零失败。
 * @note 仅停车前台调用，可能阻塞；不伪造会话，保留首次故障和上一 RAM 摘要。
 */
uint8_t Datalog_Clear(void)
{
    if (Datalog_IsExporting())
    {
        return 1;
    }
    s_status.active = 0;
    s_header_valid = 0;
    /* 擦除不伪造新会话；首次故障和上一会话RAM摘要保留到下次Start。 */
    return BSP_Flash_LogErase();
}

/**
 * @brief 复制已确认日志头及落盘计数，不访问Flash。
 * @details 头只由前台建立/清除，written也由前台发布；短临界区复制避免查询分散读取。
 * @param[out] meta 非空输出；无效会话头全零，written保留原统计。
 * @note 前台查询调用，不扫描上电残留，不把未成功批次计入前缀。
 */
void Datalog_GetMeta(datalog_meta_t *meta)
{
    uint32_t mask = BSP_Time_EnterCritical();
    meta->valid = s_header_valid;
    meta->written_records = s_status.written;
    if (s_header_valid)
    {
        memcpy(meta->header, &s_header, DATALOG_HEADER_SIZE);
    }
    else
    {
        memset(meta->header, 0, DATALOG_HEADER_SIZE);
    }
    BSP_Time_ExitCritical(mask);
}

/**
 * @brief 取消导出并释放起跑/清日志互斥。
 * @details 只清active和最近块许可，原日志、调试和故障统计保持。
 * @note 前台会话变化、读错、超时或正常END调用；无Flash副作用。
 */
void Datalog_CancelExport(void)
{
    s_export.active = 0;
    s_export_last_length = 0;
}

/**
 * @brief 无新成功读取满10秒时释放导出。
 * @details 无符号毫秒差覆盖tick回绕，普通查询及重复块不修改进展起点。
 * @note 前台反复服务，不等待、不产生车辆停车。
 */
void Datalog_ServiceExport(void)
{
    if (s_export.active && (uint32_t)(BSP_Time_GetTick() - s_export_progress_ms) >= 10000u)
    {
        Datalog_CancelExport();
    }
}

/**
 * @brief 先处理超时再读取导出互斥。
 * @details 到期会清active及最近块许可，未到期只读取前台状态。
 * @return 当前仍持互斥1，否则0。
 * @note 所有前台起跑/清日志入口使用同一判定，不改变原故障状态。
 */
uint8_t Datalog_IsExporting(void)
{
    Datalog_ServiceExport();
    return s_export.active;
}

/**
 * @brief 复制前台导出状态。
 * @details 复制前统一服务租期，失效历史字段保留但active为零，不能继续读取。
 * @param[out] status 非空副本，不转移内部所有权。
 * @note 前台串行调用，先服务租期，完成后历史字段仅供监视。
 */
void Datalog_GetExportStatus(datalog_export_status_t *status)
{
    Datalog_ServiceExport();
    *status = s_export;
}

/**
 * @brief 在停车收尾返回后冻结已确认前缀。
 * @details 先隔离运行/待收尾和已有导出，再检查有效会话；失败不修改原日志或导出身份。
 *     写失败可有未写尾包，用户允许只导出written前缀，不重试、不补造数据。
 * @param[out] status 非空输出，成功时写完整冻结状态，失败保持调用方对象。
 * @return OK成功；运行/已有导出BUSY；无有效会话NOT_FOUND。
 * @note 前台调用，无Flash读写；ID递增并跳过0，会话取消旧传输缓存由SCB负责。
 */
scb_status_t Datalog_BeginExport(datalog_export_status_t *status)
{
    if (!App_IsStopped() || s_status.active || Datalog_IsExporting())
    {
        return SCB_BUSY;
    }
    if (!s_header_valid)
    {
        return SCB_NOT_FOUND;
    }
    ++s_export.transfer_id;
    if (!s_export.transfer_id)
    {
        ++s_export.transfer_id;
    }
    s_export.record_count = s_status.written;
    s_export.total_bytes = DATALOG_HEADER_SIZE + DATALOG_RECORD_SIZE * s_status.written;
    s_export.next_offset = s_export.crc32 = 0;
    s_export_last_length = 0;
    s_export_progress_ms = BSP_Time_GetTick();
    s_export.active = 1;
    *status = s_export;
    return SCB_OK;
}

/**
 * @brief 顺序读取原Flash字节并产生LOG_DATA，重复最近块不续租。
 * @details 先用减法检查边界避免offset+length溢出；只有新成功块才推进CRC和租期。
 *     Flash读前后喂狗；读错立即取消，保留旧首故障，不改变车辆起停或调参。
 * @param[in] id 当前非零导出身份。
 * @param[in] offset 原日志绝对字节偏移。
 * @param[in] length 1～960字节。
 * @param[out] payload 非空且至少968B容量，成功写8B头及length字节。
 * @param[out] payload_length 非空，先置0，成功写8+length。
 * @return OK成功；会话失效NOT_FOUND；边界OUT_OF_RANGE；乱序INVALID_ARG；读错IO_ERROR。
 * @note 仅前台串行；大块静态，不持整份日志RAM，不压缩原tick时间轴。
 */
scb_status_t Datalog_ReadExport(uint32_t id, uint32_t offset, uint32_t length,
    uint8_t *payload, uint16_t *payload_length)
{
    *payload_length = 0;
    if (!Datalog_IsExporting() || id != s_export.transfer_id)
    {
        return SCB_NOT_FOUND;
    }
    if (!length || length > 960u || offset > s_export.total_bytes
        || length > s_export.total_bytes - offset)
    {
        return SCB_OUT_OF_RANGE;
    }
    if (s_export_last_length && offset == s_export_last_offset
        && length == s_export_last_length)
    {
        memcpy(payload, s_export_block, length + 8u);
        *payload_length = (uint16_t)(length + 8u);
        return SCB_OK;
    }
    if (offset != s_export.next_offset)
    {
        return SCB_INVALID_ARG;
    }
    BSP_WDT_Feed();
    if (BSP_Flash_LogRead(offset, s_export_block + 8, length))
    {
        BSP_WDT_Feed();
        Datalog_LatchFault(DATALOG_FAULT_READ);
        Datalog_CancelExport();
        return SCB_IO_ERROR;
    }
    BSP_WDT_Feed();
    SCB_WriteU32(s_export_block, offset);
    SCB_WriteU16(s_export_block + 4, (uint16_t)length);
    SCB_WriteU16(s_export_block + 6, 0);
    s_export.crc32 = SCB_CalculateCrc(s_export.crc32, s_export_block + 8, length);
    s_export.next_offset = offset + length;
    s_export_last_offset = offset;
    s_export_last_length = (uint16_t)length;
    s_export_progress_ms = BSP_Time_GetTick();
    memcpy(payload, s_export_block, length + 8u);
    *payload_length = (uint16_t)(length + 8u);
    return SCB_OK;
}

/**
 * @brief 完整按序交付后返回总长及CRC并解锁。
 * @details 提前结束保留会话让对端继续读取；成功输出冻结总长和累计CRC后释放互斥。
 * @param[in] id 当前导出身份。
 * @param[out] total 非空，成功写总字节数。
 * @param[out] crc 非空，成功写整体zlib兼容CRC32。
 * @return OK成功；失效NOT_FOUND；尚未交付全长BUSY。
 * @note 仅前台，无Flash读取；失败输出对象保持。
 */
scb_status_t Datalog_EndExport(uint32_t id, uint32_t *total, uint32_t *crc)
{
    if (!Datalog_IsExporting() || id != s_export.transfer_id)
    {
        return SCB_NOT_FOUND;
    }
    if (s_export.next_offset != s_export.total_bytes)
    {
        return SCB_BUSY;
    }
    *total = s_export.total_bytes;
    *crc = s_export.crc32;
    Datalog_CancelExport();
    return SCB_OK;
}

/**
 * @brief 按原读取位置发布完整控制快照。
 * @details 按现有调用顺序使用 Track_GetState、Gray_GetState、Tracking_GetState、memset 完成本函数处理。
 * @note TIM6 或起跑前调用；TIM15 同优先级不交错，先清保留区再写全部字段，结尾内存屏障保持。
 */
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
    s_snapshot.dl1b_mm = Lifecycle_GetDistanceMm();
    s_snapshot.battery = (uint16_t)Battery_GetFiltered();
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

/**
 * @brief 将已发布控制快照与本拍采样组成记录并入队。
 * @details 先检查原Flash会话/容量/环形队列门禁，再把最近控制快照与本拍采样组成96B记录。
 *     完整记录复制到独立上传队列后，按原屏障顺序发布Flash槽和accepted计数；上传队满只
 *     记录上传丢弃，绝不修改Flash消费位置。Flash原故障仍立即请求安全停车。
 * @note 仅 TIM15 单生产者；TIM6 同优先级不互抢，记录写入后屏障再发布 head，故障立即请求停车。
 */
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
    /* 上传复制完整记录，使用独立索引；拥堵不影响Flash消费位置或故障停车。 */
    Telemetry_PushRecord(&rec);
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

/**
 * @brief 将环形队列中的至多八条记录写入当前日志会话。
 * @details 前台作为唯一消费者，先固定 tail，检查会话有效标志和写失败锁存，再计算待写数量；
 *     记录仍启用时不足八条暂不写，停止生产后允许短尾包。复制记录期间和 Flash 阻塞期间
 *     都不推进 tail，保证 ISR 生产者不会复用本批槽；成功后才发布消费完成。
 * @pre 由主循环单消费者路径调用，不与另一个消费者或日志初始化/导出并发；ISR 不调用。
 * @note 无显式形参和返回值；读取 s_head、s_tail、s_buf、会话有效/生产/故障状态，
 *     写 s_batch、s_status.written 和 s_tail。Flash 可阻塞，并可被定时中断打断。
 *     写失败时置 s_write_failed，锁存故障并请求安全停车，不消费本批且后续不重试；
 *     不保证已发出的 Flash 写入可回滚，也不把计数与 tail 声称为原子整包更新。
 */
void Datalog_Service(void)
{
    uint32_t pending;
    uint32_t n;
    /* 固定本批的消费起点；Flash 完成前不释放这些槽，生产者只能继续使用其他空槽。 */
    uint16_t tail = s_tail;

    /* 没有有效会话或此前写失败时不再发起写入，避免无头记录和失败重试。 */
    if (s_write_failed || !s_header_valid)
    {
        return;
    }
    /* 运行中累计到八条再写，降低服务频度；停止生产后允许不足八条的尾包。 */
    pending = ((uint32_t)s_head + DATALOG_BUFFER_RECORDS - tail) % DATALOG_BUFFER_RECORDS;
    if (!pending || (s_status.active && pending < DATALOG_BATCH_RECORDS))
    {
        return;
    }
    n = pending < DATALOG_BATCH_RECORDS ? pending : DATALOG_BATCH_RECORDS;
    BSP_Time_MemoryBarrier();
    /* 复制为连续批次，一次最多八条；不移动 tail，因此被中断时本批记录仍被占用。 */
    for (uint32_t i = 0; i < n; ++i)
    {
        memcpy(s_batch + i * 96, &s_buf[(tail + i) % DATALOG_BUFFER_RECORDS], 96);
    }
    /* 会话头占前 256 字节，每条 96 字节；页/扇区拆写由 BSP 完成，失败锁存并请求停车。 */
    if (BSP_Flash_LogWrite(256 + s_status.written * 96, s_batch, n * 96))
    {
        s_write_failed = 1;
        Datalog_RequestFaultStop(
            DATALOG_FAULT_WRITE, APP_STOP_LOG_WRITE, TRACKING_PROTECT_LOG_WRITE);
        return;
    }
    /* 写成功才累计落盘数，并在屏障后发布 tail；失败可能已部分写入，不能承诺 Flash 回滚。 */
    s_status.written += n;
    BSP_Time_MemoryBarrier();
    s_tail = (uint16_t)((tail + n) % DATALOG_BUFFER_RECORDS);
}

/**
 * @brief 保留单次服务包装。
 * @note 前台调用，可阻塞；仍只服务一次，不改为无界清空队列。
 */
void Datalog_Flush(void)
{
    Datalog_Service();
}

/**
 * @brief 停止生产并有界刷写尾包。
 * @details 按现有调用顺序使用 Datalog_Service、BSP_WDT_Feed 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 仅前台调用，可能阻塞；最多 128 次服务并喂狗，写失败不重试，允许 active 已为零时收尾。
 */
void Datalog_Stop(void)
{
    /* 先禁止 ISR 继续生产，再以 128 次为上限服务剩余尾包并喂狗；队列清空或写失败时提前结束。 */
    s_status.active = 0;
    for (uint16_t i = 0; i < 128 && s_tail != s_head && !s_write_failed; ++i)
    {
        Datalog_Service();
        BSP_WDT_Feed();
    }
}

/**
 * @brief 逐项校验原 v2 头布局、配置、CRC 和标定。
 * @details 按现有调用顺序使用 Datalog_CalculateCrc16、Gray_ValidateCalibration 完成本函数处理。
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

/**
 * @brief 导出原会话头、记录和 57 列 CSV。
 * @details 按现有调用顺序使用 Datalog_Stop、BSP_Flash_LogRead、Datalog_LatchFault、BSP_UART_WriteString
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 仅停车前台，阻塞且不可重入；静态缓冲避免占用 AC5 小栈，读错仍输出原错误、摘要及 EOF。
 */
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
