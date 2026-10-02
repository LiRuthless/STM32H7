// ============================================================
// 文件名: datalog.c
// 功能说明: 运行数据记录模块
// TIM15 采样器 1ms 每拍采集一条 32B 记录入 RAM 环形缓冲（App_SampleISR
// 调用 Datalog_Push），TIM7(5ms) 刷入 W25Q64 日志区（8MB 减末尾
// 4KB 参数扇区，跨入新扇区时擦除）。停车后串口 CSV 导出。
// 容量 262016 条 @1ms ≈ 262s（DATALOG_DIV 可降频延长）。
// ============================================================

#include "bsp.h"
#include "app_config.h"
#include "datalog.h"
#include "app.h"
#include "track_sensor.h"
#include "imu_proc.h"
#include "control.h"
#include "motor.h"
#include "roundabout.h"

#include <stdio.h>
#include <string.h>

/* 私有定义 -----------------------------------------------------------*/
#define LOG_BUF_RECORDS     16u     /* RAM 缓冲条数（2 个 256B 页；掉电最多丢失这么多条） */
#define LOG_PAGE_RECORDS    8u      /* 每次刷写 8 条 = 256B = 1 个 W25Q 页 */

/* 私有变量 ---------------------------------------------------------*/
static datalog_record_t s_buf[LOG_BUF_RECORDS];
static volatile uint8_t s_head;         /* 写指针（TIM15 采样器生产） */
static volatile uint8_t s_tail;         /* 读指针（TIM7 消费） */
static uint8_t  s_active;               /* 记录中标志 */
static uint32_t s_flash_off;            /* 日志扇区写入偏移（字节） */
static uint32_t s_count;                /* 已写记录条数 */
#if DATALOG_DIV > 1
static uint8_t  s_div_cnt;              /* DATALOG_DIV 分频计数 */
#endif

/* 导出函数定义 -------------------------------------------*/

void Datalog_Init(void)
{
    s_head = 0;
    s_tail = 0;
    s_active = 0;
    s_flash_off = 0;
    s_count = 0;
#if DATALOG_DIV > 1
    s_div_cnt = 0;
#endif
}

// 函数名: Datalog_Start
// 功能: 擦除首个 4KB 日志扇区并启动记录（典型几十 ms，仅停车时调用）
uint8_t Datalog_Start(void)
{
    Datalog_Init();
    if(BSP_Flash_LogErase() != 0)
    {
        return 1;
    }
    s_active = 1;
    return 0;
}

void Datalog_Stop(void)
{
    s_active = 0;
    Datalog_Flush();                /* 刷出缓冲中不足一页的剩余记录 */
}

uint8_t Datalog_IsActive(void)
{
    return s_active;
}

uint32_t Datalog_GetCount(void)
{
    return s_count;
}

// 函数名: Datalog_Push
// 功能: 采集一条记录入 RAM 缓冲（TIM15 采样器 1ms 调用，DATALOG_DIV>1 时分频）
// 说明: 生产者（TIM15 优先级 0）只写 s_head，消费者（TIM7）只写 s_tail，
//       单生产单消费无锁安全。缓冲满时丢弃新记录。
//       编码器/陀螺取采样器 1ms 实时值；电感/偏差等控制量按 2ms 节拍更新。
void Datalog_Push(void)
{
    datalog_record_t *rec;
    int16_t gyro_raw[3];
    uint8_t next;

    if(!s_active)
    {
        return;
    }
#if DATALOG_DIV > 1
    if(++s_div_cnt < DATALOG_DIV)       /* 分频：每 N 拍记一条 */
    {
        return;
    }
    s_div_cnt = 0;
#endif
    if(s_count >= DATALOG_CAPACITY)     /* 扇区写满自动停止 */
    {
        s_active = 0;
        return;
    }

    next = (uint8_t)((s_head + 1u) % LOG_BUF_RECORDS);
    if(next == s_tail)                  /* 缓冲满：丢弃本条（等待刷写赶上） */
    {
        return;
    }

    BSP_Sampler_GetGyroRaw(&gyro_raw[0], &gyro_raw[1], &gyro_raw[2]);

    rec = &s_buf[s_head];
    rec->tick             = BSP_Sampler_GetTick();          /* 1ms 采样节拍 */
    rec->adc[0]           = adc_filted[0];
    rec->adc[1]           = adc_filted[1];
    rec->adc[2]           = adc_filted[2];
    rec->adc[3]           = adc_filted[3];
    rec->speed_l          = BSP_Sampler_GetEncL_1ms();      /* 1ms 编码器计数 */
    rec->speed_r          = BSP_Sampler_GetEncR_1ms();
    rec->track_error      = track_error;
    rec->track_out        = track_out;
    rec->gyro_x           = (int16_t)((float)gyro_raw[0] / GYRO_RAW_TO_DPS);  /* °/s 取整 */
    rec->gyro_y           = (int16_t)((float)gyro_raw[1] / GYRO_RAW_TO_DPS);
    rec->gyro_z           = (int16_t)((float)gyro_raw[2] / GYRO_RAW_TO_DPS);
    rec->dl1b_mm          = dl1b_distance_mm;
    rec->battery          = (uint16_t)battery_filt;
    rec->kernel_state     = kernel_state;
    rec->roundabout_state = roundabout_state;

    s_head = next;
}

