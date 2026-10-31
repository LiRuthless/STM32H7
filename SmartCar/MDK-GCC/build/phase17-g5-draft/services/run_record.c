/**
 * @file run_record.c
 * @brief 固定日志头和完整控制记录组装。
 * @details TIM6发布唯一96B快照，TIM15在入队门禁后复制并补采样字段。
 *     前台构造唯一256B会话头；布局、CRC16和标定校验在本模块集中。
 */
#include "services/run_record.h"
#include "services/logger.h"
#include "bsp.h"
#include "app_config.h"
#include "sensing/battery.h"
#include "system/lifecycle.h"
#include "control/tracking.h"
#include "sensing/track_sensor.h"
#include "sensing/gray_sensor.h"
#include "control/optical_fsm.h"
#include "control/electromagnetic_fsm.h"
#include "services/parameter_bank.h"
#include "system/parameter_application.h"
#include "control/pid.h"
#include "services/telemetry.h"
#include <stddef.h>
#include <string.h>

#define DATALOG_MAGIC 0x324C4353u
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

/* TIM6发布、TIM15复制，同级中断不交错。 */
static datalog_record_t s_snapshot;
/* 前台唯一当前会话头；logger和导出只借用其地址。 */
static datalog_header_t s_header;

/**
 * @brief 按原 CCITT 位序计算日志校验。
 * @details 从0xffff开始逐字节异或高8位，再按最高位执行八次左移与0x1021反馈；
 *     每步截断至16位，输入只读，不修改会话或队列。
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
 * @brief 将方向 bank 写入头部并清保留字。
 * @details 依次复制kp、k2p、kd、base_speed并将reserved清零；不做范围校验、
 *     钳位或活动控制配置覆写，目标仅为日志头内的持久化方向配置。
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
 * @brief 清空完整控制快照。
 * @note 仅日志初始化调用；不清会话头，也不触碰日志队列或上传状态。
 */
void RunRecord_ResetSnapshot(void)
{
    memset(&s_snapshot, 0, sizeof(s_snapshot));
}

/**
 * @brief 检查当前灰度标定能否建立日志会话。
 * @return 标定完整且对比度满足原规则为1，否则0。
 * @note 起跑前台调用，只转发Gray_CheckCalibration(0)，不输出诊断文本。
 */
uint8_t RunRecord_CheckCalibration(void)
{
    return Gray_CheckCalibration(0);
}

/**
 * @brief 组装本次会话的固定256字节头。
 * @details 清零头部后依次复制模式/档位、标定、两套方向bank和活动速度PI。
 *     固定预算、周期及容量写入后计算字节8..255的CRC16，保留区保持零。
 * @return 唯一静态会话头的只读地址；下次构造会覆盖内容。
 * @note 仅起跑前台在配置和标定门禁通过后调用；无Flash访问。
 */
const datalog_header_t *RunRecord_BuildHeader(void)
{
    const tracking_state_t *t = Tracking_GetState();
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
    return &s_header;
}

/**
 * @brief 借用当前内存会话头。
 * @return 同一静态256字节头的只读地址；有效性由logger单独维护。
 * @note 读取不扫描Flash、不构造头；调用方不得写入或保存另一份当前会话状态。
 */
const datalog_header_t *RunRecord_GetHeader(void)
{
    return &s_header;
}

/**
 * @brief 按原读取位置发布完整控制快照。
 * @details 先取各模块只读状态并清96B快照，再复制电感/灰度、控制输出、距离/电压、
 *     双FSM与序号、保护及停止摘要；灰度有效要求标定/线有效且无采集错，屏障后可供TIM15取用。
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
    s_snapshot.stop_reason = Logger_GetStopReason();
    BSP_Time_MemoryBarrier();
}

/**
 * @brief 把完整控制快照与当前1ms采样合为记录。
 * @details 先复制快照，再依次读tick、左右编码器和三轴陀螺原始值。
 *     三轴逐个除14.3并截断到int16_t，完整记录交给独立上传队列后返回。
 * @param[out] record 非空96字节目标，logger门禁全部通过后才写入。
 * @note 仅TIM15单生产者；TIM6同级不抢占，不发布Flash队列head或计数。
 */
void RunRecord_CaptureSample(datalog_record_t *record)
{
    int16_t raw[3];
    *record = s_snapshot;
    record->tick = BSP_Sampler_GetTick();
    record->speed_l = BSP_Sampler_GetEncL_1ms();
    record->speed_r = BSP_Sampler_GetEncR_1ms();
    BSP_Sampler_GetGyroRaw(&raw[0], &raw[1], &raw[2]);
    record->gyro_x = (int16_t)((float)raw[0] / IMU_GYRO_RAW_TO_DPS);
    record->gyro_y = (int16_t)((float)raw[1] / IMU_GYRO_RAW_TO_DPS);
    record->gyro_z = (int16_t)((float)raw[2] / IMU_GYRO_RAW_TO_DPS);
    /* 上传复制完整记录，使用独立索引；拥堵不影响Flash消费位置或故障停车。 */
    Telemetry_PushRecord(record);
}

/**
 * @brief 逐项校验原 v2 头布局、配置、CRC 和标定。
 * @details 依次短路比较魔数、版本、长度、周期、模式/档位、通道、阈值、容量与分频，
 *     再验证8..255字节CRC和头内黑白标定；预算、方向参数和保留区不在这些条件内。
 * @param[in] h 非空已读出的 256 B 头对象。
 * @return 全部条件成立为 1，否则 0。
 * @note 停车导出前台调用，保持短路顺序、校验覆盖 8..255 字节及所有原比较。
 */
uint8_t RunRecord_IsHeaderValid(const datalog_header_t *h)
{
    return h->magic == DATALOG_MAGIC && h->version == 2 && h->header_size == 256
        && h->record_size == 96 && h->sample_period_us == 1000 && h->control_period_us == 2000
        && h->mode < 2 && h->gear < 2 && h->channel_count == 16 && h->noise == 50 && h->lost == 200
        && h->min_contrast == 200 && h->capacity == DATALOG_CAPACITY && h->datalog_div == 1
        && h->crc16 == Datalog_CalculateCrc16((const uint8_t *)h + 8, 248)
        && !Gray_ValidateCalibration(h->black, h->white);
}
