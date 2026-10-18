/**
 * @file telemetry.c
 * @brief 与Flash独立的256槽SPSC上传支路。
 * @details TIM15完成96B记录后复制槽，屏障发布head；前台只推进tail并每10ms封包。
 *     槽携带发生时的index/run_id/revision；拥堵仅丢上传，不访问Flash、不请求停车。
 */
#include "telemetry.h"
#include "scb_link.h"
#include "bsp.h"
#include <string.h>

typedef struct
{
    datalog_record_t record; /**< 原96B固定记录，包含零保留区。 */
    uint32_t index; /**< 采样索引，TIM15写入。 */
    uint32_t run_id; /**< 采样时运行编号。 */
    uint32_t revision; /**< 采样时有效参数版本。 */
} telemetry_slot_t;

typedef char telemetry_slot_size_assert_t[(sizeof(telemetry_slot_t) == 108) ? 1 : -1];

/* TIM15写槽/head/index/生产丢弃；前台只写tail/消费丢弃，初始化除外。 */
static telemetry_slot_t s_slots[TELEMETRY_CAPACITY];
static volatile uint32_t s_head;
static volatile uint32_t s_tail;
static volatile uint32_t s_next_index;
static volatile uint32_t s_producer_dropped;
static uint32_t s_consumer_dropped;
/* run/revision在同级TIM6/TIM15间一致；BeginRun在定时链停止时写run。 */
static uint32_t s_run_id;
static volatile uint32_t s_revision;
static uint32_t s_last_packet;

/**
 * @brief 初始化上传支路。
 * @details 清所有索引和会话元数据，不读取或修改Flash日志。
 * @note 仅上电前台调用一次，生产者尚未启动。
 */
void Telemetry_Init(void)
{
    s_head = 0;
    s_tail = 0;
    s_next_index = 0;
    s_producer_dropped = 0;
    s_consumer_dropped = 0;
    s_run_id = 0;
    s_revision = 0;
    s_last_packet = HAL_GetTick();
}

/**
 * @brief 丢弃旧会话尚未发送的上传记录。
 * @details 固定head后只推进tail，差值计入消费者丢弃；不修改ISR拥有的head或记录索引。
 * @note 前台调用，可被生产者打断；边界以本次head快照为准。
 */
void Telemetry_ResetSession(void)
{
    uint32_t head = s_head;
    BSP_Time_MemoryBarrier();
    s_consumer_dropped += head - s_tail;
    s_tail = head;
}

/**
 * @brief 在实际起跑前分配运行身份。
 * @details 保留revision和累计丢弃，清上次运行积压并重置索引；run_id回绕前重建会话。
 * @pre 全部起跑门禁通过，TIM15/TIM6尚未启动。
 * @note 前台调用，不能用通信重连冒充新起跑。
 */
void Telemetry_BeginRun(void)
{
    if (s_run_id == 0xffffffffu)
    {
        SCB_ResetSession(HAL_GetTick());
        s_run_id = 0;
    }
    Telemetry_ResetSession();
    ++s_run_id;
    s_next_index = 0;
    s_last_packet = HAL_GetTick();
}

/**
 * @brief 复制一条记录及同一时点的元数据。
 * @details 索引先递增，即使队满也保留缺口；全256槽可用，写完内容后DMB再发布head。
 * @param[in] record 非空完整96B记录，仅本次调用借用。
 * @note 仅TIM15单生产者，TIM6同级不互抢；无CRC、JSON、等待或Flash访问。
 */
void Telemetry_PushRecord(const datalog_record_t *record)
{
    uint32_t head = s_head;
    uint32_t index = s_next_index++;
    telemetry_slot_t *slot;
    if (head - s_tail >= TELEMETRY_CAPACITY)
    {
        ++s_producer_dropped;
        return;
    }
    slot = &s_slots[head & (TELEMETRY_CAPACITY - 1u)];
    slot->record = *record;
    slot->index = index;
    slot->run_id = s_run_id;
    slot->revision = s_revision;
    BSP_Time_MemoryBarrier();
    s_head = head + 1u;
}

