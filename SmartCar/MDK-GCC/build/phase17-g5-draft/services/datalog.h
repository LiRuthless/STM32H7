/**
 * @file datalog.h
 * @brief App 固定 v2 日志与 CSV；TIM15 生产、TIM6 发布、前台唯一 Flash 消费。
 * @details 声明日志会话、生产、消费、导出及状态接口；完整并发和失败语义分别位于run_record/logger/log_export实现。
 */
#ifndef SMARTCAR_APP_SERVICES_DATALOG_H
#define SMARTCAR_APP_SERVICES_DATALOG_H
#include <stdint.h>
#define DATALOG_RECORD_SIZE 96u
#define DATALOG_HEADER_SIZE 256u
#define DATALOG_CAPACITY 87336u
#define DATALOG_BUFFER_RECORDS 1024u

typedef struct
{
    uint32_t tick;             /**< 1 ms 采样器 tick，uint32_t 自然回绕。 */
    int16_t adc[4];            /**< 四路滤波电感 ADC，横左/竖左/竖右/横右。 */
    int16_t speed_l;           /**< 左轮原始 1 ms 编码器有符号脉冲。 */
    int16_t speed_r;           /**< 右轮原始 1 ms 编码器有符号脉冲。 */
    int16_t track_error;       /**< 所选模式本拍归一化偏差。 */
    int16_t track_out;         /**< 唯一执行口方向输出，正常 ±1000。 */
    int16_t gyro_x;            /**< x 轴原始角速度除以 14.3 后截断，度/秒。 */
    int16_t gyro_y;            /**< y 轴原始角速度除以 14.3 后截断，度/秒。 */
    int16_t gyro_z;            /**< z 轴原始角速度除以 14.3 后截断，度/秒。 */
    uint16_t dl1b_mm;          /**< 激光距离毫米，8192 为无效。 */
    uint16_t battery;          /**< 电池低通后 ADC 值，保留 uint16_t。 */
    uint8_t kernel_state;      /**< 电磁影子主状态编号。 */
    uint8_t roundabout_state;  /**< 电磁影子环岛子状态编号。 */
    uint16_t gray_raw[16];     /**< 十六路完整帧 ADC，ch0 最左。 */
    uint32_t gray_sequence;    /**< 最近完整灰度帧序号。 */
    uint32_t control_sequence; /**< 最近完整控制拍序号。 */
    int16_t em_error;          /**< 电磁观察偏差。 */
    int16_t optical_error;     /**< 光电观察偏差。 */
    uint16_t protection_flags; /**< 累计 TRACKING_PROTECT_* 位图。 */
    uint8_t mode;              /**< 当前模式，0 电磁、1 光电。 */
    uint8_t optical_state;     /**< 光电影子 OPTICAL_* 状态。 */
    uint8_t gray_valid;        /**< 标定/有效线/采集状态共同成立为 1。 */
    uint8_t stop_reason;       /**< 首次 APP_STOP_* 原因字节。 */
    uint8_t reserved[14];      /**< 原十四字节保留区，快照清零，记录仍为 96 B。 */
} datalog_record_t;

typedef struct
{
    float kp;           /**< 记录的方向比例增益。 */
    float k2p;          /**< 记录的方向非线性比例增益。 */
    float kd;           /**< 记录的方向拍间差分增益。 */
    int16_t base_speed; /**< 基础速度，脉冲/2 ms。 */
    uint16_t reserved;  /**< 原两字节保留区，构造头部时清零。 */
} datalog_profile_t;

typedef struct
{
    uint32_t magic;             /**< 原日志魔数，0x324C4353。 */
    uint16_t version;           /**< 固定日志版本 2，不随 C 符号改名升级。 */
    uint16_t crc16;             /**< 从字节 8 开始覆盖 248 B 的原 CRC。 */
    uint16_t header_size;       /**< 头部固定 256 B。 */
    uint16_t record_size;       /**< 记录固定 96 B。 */
    uint16_t sample_period_us;  /**< 采样周期，1000 微秒。 */
    uint16_t control_period_us; /**< 控制周期，2000 微秒。 */
    uint8_t mode;               /**< 会话模式，0 电磁、1 光电。 */
    uint8_t gear;               /**< 会话档位，0 低、1 高。 */
    uint16_t channel_count;     /**< 灰度通道数，16。 */
    uint16_t noise;             /**< 强度噪声阈值，50。 */
    uint16_t lost;              /**< 有效线强度阈值，200。 */
    uint16_t min_contrast;      /**< 黑白 ADC 最小绝对差，200。 */
    uint16_t reserved;          /**< 原两字节保留区，构造时清零。 */
    int16_t black[16];          /**< 十六路黑底实测 ADC，原表快照。 */
    int16_t white[16];          /**< 十六路白线实测 ADC，原表快照。 */
    int8_t weights[16];         /**< 从 15 到 -15 的十六路质心整数权重。 */
    datalog_profile_t em;       /**< 当前档显式电磁 bank 快照。 */
    datalog_profile_t optical;  /**< 当前档显式光电 bank 快照。 */
    float kp_v;                 /**< 共用速度比例增益快照。 */
    float ki_v;                 /**< 共用速度积分增益快照。 */
    int16_t fan_run;            /**< 当前档运行风扇万分比。 */
    int16_t fan_idle;           /**< 当前档停车风扇万分比。 */
    uint32_t gray_budget_us;    /**< 灰度预算，700 微秒。 */
    uint32_t control_budget_us; /**< 控制预算，900 微秒。 */
    uint32_t run_delay_count;   /**< 起跑等待，1000 个 2 ms 控制拍。 */
    uint32_t capacity;          /**< 固定最大记录条数，87336。 */
    uint32_t datalog_div;       /**< 固定记录分频，1；其他配置拒绝起跑。 */
    uint8_t reserved_tail[84];  /**< 原八十四字节尾保留区，构造时清零。 */
} datalog_header_t;

