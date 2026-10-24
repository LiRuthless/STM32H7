/**
 * @file logger.c
 * @brief 日志会话和Flash单生产者单消费者队列。
 * @details TIM15门禁后生产1024槽环形缓冲，前台每次至多写八条。
 *     会话有效位、首故障、首因及统计均在此持有；写失败后不重试、不提前释放槽。
 */
#include "services/logger.h"
#include "services/run_record.h"
#include "services/log_store.h"
#include "services/log_export.h"
#include "bsp.h"
#include "app.h"
#include "app_config.h"
#include "control/tracking.h"
#include <string.h>

#define DATALOG_BATCH_RECORDS 8u
/* TIM15 单生产者写槽和 head，前台单消费者写 tail/batch；屏障位置不能移动。 */
static datalog_record_t s_buf[DATALOG_BUFFER_RECORDS];
/* 前台批写缓冲及环形索引；head 由 TIM15 写，tail 与状态统计由前台写。 */
static uint8_t s_batch[DATALOG_BATCH_RECORDS * DATALOG_RECORD_SIZE];
static volatile uint16_t s_head;
static volatile uint16_t s_tail;
static datalog_status_t s_status;
/* 前台建立会话成功后置位，清除操作不伪造新会话。 */
static uint8_t s_header_valid;
/* 首次写失败后锁存，active 清零也不能触发重试。 */
static uint8_t s_write_failed;

/**
 * @brief 锁存首次故障并按原规则停生产。
 * @details 在短临界区仅当fault尚为零时保存故障；READ以外的故障均清active，
 *     即使已有首故障也执行停生产，最后恢复调用前的中断屏蔽状态。
 * @param[in] fault DATALOG_FAULT_* 字节值；READ不清active；任何其他传入值均清active。
 * @note ISR/前台可调用，只在原短临界区更新，不进行 Flash I/O。
 */
void Logger_LatchFault(uint8_t fault)
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
 * @details 先锁存日志故障并停止生产，再累计控制保护位，最后请求安全停车；
 *     首次原因由各所属模块保留，本函数不等待前台刷写收尾。
 * @param[in] fault 原 DATALOG_FAULT_* 故障值。
 * @param[in] reason 原 APP_STOP_* 停车原因。
 * @param[in] flag 原 TRACKING_PROTECT_* 对应位。
 * @note ISR/前台可调用，有界；不将重复停车原因覆盖首次原因。
 */
static void Datalog_RequestFaultStop(uint8_t fault, app_stop_reason_t reason, uint16_t flag)
{
    Logger_LatchFault(fault);
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
 * @details 每次查询先以accepted减written刷新pending，再返回同一状态对象；
 *     不读Flash、不移动队列索引，也不为调用方建立独立副本。
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
 * @details 依次复位head/tail、清日志统计和控制快照，再撤销头有效与写失败锁存；
 *     槽内容无需清零，导出身份、块缓存和租期不在此初始化。
 * @note 仅启动前台路径调用，不擦 Flash；保持共享对象写入顺序。
 */
void Datalog_Init(void)
{
    s_head = s_tail = 0;
    memset(&s_status, 0, sizeof(s_status));
    RunRecord_ResetSnapshot();
    s_header_valid = 0;
    s_write_failed = 0;
}

/**
 * @brief 检查配置与标定，擦除并写回读校验原日志头。
 * @details 导出互斥有效时直接失败；其余路径先清会话，再检查编译配置和灰度标定。
 *     头部记录当前模式/档位、显式方向bank、活动轮速PI和标定，清保留区后计算CRC；
 *     擦除、写头、回读及逐字节比较全部成功才置头有效和active，失败锁存对应故障。
 * @return 成功 0；配置、标定、擦写或回读失败为 1 并锁存原故障。
 * @note 仅起跑门禁前台调用，可能阻塞；先初始化，失败不置 active，保留 256 B 头、CRC 和所有错误路径。
 */
uint8_t Datalog_Start(void)
{
    /*
     * 先保留原tracking读取时点并检查导出互斥，再复位会话/队列；配置和标定通过后，
     * 才构造头并擦写、回读校验。全部成功后发布头有效和active。
     */
    const tracking_state_t *t = Tracking_GetState();
    const datalog_header_t *header;
    (void)t;
    if (Datalog_IsExporting())
    {
        return 1;
    }
    Datalog_Init();
    if (!DATALOG_ENABLE || DATALOG_DIV != 1)
    {
        Logger_LatchFault(DATALOG_FAULT_CONFIG);
        return 1;
    }
    if (!RunRecord_CheckCalibration())
    {
        Logger_LatchFault(DATALOG_FAULT_HEADER);
        return 1;
    }
    header = RunRecord_BuildHeader();
    if (LogStore_Erase())
    {
        Logger_LatchFault(DATALOG_FAULT_UNAVAILABLE);
        return 1;
    }
    if (LogStore_WriteHeader(header))
    {
        Logger_LatchFault(DATALOG_FAULT_HEADER);
        return 1;
    }
    s_header_valid = 1;
    s_status.active = 1;
    return 0;
}

/**
 * @brief 停止入队并擦除日志首扇区、复位懒擦除前沿。
 * @details 导出仍有效时失败且不修改会话；否则先清active和头有效，再请求BSP擦除。
 *     擦除失败不恢复这两个标志，既有统计、快照及首故障保留。
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
    return LogStore_Erase();
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
        memcpy(meta->header, RunRecord_GetHeader(), DATALOG_HEADER_SIZE);
    }
    else
    {
        memset(meta->header, 0, DATALOG_HEADER_SIZE);
    }
    BSP_Time_ExitCritical(mask);
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
    RunRecord_CaptureSample(&rec);
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
    if (LogStore_Write(256 + s_status.written * 96, s_batch, n * 96))
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
 * @details 先清active禁止后续生产，再在队列非空且未锁存写失败时服务至多128批；
 *     每批服务后喂狗，允许不足八条的尾包，写错后保留未消费槽且不再重试。
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
 * @brief 读取当前会话首次停车原因。
 * @return 对应原状态字节，不清状态也不更新pending。
 * @note 只执行一次标量读取，不代表整个会话原子快照。
 */
uint8_t Logger_GetStopReason(void)
{
    return s_status.reason;
}

/**
 * @brief 读取当前会话头已写回校验标志。
 * @return 对应原状态字节，不清状态也不更新pending。
 * @note 只执行一次标量读取，不代表整个会话原子快照。
 */
uint8_t Logger_HasHeader(void)
{
    return s_header_valid;
}
