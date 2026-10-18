#include "protocol.h"
#include <float.h>
#include <limits.h>

enum { MAGIC = 0, VERSION = 4, TYPE = 5, FLAGS = 6, PAYLOAD_LEN = 8,
       RECORD_COUNT = 10, SESSION = 12, SEQ = 16, ACK = 20, REQUEST = 24, CRC = 28 };
_Static_assert(CHAR_BIT == 8 && sizeof(uint16_t) == 2 && sizeof(uint32_t) == 4, "SCB integer widths");
_Static_assert(SCB_HEADER_BYTES == 32 && CRC + 4 == SCB_HEADER_BYTES, "SCB header size");
_Static_assert(VERSION == MAGIC + 4 && TYPE == VERSION + 1 && FLAGS == TYPE + 1 &&
               PAYLOAD_LEN == FLAGS + 2 && RECORD_COUNT == PAYLOAD_LEN + 2 &&
               SESSION == RECORD_COUNT + 2 && SEQ == SESSION + 4 && ACK == SEQ + 4 &&
               REQUEST == ACK + 4 && CRC == REQUEST + 4, "SCB all header offsets");
_Static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128,
               "SCB binary32 requirement");
/* Layout witness for compile-time cross-check only: never cast received bytes
 * to this type and never send it. All wire reads below use byte offsets. */
typedef struct {
    uint32_t tick;
    int16_t adc[4], speed_l, speed_r, track_error, track_out, gyro_x, gyro_y, gyro_z;
    uint16_t dl1b_mm, battery;
    uint8_t kernel_state, roundabout_state;
    uint16_t gray_raw[16];
    uint32_t gray_sequence, control_sequence;
    int16_t em_error, optical_error;
    uint16_t protection_flags;
    uint8_t mode, optical_state, gray_valid, stop_reason, reserved[14];
} record_layout_witness_t;
#define OFFSET(field, value) _Static_assert(offsetof(record_layout_witness_t, field) == value, #field " offset")
OFFSET(tick, 0); OFFSET(adc, 4); OFFSET(speed_l, 12); OFFSET(speed_r, 14);
OFFSET(track_error, 16); OFFSET(track_out, 18); OFFSET(gyro_x, 20); OFFSET(gyro_y, 22);
OFFSET(gyro_z, 24); OFFSET(dl1b_mm, 26); OFFSET(battery, 28); OFFSET(kernel_state, 30);
OFFSET(roundabout_state, 31); OFFSET(gray_raw, 32); OFFSET(gray_sequence, 64);
OFFSET(control_sequence, 68); OFFSET(em_error, 72); OFFSET(optical_error, 74);
OFFSET(protection_flags, 76); OFFSET(mode, 78); OFFSET(optical_state, 79);
OFFSET(gray_valid, 80); OFFSET(stop_reason, 81); OFFSET(reserved, 82);
_Static_assert(sizeof(record_layout_witness_t) == SCB_RECORD_BYTES, "96-byte record");
_Static_assert(SCB_TELEMETRY_PREFIX + 10 * SCB_RECORD_BYTES + SCB_HEADER_BYTES == 1016,
               "UDP maximum telemetry frame");