typedef enum
{
    DATALOG_FAULT_NONE = 0,    /**< 尚未锁存日志故障。 */
    DATALOG_FAULT_CONFIG,      /**< 固定记录格式或分频配置不受支持。 */
    DATALOG_FAULT_UNAVAILABLE, /**< 外部 Flash 未就绪或容量不足。 */
    DATALOG_FAULT_HEADER,      /**< 会话头擦除或写入失败。 */
    DATALOG_FAULT_OVERFLOW,    /**< ISR 生产时环形队列已满。 */
    DATALOG_FAULT_WRITE,       /**< 前台消费者写入记录页失败。 */
    DATALOG_FAULT_FULL,        /**< 已写记录达到会话容量上限。 */
    DATALOG_FAULT_READ         /**< 停车回读校验无法读取记录。 */
} datalog_fault_t;

typedef struct
{
    volatile uint8_t active;            /**< 生产使能标志，ISR 读取，前台/故障路径更新。 */
    volatile uint8_t fault;             /**< 首次 DATALOG_FAULT_*，原临界区锁存。 */
    volatile uint8_t reason;            /**< 首次 APP_STOP_*，原临界区锁存。 */
    volatile uint16_t protection_flags; /**< 累计保护位，原临界区按位或。 */
    volatile uint32_t accepted;         /**< 已入队条数，TIM15 单生产者更新。 */
    volatile uint32_t written;          /**< 成功写入条数，前台单消费者更新。 */
    volatile uint32_t rejected;         /**< 队满时拒绝条数，保留首次故障后停止生产。 */
    volatile uint32_t pending;          /**< 状态读取时按 accepted-written 更新，非原子整包快照。 */
} datalog_status_t;

typedef struct
{
    uint8_t valid; /**< 当前头已经写回校验成功，清除后为0。 */
    uint32_t written_records; /**< 仅成功写入的连续前缀条数。 */
    uint8_t header[DATALOG_HEADER_SIZE]; /**< 内存头原字节；无效时全零。 */
} datalog_meta_t;

typedef enum
{
    DATALOG_RESULT_OK, /**< 业务调用成功。 */
    DATALOG_RESULT_INVALID_ARG, /**< 乱序或参数形状无效。 */
    DATALOG_RESULT_OUT_OF_RANGE, /**< 字节范围越界。 */
    DATALOG_RESULT_BUSY, /**< 当前状态不允许。 */
    DATALOG_RESULT_NOT_FOUND, /**< 会话或导出身份不存在。 */
    DATALOG_RESULT_IO_ERROR, /**< 日志读取失败。 */
} datalog_result_t;

typedef struct
{
    uint8_t active; /**< 前台导出互斥是否持有。 */
    uint32_t transfer_id; /**< 非零导出身份，结束后仅作历史监视。 */
    uint32_t record_count; /**< 开始时冻结的成功前缀。 */
    uint32_t total_bytes; /**< 256+96*record_count。 */
    uint32_t next_offset; /**< 下一个尚未交付的绝对字节。 */
    uint32_t crc32; /**< 已按序交付前缀的zlib兼容CRC。 */
} datalog_export_status_t;

/**
 * @brief 清日志队列、摘要及会话有效状态。
 * @note 仅启动前台路径调用，不擦 Flash；保持共享对象写入顺序。 完整契约见 logger.c::Datalog_Init。
 */
void Datalog_Init(void);

/**
 * @brief 检查配置与标定，擦除并写回读校验原日志头。
 * @note 仅起跑门禁前台调用，可能阻塞；先初始化，失败不置 active，保留 256 B 头、CRC 和所有错误路径。 完整契约见 logger.c::Datalog_Start。
 */
uint8_t Datalog_Start(void);

