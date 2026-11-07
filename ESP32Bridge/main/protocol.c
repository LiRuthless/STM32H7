/**
 * @file protocol.c
 * @brief SCB显式帧编解码。
 * @details 输入字段或完整字节，先校验头/CRC/类型负载，再发布编码或借用视图。
 *     SPI/NET各自任务提供工作区；无共享传输状态、不操作ISR/DMA/socket或业务ACK。
 *
 * @note Phase02 G4：完整入口scb_encode校验新负载；scb_encode_validated复用内部负载验证结果。
 */
#include "protocol.h"
#include <float.h>
#include <limits.h>

enum
{
    MAGIC = 0, /**< magic起始字节偏移。 */
    VERSION = 4, /**< 版本字节偏移。 */
    TYPE = 5, /**< 消息类型字节偏移。 */
    FLAGS = 6, /**< flags起始字节偏移。 */
    PAYLOAD_LEN = 8, /**< 负载长度起始字节偏移。 */
    RECORD_COUNT = 10, /**< 记录数起始字节偏移。 */
    SESSION = 12, /**< 会话起始字节偏移。 */
    SEQ = 16, /**< 序号起始字节偏移。 */
    ACK = 20, /**< 确认序号起始字节偏移。 */
    REQUEST = 24, /**< 请求ID起始字节偏移。 */
    CRC = 28 /**< CRC起始字节偏移。 */
};

_Static_assert(
    CHAR_BIT == 8 && sizeof(uint16_t) == 2 && sizeof(uint32_t) == 4, "SCB integer widths");
_Static_assert(SCB_HEADER_BYTES == 32 && CRC + 4 == SCB_HEADER_BYTES, "SCB header size");
_Static_assert(VERSION == MAGIC + 4 && TYPE == VERSION + 1 && FLAGS == TYPE + 1
        && PAYLOAD_LEN == FLAGS + 2 && RECORD_COUNT == PAYLOAD_LEN + 2
        && SESSION == RECORD_COUNT + 2 && SEQ == SESSION + 4 && ACK == SEQ + 4 && REQUEST == ACK + 4
        && CRC == REQUEST + 4,
    "SCB all header offsets");
_Static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128,
    "SCB binary32 requirement");

/* 仅作编译期布局见证，禁止按此类型强转接收字节或裸发结构体。 */
typedef struct
{
    uint32_t tick; /**< 记录tick字段布局见证，单位由公共记录契约定义。 */
    int16_t adc[4]; /**< 四路ADC字段布局见证，不在桥接端解释物理量。 */
    int16_t speed_l; /**< 左速度原始字段布局见证。 */
    int16_t speed_r; /**< 右速度原始字段布局见证。 */
    int16_t track_error; /**< 跟踪误差原始字段布局见证。 */
    int16_t track_out; /**< 跟踪输出原始字段布局见证。 */
    int16_t gyro_x; /**< X陀螺原始字段布局见证。 */
    int16_t gyro_y; /**< Y陀螺原始字段布局见证。 */
    int16_t gyro_z; /**< Z陀螺原始字段布局见证。 */
    uint16_t dl1b_mm; /**< 距离毫米字段布局见证。 */
    uint16_t battery; /**< 电池原始字段布局见证，单位由公共契约定义。 */
    uint8_t kernel_state; /**< 内核状态字节布局见证。 */
    uint8_t roundabout_state; /**< 环岛状态字节布局见证。 */
    uint16_t gray_raw[16]; /**< 16路灰度原始字段布局见证。 */
    uint32_t gray_sequence; /**< 灰度序列字段布局见证。 */
    uint32_t control_sequence; /**< 控制序列字段布局见证。 */
    int16_t em_error; /**< 电磁误差原始字段布局见证。 */
    int16_t optical_error; /**< 光学误差原始字段布局见证。 */
    uint16_t protection_flags; /**< 保护标志字段布局见证。 */
    uint8_t mode; /**< 模式字节布局见证。 */
    uint8_t optical_state; /**< 光学状态字节布局见证。 */
    uint8_t gray_valid; /**< 灰度有效标志布局见证。 */
    uint8_t stop_reason; /**< 停车原因字节布局见证，不在桥接端决策。 */
    uint8_t reserved[14]; /**< 14字节保留区布局见证，桥接不解释其内容。 */
} record_layout_witness_t;