static uint16_t get16(const uint8_t *b) { return (uint16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8)); }
static uint32_t get32(const uint8_t *b)
{
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}
static void put16(uint8_t *b, uint16_t x) { b[0] = (uint8_t)x; b[1] = (uint8_t)(x >> 8); }
static void put32(uint8_t *b, uint32_t x)
{
    for (unsigned i = 0; i < 4u; ++i) b[i] = (uint8_t)(x >> (i * 8u));
}
static uint32_t crc_update(uint32_t crc, const uint8_t *b, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        crc ^= b[i];
        for (unsigned bit = 0; bit < 8u; ++bit) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return crc;
}
static uint32_t frame_crc(const uint8_t *b, size_t payload_len)
{
    return ~crc_update(crc_update(UINT32_MAX, b, CRC), b + SCB_HEADER_BYTES, payload_len);
}
static bool json_type(uint8_t type)
{
    return type == SCB_HELLO || type == SCB_HELLO_ACK || type == SCB_COMMAND ||
           type == SCB_RESULT || type == SCB_EVENT;
}
static scb_error_t header_check(const scb_header_t *h, scb_transport_t t)
{
    if (t != SCB_SPI && t != SCB_TCP && t != SCB_UDP) return SCB_TRANSPORT;
    if (h->payload_len > SCB_PAYLOAD_BYTES) return SCB_LENGTH;
    if (!json_type(h->type) && h->type != SCB_IDLE && h->type != SCB_TELEMETRY && h->type != SCB_LOG_DATA)
        return SCB_TYPE;
    uint16_t flags = h->type == SCB_COMMAND || h->type == SCB_RESULT || h->type == SCB_EVENT ||
                     h->type == SCB_LOG_DATA ? SCB_RELIABLE : 0;
    if (h->flags != flags) return SCB_FLAGS;
    if (t == SCB_UDP && h->type != SCB_TELEMETRY) return SCB_TRANSPORT;
    if (h->type == SCB_TELEMETRY) {
        if (h->record_count < 1u || h->record_count > 10u) return SCB_RECORD_COUNT;
        if (h->payload_len != SCB_TELEMETRY_PREFIX + SCB_RECORD_BYTES * h->record_count) return SCB_LENGTH;
    } else if (h->record_count != 0) return SCB_RECORD_COUNT;
    if (h->type == SCB_IDLE && h->payload_len != 0) return SCB_PAYLOAD;
    return SCB_OK;
}
static scb_error_t payload_check(const scb_header_t *h, const uint8_t *p, bridge_json_workspace_t *w)
{
    if (h->payload_len && !p) return SCB_ARGUMENT;
    if (json_type(h->type))
        return bridge_json_validate(p, h->payload_len, w) == BRIDGE_JSON_OK ? SCB_OK : SCB_JSON;
    if (h->type == SCB_TELEMETRY) {
        if (get32(p + 20) == 0 || get32(p + 16) != get32(p + SCB_TELEMETRY_PREFIX)) return SCB_PAYLOAD;
        for (size_t record = 0; record < h->record_count; ++record)
            for (size_t offset = 82; offset < SCB_RECORD_BYTES; ++offset)
                if (p[SCB_TELEMETRY_PREFIX + SCB_RECORD_BYTES * record + offset]) return SCB_PAYLOAD;
    } else if (h->type == SCB_LOG_DATA) {
        if (h->payload_len < 9u) return SCB_LENGTH;
        uint16_t length = get16(p + 4);
        if (length < 1u || length > 960u || h->payload_len != 8u + length) return SCB_LENGTH;
        if (get16(p + 6) != 0 || get32(p) > UINT32_MAX - length) return SCB_PAYLOAD;
    }
    return SCB_OK;
}
scb_error_t scb_decode(const uint8_t *b, size_t n, scb_transport_t t,
                       bridge_json_workspace_t *w, scb_frame_view_t *view)
{
    if (!view) return SCB_ARGUMENT;
    *view = (scb_frame_view_t){0};
    if (!b) return SCB_ARGUMENT;
    if (n < SCB_HEADER_BYTES) return SCB_LENGTH;
    if (b[0] != 'S' || b[1] != 'C' || b[2] != 'B' || b[3] != '1') return SCB_MAGIC;
    if (b[VERSION] != 1) return SCB_VERSION;
    scb_header_t h = {.type = b[TYPE], .flags = get16(b + FLAGS), .payload_len = get16(b + PAYLOAD_LEN),
        .record_count = get16(b + RECORD_COUNT), .session_id = get32(b + SESSION), .seq = get32(b + SEQ),
        .ack_seq = get32(b + ACK), .request_id = get32(b + REQUEST)};
    scb_error_t error = header_check(&h, t);
    if (error != SCB_OK) return error;
    if (n != (t == SCB_SPI ? SCB_SPI_BYTES : SCB_HEADER_BYTES + h.payload_len)) return SCB_LENGTH;
    if (get32(b + CRC) != frame_crc(b, h.payload_len)) return SCB_CRC;
    error = payload_check(&h, b + SCB_HEADER_BYTES, w);
    if (error != SCB_OK) return error;
    view->header = h; view->payload = b + SCB_HEADER_BYTES;
    return SCB_OK;
}
scb_error_t scb_encode(const scb_header_t *header, const uint8_t *p, scb_transport_t t,
                       uint8_t *out, size_t capacity, bridge_json_workspace_t *w, size_t *used)
{
    if (!used) return SCB_ARGUMENT;
    *used = 0;
    if (!header || !out) return SCB_ARGUMENT;
    scb_header_t h = *header;
    scb_error_t error = header_check(&h, t);
    if (error != SCB_OK) return error;
    size_t n = t == SCB_SPI ? SCB_SPI_BYTES : SCB_HEADER_BYTES + h.payload_len;
    if (capacity < n) return SCB_LENGTH;
    if (h.payload_len && p) {
        uintptr_t start = (uintptr_t)p, dest = (uintptr_t)out;
        if (start > UINTPTR_MAX - h.payload_len || dest > UINTPTR_MAX - n) return SCB_ALIAS;
        if (start < dest + n && dest < start + h.payload_len) return SCB_ALIAS;
    }
    error = payload_check(&h, p, w);
    if (error != SCB_OK) return error;
    for (size_t i = 0; i < n; ++i) out[i] = 0;
    out[0] = 'S'; out[1] = 'C'; out[2] = 'B'; out[3] = '1'; out[VERSION] = 1; out[TYPE] = h.type;
    put16(out + FLAGS, h.flags); put16(out + PAYLOAD_LEN, h.payload_len); put16(out + RECORD_COUNT, h.record_count);
    put32(out + SESSION, h.session_id); put32(out + SEQ, h.seq); put32(out + ACK, h.ack_seq);
    put32(out + REQUEST, h.request_id);
    for (size_t i = 0; i < h.payload_len; ++i) out[SCB_HEADER_BYTES + i] = p[i];
    put32(out + CRC, frame_crc(out, h.payload_len));
    *used = n;
    return SCB_OK;
}
