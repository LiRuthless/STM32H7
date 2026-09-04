#ifndef __DATALOG_H
#define __DATALOG_H

#include "main.h"
#include "bsp_flash.h"

/* 运行数据记录：TIM15 采样器 1ms 每拍采集一条 32B 记录，RAM 缓冲后顺序追加到
 * Flash 日志区（BSP_Flash_Log*，Bank2 Sector5+6 共 256KB）。
 * 容量 8192 条，1ms 周期下约 8.2s（DATALOG_DIV 可降频延长）。
 * 流程：Datalog_Start(停车时,含整扇区擦除) → TIM15 每拍 Datalog_Push
 *       → TIM7 Datalog_Flush 刷入 Flash → 停车后 Datalog_Dump 串口导出。 */

/* 单条记录：32 字节 = 1 个 Flash Word */
typedef struct
{
    uint32_t tick;              /* 采样节拍计数（1ms） */
    int16_t  adc[4];            /* 四路电感滤波值（12bit，2ms 节拍更新） */
    int16_t  speed_l;           /* 左轮 1ms 编码器计数 */
    int16_t  speed_r;           /* 右轮 1ms 编码器计数 */
    int16_t  track_error;       /* 循迹偏差（2ms 节拍更新） */
    int16_t  track_out;         /* 方向环输出（2ms 节拍更新） */
    int16_t  gyro_x;            /* °/s 取整（1ms 实时） */
    int16_t  gyro_y;
    int16_t  gyro_z;
    uint16_t dl1b_mm;           /* 激光距离 mm（8192=无效） */
    uint16_t battery;           /* 电池电压滤波值（12bit） */
    uint8_t  kernel_state;      /* 主状态机 */
    uint8_t  roundabout_state;  /* 环岛子状态机 */
} datalog_record_t;             /* 共 32 字节 */

#define DATALOG_RECORD_SIZE     32u
#define DATALOG_CAPACITY        (BSP_FLASH_LOG_SIZE / DATALOG_RECORD_SIZE)  /* 4096 条 */

void     Datalog_Init(void);
uint8_t  Datalog_Start(void);       /* 擦除日志扇区并从头记录（阻塞约 2s，仅停车时调用）0=成功 */
void     Datalog_Stop(void);
void     Datalog_Push(void);        /* TIM6 控制环每拍调用（受 DATALOG_DIV 分频） */
void     Datalog_Flush(void);       /* TIM7 辅助环调用：把 RAM 缓冲刷入 Flash */
void     Datalog_Dump(void);        /* 串口 CSV 导出（阻塞，仅停车时调用） */
uint8_t  Datalog_IsActive(void);
uint32_t Datalog_GetCount(void);    /* 已写入 Flash 的记录条数 */

#endif /* __DATALOG_H */
