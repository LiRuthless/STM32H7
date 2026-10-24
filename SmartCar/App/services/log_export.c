/**
 * @file log_export.c
 * @brief 停车日志原始字节导出与CSV输出。
 * @details 前台冻结成功落盘前缀，使用唯一968B块缓存与10秒无进展租期。
 *     本模块返回独立业务结果，不编码通信头；CSV保持静态缓冲及原文本。
 */
#include "services/log_export.h"
#include "services/logger.h"
#include "services/run_record.h"
#include "services/log_store.h"
#include "services/checksum.h"
#include "sensing/gray_sensor.h"
#include "app.h"
#include "bsp.h"
#include <stdio.h>
#include <string.h>

/* 前台导出独占状态；窗口1，块仅在成功新读取时替换，重复不推进CRC或租期。 */
static datalog_export_status_t s_export;
static uint32_t s_export_progress_ms;
static uint32_t s_export_last_offset;
static uint16_t s_export_last_length;
static uint8_t s_export_block[968];

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
 * @note 前台调用，无Flash读写；ID递增并跳过0，会话取消后的旧传输缓存由通信适配层负责。
 */
datalog_result_t Datalog_BeginExport(datalog_export_status_t *status)
{
    if (!App_IsStopped() || Datalog_IsActive() || Datalog_IsExporting())
    {
        return DATALOG_RESULT_BUSY;
    }
    if (!Logger_HasHeader())
    {
        return DATALOG_RESULT_NOT_FOUND;
    }
    ++s_export.transfer_id;
    if (!s_export.transfer_id)
    {
        ++s_export.transfer_id;
    }
    s_export.record_count = Datalog_GetCount();
    s_export.total_bytes = DATALOG_HEADER_SIZE + DATALOG_RECORD_SIZE * Datalog_GetCount();
    s_export.next_offset = s_export.crc32 = 0;
    s_export_last_length = 0;
    s_export_progress_ms = BSP_Time_GetTick();
    s_export.active = 1;
    *status = s_export;
    return DATALOG_RESULT_OK;
}

/**
 * @brief 顺序读取原始日志字节或借用最近成功块。
 * @details 先检查租期、身份和减法边界；最近同offset/length块直接借用缓存。
 *     新块只允许next_offset，读前后喂狗，成功才推进CRC/offset/租期。
 *     读错锁READ并取消导出；失败不发布data，只先把data_length置零。
 * @param[in] id 当前非零导出身份。
 * @param[in] offset 日志绝对字节偏移。
 * @param[in] length 请求1..960字节。
 * @param[out] data 非空指针槽，成功借出唯一缓存+8处原始字节，失败不写。
 * @param[out] data_length 非空长度槽，先置0，成功写原始字节数。
 * @return OK成功；失效NOT_FOUND；边界OUT_OF_RANGE；乱序INVALID_ARG；读错IO_ERROR。
 * @note 仅前台串行；借用在下次新块读取覆盖缓存前有效，调用方须同次前台调用内复制。
 */
datalog_result_t Datalog_ReadExport(uint32_t id, uint32_t offset, uint32_t length,
    const uint8_t **data, uint16_t *data_length)
{
    *data_length = 0;
    if (!Datalog_IsExporting() || id != s_export.transfer_id)
    {
        return DATALOG_RESULT_NOT_FOUND;
    }
    if (!length || length > 960u || offset > s_export.total_bytes
        || length > s_export.total_bytes - offset)
    {
        return DATALOG_RESULT_OUT_OF_RANGE;
    }
    if (s_export_last_length && offset == s_export_last_offset
        && length == s_export_last_length)
    {
        *data = s_export_block + 8;
        *data_length = (uint16_t)length;
        return DATALOG_RESULT_OK;
    }
    if (offset != s_export.next_offset)
    {
        return DATALOG_RESULT_INVALID_ARG;
    }
    BSP_WDT_Feed();
    if (LogStore_Read(offset, s_export_block + 8, length))
    {
        BSP_WDT_Feed();
        Logger_LatchFault(DATALOG_FAULT_READ);
        Datalog_CancelExport();
        return DATALOG_RESULT_IO_ERROR;
    }
    BSP_WDT_Feed();
    s_export.crc32 = Checksum_UpdateCrc32(s_export.crc32, s_export_block + 8, length);
    s_export.next_offset = offset + length;
    s_export_last_offset = offset;
    s_export_last_length = (uint16_t)length;
    s_export_progress_ms = BSP_Time_GetTick();
    *data = s_export_block + 8;
    *data_length = (uint16_t)length;
    return DATALOG_RESULT_OK;
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
datalog_result_t Datalog_EndExport(uint32_t id, uint32_t *total, uint32_t *crc)
{
    if (!Datalog_IsExporting() || id != s_export.transfer_id)
    {
        return DATALOG_RESULT_NOT_FOUND;
    }
    if (s_export.next_offset != s_export.total_bytes)
    {
        return DATALOG_RESULT_BUSY;
    }
    *total = s_export.total_bytes;
    *crc = s_export.crc32;
    Datalog_CancelExport();
    return DATALOG_RESULT_OK;
}

/**
 * @brief 导出原会话头、记录和 57 列 CSV。
 * @details 先停止并刷尾包，读取并验证头后输出元数据、标定和固定列名；仅遍历written前缀，
 *     灰度归一化使用头内标定，每条有效文本发送后喂狗。头读错、头无效或记录读错均进入
 *     原错误输出路径，末尾仍发送RAM会话摘要及EOF，不扫描未确认记录。
 * @note 仅停车前台，阻塞且不可重入；静态大缓冲避免占用运行栈，读错仍输出原错误、摘要及 EOF。
 */
void Datalog_Dump(void)
{
    /* 仅停车前台调用，不可重入；大缓冲静态分配，避免占用运行栈。 */
    static datalog_header_t s_dump_header;
    static datalog_record_t s_dump_record;
    static char s_dump_line[640];
    int len;
    Datalog_Stop();
    if (LogStore_Read(0, (uint8_t *)&s_dump_header, 256))
    {
        Logger_LatchFault(DATALOG_FAULT_READ);
        BSP_UART_WriteString("#dump_error=read\r\n");
        goto done;
    }
    if (!Logger_HasHeader() || !RunRecord_IsHeaderValid(&s_dump_header))
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
    for (uint32_t i = 0; i < Datalog_GetCount(); ++i)
    {
        if (LogStore_Read(256 + i * 96, (uint8_t *)&s_dump_record, 96))
        {
            Logger_LatchFault(DATALOG_FAULT_READ);
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