// 函数名: Datalog_Flush
// 功能: 把 RAM 缓冲中的记录追加写入 W25Q64（TIM7 调用）
// 说明: 攒满 8 条（256B=1 页）整页编程一次（约 0.7ms）；不足 8 条时
//       由 Datalog_Stop/Datalog_Dump 触发兜底刷写（len 为 32 的倍数即可）。
void Datalog_Flush(void)
{
    static uint8_t batch[W25Q64_PAGE_SIZE];     /* 256B 页拼装缓冲 */
    uint32_t pending;
    uint32_t n;
    uint32_t i;

    for(;;)
    {
        pending = ((uint32_t)s_head + LOG_BUF_RECORDS - s_tail) % LOG_BUF_RECORDS;
        if(pending == 0)
        {
            return;
        }
        /* 运行中（s_active）只整页刷写，避免频繁小额页编程占用 TIM7 */
        if(s_active && pending < LOG_PAGE_RECORDS)
        {
            return;
        }
        n = (pending < LOG_PAGE_RECORDS) ? pending : LOG_PAGE_RECORDS;
        for(i = 0; i < n; i++)                  /* 环形缓冲拼装成连续页 */
        {
            memcpy(&batch[i * DATALOG_RECORD_SIZE],
                   &s_buf[(s_tail + i) % LOG_BUF_RECORDS], DATALOG_RECORD_SIZE);
        }
        if(BSP_Flash_LogWrite(s_flash_off, batch, n * DATALOG_RECORD_SIZE) != 0)
        {
            s_active = 0;               /* 写失败：停止记录，保住已有数据 */
            return;
        }
        s_flash_off += n * DATALOG_RECORD_SIZE;
        s_count    += n;
        s_tail      = (uint8_t)((s_tail + n) % LOG_BUF_RECORDS);
    }
}

// 函数名: Datalog_Dump
// 功能: 串口 CSV 导出全部记录（阻塞，仅停车时调用）
void Datalog_Dump(void)
{
    datalog_record_t rec;
    char line[96];
    uint32_t i;
    int len;

    Datalog_Stop();                 /* 停止记录并刷出缓冲中剩余记录 */

    BSP_UART_WriteString("tick,adc0,adc1,adc2,adc3,spdL,spdR,err,tout,gx,gy,gz,dl1b,bat,kstate,rstate\r\n");
    for(i = 0; i < s_count; i++)
    {
        if(BSP_Flash_LogRead(i * DATALOG_RECORD_SIZE, (uint8_t *)&rec,
                             DATALOG_RECORD_SIZE) != 0)
        {
            break;
        }
        len = snprintf(line, sizeof(line), "%lu,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u\r\n",
                       (unsigned long)rec.tick,
                       (int)rec.adc[0], (int)rec.adc[1], (int)rec.adc[2], (int)rec.adc[3],
                       (int)rec.speed_l, (int)rec.speed_r,
                       (int)rec.track_error, (int)rec.track_out,
                       (int)rec.gyro_x, (int)rec.gyro_y, (int)rec.gyro_z,
                       (unsigned int)rec.dl1b_mm, (unsigned int)rec.battery,
                       (unsigned int)rec.kernel_state, (unsigned int)rec.roundabout_state);
        if(len > 0)
        {
            BSP_UART_Write((const uint8_t *)line, (uint16_t)len);
        }
    }
    BSP_UART_WriteString("EOF\r\n");
}