#define OFFSET(field, value)                                                                       \
    _Static_assert(offsetof(record_layout_witness_t, field) == value, #field " offset")
OFFSET(tick, 0);
OFFSET(adc, 4);
OFFSET(speed_l, 12);
OFFSET(speed_r, 14);
OFFSET(track_error, 16);
OFFSET(track_out, 18);
OFFSET(gyro_x, 20);
OFFSET(gyro_y, 22);
OFFSET(gyro_z, 24);
OFFSET(dl1b_mm, 26);
OFFSET(battery, 28);
OFFSET(kernel_state, 30);
OFFSET(roundabout_state, 31);
OFFSET(gray_raw, 32);
OFFSET(gray_sequence, 64);
OFFSET(control_sequence, 68);
OFFSET(em_error, 72);
OFFSET(optical_error, 74);
OFFSET(protection_flags, 76);
OFFSET(mode, 78);
OFFSET(optical_state, 79);
OFFSET(gray_valid, 80);
OFFSET(stop_reason, 81);
OFFSET(reserved, 82);
_Static_assert(sizeof(record_layout_witness_t) == SCB_RECORD_BYTES, "96-byte record");
_Static_assert(SCB_TELEMETRY_PREFIX + 10 * SCB_RECORD_BYTES + SCB_HEADER_BYTES == 1016,
    "UDP maximum telemetry frame");

/**
 * @brief 读取小端16位字段。
 * @details 逐字节组合，不要求输入地址按整数对齐。
 * @param[in] b 非空且至少两字节可读，调用期间借用。
 * @return 解码后的16位无符号值。
 */
static uint16_t get16(const uint8_t *b)
{
    return (uint16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
}

/**
 * @brief 读取小端32位字段。
 * @details 逐字节组合，不强转线数据为结构体。
 * @param[in] b 非空且至少四字节可读，调用期间借用。
 * @return 解码后的32位无符号值。
 */
static uint32_t get32(const uint8_t *b)
{
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

/**
 * @brief 写入小端16位字段。
 * @details 按低字节到高字节写入，不改变其他字节。
 * @param[out] b 非空且至少两字节可写。
 * @param[in] x 待编码字段值。
 */
static void put16(uint8_t *b, uint16_t x)
{
    b[0] = (uint8_t)x;
    b[1] = (uint8_t)(x >> 8);
}

/**
 * @brief 写入小端32位字段。
 * @details 依序写四个字节，不要求地址整数对齐。
 * @param[out] b 非空且至少四字节可写。
 * @param[in] x 待编码字段值。
 */
static void put32(uint8_t *b, uint32_t x)
{
    for (unsigned i = 0; i < 4u; ++i)
    {
        b[i] = (uint8_t)(x >> (i * 8u));
    }
}

/**
 * @brief 累计SCB的逐位CRC状态。
 * @details 对每字节执行八步反射多项式更新；本函数不做最终取反。
 * @param[in] crc 尚未最终取反的累计状态。
 * @param[in] b n非零时须有n字节可读，调用期间借用。
 * @param[in] n 字节数。
 * @return 更新后的未取反CRC状态。
 */
static uint32_t crc_update(uint32_t crc, const uint8_t *b, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        crc ^= b[i];
        for (unsigned bit = 0; bit < 8u; ++bit)
        {
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
        }
    }
    return crc;
}

/**
 * @brief 计算头前28字节与实际负载的CRC。
 * @details 从UINT32_MAX起算，跳过头内CRC及SPI填充，最后取反。
 * @param[in] b 非空完整头及payload_len字节负载，调用期间借用。
 * @param[in] payload_len 实际负载字节数，调用方已校验长度。
 * @return 线格式使用的CRC值。
 */
static uint32_t frame_crc(const uint8_t *b, size_t payload_len)
{
    return ~crc_update(crc_update(UINT32_MAX, b, CRC), b + SCB_HEADER_BYTES, payload_len);
}

/**
 * @brief 判断消息类型是否使用JSON负载。
 * @details 只覆盖HELLO、HELLO_ACK、COMMAND、RESULT和EVENT。
 * @param[in] type 待检查类型字节。
 * @return 属于JSON类型返回true，否则false。
 */
static bool json_type(uint8_t type)
{
    return type == SCB_HELLO || type == SCB_HELLO_ACK || type == SCB_COMMAND || type == SCB_RESULT
        || type == SCB_EVENT;
}

/**
 * @brief 验证字段值与传输类型的组合。
 * @details 按传输、长度、类型、flags、记录数和IDLE空负载顺序检查，首错返回；不解释车辆业务。
 * @param[in] h 非空头字段值，只读借用。
 * @param[in] t SPI、TCP或UDP；其他值拒绝。
 * @return 合法SCB_OK，否则返回首个格式错误。
 */
static scb_error_t header_check(const scb_header_t *h, scb_transport_t t)
{
    if (t != SCB_SPI && t != SCB_TCP && t != SCB_UDP)
    {
        return SCB_TRANSPORT;
    }
    if (h->payload_len > SCB_PAYLOAD_BYTES)
    {
        return SCB_LENGTH;
    }
    if (!json_type(h->type) && h->type != SCB_IDLE && h->type != SCB_TELEMETRY
        && h->type != SCB_LOG_DATA)
    {
        return SCB_TYPE;
    }
    uint16_t flags = h->type == SCB_COMMAND || h->type == SCB_RESULT || h->type == SCB_EVENT
            || h->type == SCB_LOG_DATA
        ? SCB_RELIABLE
        : 0;
    if (h->flags != flags)
    {
        return SCB_FLAGS;
    }
    if (t == SCB_UDP && h->type != SCB_TELEMETRY)
    {
        return SCB_TRANSPORT;
    }
    if (h->type == SCB_TELEMETRY)
    {
        if (h->record_count < 1u || h->record_count > 10u)
        {
            return SCB_RECORD_COUNT;
        }
        if (h->payload_len != SCB_TELEMETRY_PREFIX + SCB_RECORD_BYTES * h->record_count)
        {
            return SCB_LENGTH;
        }
    }
    else if (h->record_count != 0)
    {
        return SCB_RECORD_COUNT;
    }
    if (h->type == SCB_IDLE && h->payload_len != 0)
    {
        return SCB_PAYLOAD;
    }
    return SCB_OK;
}

/**
 * @brief 验证类型对应的负载边界与内容。
 * @details JSON交给有界校验器；遥测结构由头检查保证；日志仅核对长度与offset加法边界。
 * @param[in] h 已通过header_check的非空字段值。
 * @param[in] p 负载非零时不能为空，调用期间只读借用。
 * @param[in,out] w JSON类型必须非空且独占；其他类型不访问。
 * @return 合法SCB_OK，否则返回参数、JSON、长度或负载错误。
 */
static scb_error_t payload_check(
    const scb_header_t *h, const uint8_t *p, bridge_json_workspace_t *w)
{
    if (h->payload_len && !p)
    {
        return SCB_ARGUMENT;
    }
    if (json_type(h->type))
    {
        return bridge_json_validate(p, h->payload_len, w) == BRIDGE_JSON_OK ? SCB_OK : SCB_JSON;
    }
    if (h->type == SCB_LOG_DATA)
    {
        if (h->payload_len < 9u)
        {
            return SCB_LENGTH;
        }
        uint16_t length = get16(p + 4);
        if (length < 1u || length > 960u || h->payload_len != 8u + length)
        {
            return SCB_LENGTH;
        }
        if (get32(p) > UINT32_MAX - length)
        {
            return SCB_PAYLOAD;
        }
    }
    return SCB_OK;
}

/**
 * @brief 验证完整帧并发布借用负载视图。
 * @details 先清空有效view，再按头、传输长度、CRC和负载顺序校验；SPI忽略填充，TCP/UDP必须恰好一帧。失败不发布部分视图。
 * @param[in] b 非空输入帧，n字节可读；视图存续期间保持有效且不变。
 * @param[in] n 输入字节数，SPI必须1024，其他传输为头加实际负载。
 * @param[in] t 传输类别。
 * @param[in,out] w JSON负载时非空且由调用任务独占，其他类型可为空。
 * @param[out] view 不能为空；进入时清零，成功后写头字段及指向b的负载指针。
 * @return 合法SCB_OK，否则首个scb_error_t错误；view为空时直接SCB_ARGUMENT。
 * @note 不执行ACK或业务，不拥有输入。
 */
scb_error_t scb_decode(const uint8_t *b, size_t n, scb_transport_t t, bridge_json_workspace_t *w,
    scb_frame_view_t *view)
{
    if (!view)
    {
        return SCB_ARGUMENT;
    }
    *view = (scb_frame_view_t){0};
    if (!b)
    {
        return SCB_ARGUMENT;
    }
    if (n < SCB_HEADER_BYTES)
    {
        return SCB_LENGTH;
    }
    if (b[0] != 'S' || b[1] != 'C' || b[2] != 'B' || b[3] != '1')
    {
        return SCB_MAGIC;
    }
    if (b[VERSION] != 1)
    {
        return SCB_VERSION;
    }
    scb_header_t h =
    {
        .type = b[TYPE],
        .flags = get16(b + FLAGS),
        .payload_len = get16(b + PAYLOAD_LEN),
        .record_count = get16(b + RECORD_COUNT),
        .session_id = get32(b + SESSION),
        .seq = get32(b + SEQ),
        .ack_seq = get32(b + ACK),
        .request_id = get32(b + REQUEST)
    };
    scb_error_t error = header_check(&h, t);
    if (error != SCB_OK)
    {
        return error;
    }
    if (n != (t == SCB_SPI ? SCB_SPI_BYTES : SCB_HEADER_BYTES + h.payload_len))
    {
        return SCB_LENGTH;
    }
    if (get32(b + CRC) != frame_crc(b, h.payload_len))
    {
        return SCB_CRC;
    }
    error = payload_check(&h, b + SCB_HEADER_BYTES, w);
    if (error != SCB_OK)
    {
        return error;
    }
    view->header = h;
    view->payload = b + SCB_HEADER_BYTES;
    return SCB_OK;
}

/**
 * @brief 共享编码前的结构、容量与地址范围检查。
 * @details 不检查负载内容，不写输出；成功后才允许进入负载校验或已验证写出。
 * @param[in] header 非空头字段，调用期间借用。
 * @param[in] p 非零负载时非空且可读，不得与输出重叠。
 * @param[in] t 目标传输类型。
 * @param[out] out 非空独立输出缓冲；本函数不写入。
 * @param[in] capacity 输出可写字节数。
 * @param[out] used 非空；本函数清零。
 * @return 合法SCB_OK，否则参数、头、容量或别名错误。
 */
static scb_error_t encode_geometry(const scb_header_t *header, const uint8_t *p,
    scb_transport_t t, uint8_t *out, size_t capacity, size_t *used)
{
    if (!used)
    {
        return SCB_ARGUMENT;
    }
    *used = 0;
    if (!header || !out)
    {
        return SCB_ARGUMENT;
    }
    scb_header_t h = *header;
    scb_error_t error = header_check(&h, t);
    if (error != SCB_OK)
    {
        return error;
    }
    size_t n = t == SCB_SPI ? SCB_SPI_BYTES : SCB_HEADER_BYTES + h.payload_len;
    if (capacity < n)
    {
        return SCB_LENGTH;
    }
    if (h.payload_len && !p)
    {
        return SCB_ARGUMENT;
    }
    if (h.payload_len)
    {
        uintptr_t start = (uintptr_t)p;
        uintptr_t dest = (uintptr_t)out;
        if (start > UINTPTR_MAX - h.payload_len || dest > UINTPTR_MAX - n)
        {
            return SCB_ALIAS;
        }
        if (start < dest + n && dest < start + h.payload_len)
        {
            return SCB_ALIAS;
        }
    }
    return SCB_OK;
}

/**
 * @brief 将已经确认可编码的字段和负载写入线图像。
 * @details 清空目标，逐字段写出并重新计算CRC，最后发布长度。
 * @param[in] header 已通过结构检查的头，调用期间借用。
 * @param[in] p 已通过负载检查的字节，非零长度时非空。
 * @param[in] t 已检查的目标传输类型。
 * @param[out] out 容量和别名检查通过的独立目标。
 * @param[out] used 非空，成功后写实际长度。
 * @return SCB_OK。
 * @pre 调用方已完成encode_geometry及负载验证，期间输入未变。
 */
static scb_error_t encode_image(const scb_header_t *header, const uint8_t *p,
    scb_transport_t t, uint8_t *out, size_t *used)
{
    const scb_header_t h = *header;
    const size_t n = t == SCB_SPI ? SCB_SPI_BYTES : SCB_HEADER_BYTES + h.payload_len;
    for (size_t i = 0; i < n; ++i)
    {
        out[i] = 0;
    }
    out[0] = 'S';
    out[1] = 'C';
    out[2] = 'B';
    out[3] = '1';
    out[VERSION] = 1;
    out[TYPE] = h.type;
    put16(out + FLAGS, h.flags);
    put16(out + PAYLOAD_LEN, h.payload_len);
    put16(out + RECORD_COUNT, h.record_count);
    put32(out + SESSION, h.session_id);
    put32(out + SEQ, h.seq);
    put32(out + ACK, h.ack_seq);
    put32(out + REQUEST, h.request_id);
    for (size_t i = 0; i < h.payload_len; ++i)
    {
        out[SCB_HEADER_BYTES + i] = p[i];
    }
    put32(out + CRC, frame_crc(out, h.payload_len));
    *used = n;
    return SCB_OK;
}

/**
 * @brief 验证字段并显式编码到独立输出。
 * @details 先清零有效used，再验证头、容量、负载别名和内容，之后才清目标并逐字段写入。SPI输出1024字节且尾零；其他传输无填充。
 * @param[in] header 非空字段值，调用期间借用。
 * @param[in] p 负载非零时不能为空，payload_len字节可读；不得与输出范围重叠。
 * @param[in] t 传输类别。
 * @param[out] out 非空独立可写缓冲；参数及负载检查失败时不写入。
 * @param[in] capacity 输出可写容量，字节。
 * @param[in,out] w JSON类型时非空且独占，其他类型可为空。
 * @param[out] used 非空；先清零，仅成功后写入实际输出字节数。
 * @return 成功SCB_OK，否则参数、长度、别名或内容错误。
 * @note 调用者保证header、used、workspace等控制对象不与输出非法别名。
 */
scb_error_t scb_encode(const scb_header_t *header, const uint8_t *p, scb_transport_t t,
    uint8_t *out, size_t capacity, bridge_json_workspace_t *w, size_t *used)
{
    scb_error_t error = encode_geometry(header, p, t, out, capacity, used);
    if (error != SCB_OK)
    {
        return error;
    }
    const scb_header_t h = *header;
    error = payload_check(&h, p, w);
    if (error != SCB_OK)
    {
        return error;
    }
    return encode_image(&h, p, t, out, used);
}

/**
 * @brief 重封装已验证负载，不重复扫描JSON或记录内容。
 * @details 保留目标头、容量和别名检查，重建线图像与CRC；不接受任意外部负载。
 * @param[in] frame 已完整验证的帧或其独占队列副本，借用期间内容不变。
 * @param[in] t 目标传输类型。
 * @param[out] out 非空且与负载独立的目标。
 * @param[in] capacity 输出可写字节数。
 * @param[out] used 非空；失败清零，成功发布长度。
 * @return SCB_OK或参数、头、容量、别名错误。
 * @pre type/payload_len/record_count及负载合法性沿用验证结果，仅身份可重封装。
 */
scb_error_t scb_encode_validated(const scb_frame_view_t *frame, scb_transport_t t,
    uint8_t *out, size_t capacity, size_t *used)
{
    if (!frame)
    {
        if (used)
        {
            *used = 0;
        }
        return SCB_ARGUMENT;
    }
    const scb_error_t error
        = encode_geometry(&frame->header, frame->payload, t, out, capacity, used);
    if (error != SCB_OK)
    {
        return error;
    }
    return encode_image(&frame->header, frame->payload, t, out, used);
}