/**
 * @brief 读取上传统计的一致副本。
 * @details 极短临界区固定生产者元数据，不把消费者丢弃与生产丢弃放在共同写入计数器。
 * @param[out] status 非空调用方对象。
 * @note 前台或同级ISR可用，不输出内部指针。
 */
void Telemetry_GetStatus(telemetry_status_t *status)
{
    uint32_t mask = BSP_Time_EnterCritical();
    status->run_id = s_run_id;
    status->revision = s_revision;
    status->next_index = s_next_index;
    status->dropped = s_producer_dropped + s_consumer_dropped;
    status->queued = s_head - s_tail;
    BSP_Time_ExitCritical(mask);
}

/**
 * @brief 发布后续采样使用的有效参数版本。
 * @param[in] revision 完整调参拍发布的版本。
 * @note TIM6或停车前台调用，已入队槽不重标版本。
 */
void Telemetry_SetRevision(uint32_t revision)
{
    s_revision = revision;
}

/**
 * @brief 封装一批连续、同运行同版本的记录。
 * @details 每10ms最多10条，遇索引缺口或run/revision变化提前结束；先固定head并DMB，
 *     复制完成才释放tail，生产者不能覆写仍在读取的槽。仅前台组元数据和负载。
 * @param[out] payload 非空至少984B目标。
 * @param[out] length 非空实际负载长度，仅成功时更新。
 * @param[out] count 非空条数1～10，仅成功时更新。
 * @param[in] now 单调毫秒，无符号差允许回绕。
 * @return 有上传包1，未到10ms或无记录0。
 * @note 调用方已获得READY且可靠结果/事件优先处理；无整份日志副本。
 */
uint8_t Telemetry_Service(uint8_t *payload, uint16_t *length, uint16_t *count, uint32_t now)
{
    uint32_t head;
    uint32_t tail = s_tail;
    telemetry_slot_t *first;
    uint16_t records = 0;
    uint8_t boundary = 0;
    head = s_head;
    BSP_Time_MemoryBarrier();
    while (tail != head && s_slots[tail & (TELEMETRY_CAPACITY - 1u)].run_id != s_run_id)
    {
        ++tail;
        ++s_consumer_dropped;
    }
    s_tail = tail;
    if (tail == head)
    {
        return 0;
    }
    first = &s_slots[tail & (TELEMETRY_CAPACITY - 1u)];
    /* 先只查看本批身份；版本/索引变化允许在10ms之前结束旧批，未到期的连续批不消费。 */
    while (tail + records != head && records < 10)
    {
        const telemetry_slot_t *slot = &s_slots[(tail + records) & (TELEMETRY_CAPACITY - 1u)];
        if (slot->run_id != first->run_id || slot->revision != first->revision
            || slot->index != first->index + records)
        {
            boundary = 1;
            break;
        }
        ++records;
    }
    if (!boundary && (uint32_t)(now - s_last_packet) < 10u)
    {
        return 0;
    }
    SCB_WriteU32(payload, first->index);
    SCB_WriteU32(payload + 4, first->revision);
    SCB_WriteU32(payload + 8, s_producer_dropped + s_consumer_dropped);
    SCB_WriteU32(payload + 12, 0);
    SCB_WriteU32(payload + 16, first->record.tick);
    SCB_WriteU32(payload + 20, first->run_id);
    for (uint16_t i = 0; i < records; ++i)
    {
        const telemetry_slot_t *slot = &s_slots[(tail + i) & (TELEMETRY_CAPACITY - 1u)];
        memcpy(payload + 24u + i * DATALOG_RECORD_SIZE, &slot->record, DATALOG_RECORD_SIZE);
    }
    BSP_Time_MemoryBarrier();
    s_tail = tail + records;
    s_last_packet = now;
    *count = records;
    *length = 24u + records * DATALOG_RECORD_SIZE;
    return 1;
}