/**
 * @brief 停止入队并擦除日志首扇区、复位懒擦除前沿。
 * @note 仅停车前台调用，可能阻塞；不伪造会话，保留首次故障和上一 RAM 摘要。 完整契约见 logger.c::Datalog_Clear。
 */
uint8_t Datalog_Clear(void);

/**
 * @brief 停止生产并有界刷写尾包。
 * @note 仅前台调用，可能阻塞；最多 128 次服务并喂狗，写失败不重试，允许 active 已为零时收尾。 完整契约见 logger.c::Datalog_Stop。
 */
void Datalog_Stop(void);

/**
 * @brief 将已发布控制快照与本拍采样组成记录并入队。
 * @note 仅 TIM15 单生产者；TIM6 同优先级不互抢，记录写入后屏障再发布 head，故障立即请求停车。 完整契约见 logger.c::Datalog_Push。
 */
void Datalog_Push(void);

/**
 * @brief 最多写入八条记录并成功后发布消费位置。
 * @note 仅前台单消费者，可能阻塞；运行不足八条不写，停车尾包允许不足八条；写失败锁存且禁止重试。 完整契约见 logger.c::Datalog_Service。
 */
void Datalog_Service(void);

/**
 * @brief 保留单次服务包装。
 * @note 前台调用，可阻塞；仍只服务一次，不改为无界清空队列。 完整契约见 logger.c::Datalog_Flush。
 */
void Datalog_Flush(void);

/**
 * @brief 导出原会话头、记录和 57 列 CSV。
 * @note 仅停车前台，阻塞且不可重入；静态缓冲避免占用 AC5 小栈，读错仍输出原错误、摘要及 EOF。 完整契约见 log_export.c::Datalog_Dump。
 */
void Datalog_Dump(void);

/**
 * @brief 按原读取位置发布完整控制快照。
 * @note TIM6 或起跑前调用；TIM15 同优先级不交错，先清保留区再写全部字段，结尾内存屏障保持。 完整契约见 run_record.c::Datalog_PublishControl。
 */
void Datalog_PublishControl(void);

/**
 * @brief 锁存首次停车原因并累加保护位。
 * @note ISR/前台可调用，在既有 PRIMASK 临界区更新，不改变首次原因。 完整契约见 logger.c::Datalog_SetStopSummary。
 */
void Datalog_SetStopSummary(uint8_t reason, uint16_t flags);

/**
 * @brief 更新 pending 计数并返回状态。
 * @note 指向模块持有状态，寿命整个程序；不是原子整包快照，前台/ISR 按既有调用约定访问。 完整契约见 logger.c::Datalog_GetStatus。
 */
const datalog_status_t *Datalog_GetStatus(void);

/**
 * @brief 读取生产使能标志。
 * @note 无阻塞，不代表 Flash 服务已完成。 完整契约见 logger.c::Datalog_IsActive。
 */
uint8_t Datalog_IsActive(void);

/**
 * @brief 读取已成功写入条数。
 * @note 无阻塞，不包含待刷写队列。 完整契约见 logger.c::Datalog_GetCount。
 */
uint32_t Datalog_GetCount(void);

/**
 * @brief 复制内存日志元信息，无Flash访问。
 * @note 完整契约见logger.c::Datalog_GetMeta。
 */
void Datalog_GetMeta(datalog_meta_t *meta);
/**
 * @brief 停车冻结成功前缀并取得导出互斥。
 * @note 完整契约见log_export.c::Datalog_BeginExport。
 */
datalog_result_t Datalog_BeginExport(datalog_export_status_t *status);
/**
 * @brief 顺序读取或重取最近原始日志块。
 * @note 成功借出内部缓存，下次新读取可覆盖；同次前台调用内完成复制。
 *     完整契约见log_export.c::Datalog_ReadExport。
 */
datalog_result_t Datalog_ReadExport(uint32_t id, uint32_t offset, uint32_t length,
    const uint8_t **data, uint16_t *data_length);
/**
 * @brief 全部交付后返回总CRC并释放。
 * @note 完整契约见log_export.c::Datalog_EndExport。
 */
datalog_result_t Datalog_EndExport(uint32_t id, uint32_t *total, uint32_t *crc);
/**
 * @brief 释放导出互斥，保留原日志和控制状态。
 * @note 完整契约见log_export.c::Datalog_CancelExport。
 */
void Datalog_CancelExport(void);
/**
 * @brief 服务无进展租期超时。
 * @note 完整契约见log_export.c::Datalog_ServiceExport。
 */
void Datalog_ServiceExport(void);
/**
 * @brief 更新超时后读取互斥标志。
 * @note 完整契约见log_export.c::Datalog_IsExporting。
 */
uint8_t Datalog_IsExporting(void);
/**
 * @brief 复制导出监视状态。
 * @note 完整契约见log_export.c::Datalog_GetExportStatus。
 */
void Datalog_GetExportStatus(datalog_export_status_t *status);
#endif
