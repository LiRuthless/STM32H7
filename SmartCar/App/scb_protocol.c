/**
 * @file scb_protocol.c
 * @brief SCB/1 显式字节编解码与无堆 JSON 解析。
 * @details 前台将1024B事务校验成借用帧，再将完整JSON解析到有界令牌与字符串池。
 *     不访问硬件、不执行业务副作用；共享解析工作区仅由前台串行使用。
 */
#include "scb_protocol.h"
#include <string.h>
#include <float.h>

typedef char scb_header_assert_t[(SCB_HEADER_SIZE + SCB_PAYLOAD_MAX == SCB_SPI_SIZE) ? 1 : -1];
typedef char scb_float_assert_t[(sizeof(float) == 4 && FLT_RADIX == 2) ? 1 : -1];

/* 命令解析工作区由前台独占，不占中断或前台调用栈。 */
static scb_json_t s_command_json;
/* 前台独占的显式容器栈，避免八层JSON在C调用栈重复保留解析帧。 */
static int16_t s_json_containers[SCB_JSON_DEPTH];
static uint8_t s_json_phases[SCB_JSON_DEPTH];

static int16_t SCB_ParseValue(scb_json_t *json, int16_t parent, uint8_t depth);

/**
 * @brief 从两字节读取小端整数。
 * @param[in] data 非空且至少两字节的缓冲。
 * @return 解码的无符号值。
 */
uint16_t SCB_ReadU16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | (uint16_t)data[1] << 8);
}

/**
 * @brief 从四字节读取小端整数。
 * @param[in] data 非空且至少四字节的缓冲。
 * @return 解码的无符号值。
 */
uint32_t SCB_ReadU32(const uint8_t *data)
{
    return (uint32_t)data[0] | (uint32_t)data[1] << 8
        | (uint32_t)data[2] << 16 | (uint32_t)data[3] << 24;
}

/**
 * @brief 显式写入小端两字节。
 * @param[out] data 非空且至少两字节的目标。
 * @param[in] value 待编码整数。
 */
void SCB_WriteU16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
}

/**
 * @brief 显式写入小端四字节。
 * @param[out] data 非空且至少四字节的目标。
 * @param[in] value 待编码整数。
 */
void SCB_WriteU32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

/**
 * @brief 按反射多项式续算 zlib 兼容CRC。
 * @details 每次先撤销前次最终异或，逐位计算后再次异或；初次previous为零。
 * @param[in] previous 前段已完成的CRC，首次为0。
 * @param[in] data 非空数据，length为零时可为空。
 * @param[in] length 字节数。
 * @return 可继续输入下一段的最终CRC。
 */
uint32_t SCB_CalculateCrc(uint32_t previous, const uint8_t *data, size_t length)
{
    uint32_t crc = previous ^ 0xffffffffu;
    size_t i;
    uint8_t bit;
    for (i = 0; i < length; ++i)
    {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit)
        {
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
        }
    }
    return crc ^ 0xffffffffu;
}

/**
 * @brief 校验消息种类与对应负载约束。
 * @details 公共消息表之外的类型全部拒绝；可靠位必须精确符合消息种类。
 * @param[in] frame 非空待检查帧描述。
 * @return 规则全部成立返回1，否则0。
 */
static uint8_t SCB_BuildFrame(const scb_frame_t *frame)
{
    uint8_t reliable;
    switch (frame->type)
    {
        case SCB_HELLO:
        case SCB_HELLO_ACK:
        case SCB_IDLE:
        case SCB_TELEMETRY:
            reliable = 0;
            break;
        case SCB_COMMAND:
        case SCB_RESULT:
        case SCB_EVENT:
        case SCB_LOG_DATA:
            reliable = 1;
            break;
        default:
            return 0;
    }
    if (frame->flags != reliable || frame->payload_len > SCB_PAYLOAD_MAX)
    {
        return 0;
    }
    if (frame->type == SCB_TELEMETRY)
    {
        return frame->record_count >= 1 && frame->record_count <= 10
            && frame->payload_len == 24u + frame->record_count * 96u;
    }
    if (frame->record_count || (frame->type == SCB_IDLE && frame->payload_len))
    {
        return 0;
    }
    return 1;
}

/**
 * @brief 编码完整SPI事务并清零全部填充。
 * @details 先验证描述再逐字段写入，CRC只覆盖头0～27及实际负载，拒绝无效输入。
 * @param[out] output 非空1024B目标，不能与payload重叠；失败时不写入。
 * @param[in] frame 非空描述，非零负载需有效payload。
 * @return 成功1，参数或字段无效0。
 */
uint8_t SCB_EncodeFrame(uint8_t *output, const scb_frame_t *frame)
{
    uint32_t crc;
    if (!output || !frame || (frame->payload_len && !frame->payload) || !SCB_BuildFrame(frame))
    {
        return 0;
    }
    memset(output, 0, SCB_SPI_SIZE);
    memcpy(output, "SCB1", 4);
    output[4] = 1;
    output[5] = frame->type;
    SCB_WriteU16(output + 6, frame->flags);
    SCB_WriteU16(output + 8, frame->payload_len);
    SCB_WriteU16(output + 10, frame->record_count);
    SCB_WriteU32(output + 12, frame->session_id);
    SCB_WriteU32(output + 16, frame->seq);
    SCB_WriteU32(output + 20, frame->ack_seq);
    SCB_WriteU32(output + 24, frame->request_id);
    if (frame->payload_len)
    {
        memcpy(output + SCB_HEADER_SIZE, frame->payload, frame->payload_len);
    }
    crc = SCB_CalculateCrc(0, output, 28);
    crc = SCB_CalculateCrc(crc, output + SCB_HEADER_SIZE, frame->payload_len);
    SCB_WriteU32(output + 28, crc);
    return 1;
}

/**
 * @brief 验证帧字段和CRC后发布描述。
 * @details 接受精确网络帧或1024B事务；先在局部构造描述，全部通过才写输出。
 * @param[in] input 非空输入，不要求对齐。
 * @param[in] length 实际缓冲长度，32～1024B。
 * @param[out] frame 非空输出，失败保留原值，payload借用input。
 * @return 合法1，否则0；不产生ACK或业务执行。
 */
uint8_t SCB_DecodeFrame(const uint8_t *input, size_t length, scb_frame_t *frame)
{
    scb_frame_t decoded;
    uint32_t crc;
    if (!input || !frame || length < SCB_HEADER_SIZE || length > SCB_SPI_SIZE
        || memcmp(input, "SCB1", 4) || input[4] != 1)
    {
        return 0;
    }
    decoded.type = input[5];
    decoded.flags = SCB_ReadU16(input + 6);
    decoded.payload_len = SCB_ReadU16(input + 8);
    decoded.record_count = SCB_ReadU16(input + 10);
    decoded.session_id = SCB_ReadU32(input + 12);
    decoded.seq = SCB_ReadU32(input + 16);
    decoded.ack_seq = SCB_ReadU32(input + 20);
    decoded.request_id = SCB_ReadU32(input + 24);
    decoded.payload = input + SCB_HEADER_SIZE;
    if (!SCB_BuildFrame(&decoded) || (length != SCB_SPI_SIZE
        && length != SCB_HEADER_SIZE + decoded.payload_len))
    {
        return 0;
    }
    crc = SCB_CalculateCrc(0, input, 28);
    crc = SCB_CalculateCrc(crc, decoded.payload, decoded.payload_len);
    if (crc != SCB_ReadU32(input + 28))
    {
        return 0;
    }
    *frame = decoded;
    return 1;
}

/**
 * @brief 校验UTF-8编码和禁止的NUL。
 * @details 检查最短编码、代理区及Unicode上界，避免不同非法编码形成相同命令键。
 * @param[in] data 非空字节区。
 * @param[in] length 输入长度。
 * @return 全部合法1，否则0。
 */
static uint8_t SCB_ValidateUtf8(const uint8_t *data, uint16_t length)
{
    uint16_t i = 0;
    while (i < length)
    {
        uint32_t code = data[i++];
        uint32_t minimum;
        uint8_t count;
        if (!code)
        {
            return 0;
        }
        if (code < 0x80)
        {
            continue;
        }
        if (code >= 0xc2 && code <= 0xdf)
        {
            count = 1;
            minimum = 0x80;
            code &= 0x1f;
        }
        else if (code >= 0xe0 && code <= 0xef)
        {
            count = 2;
            minimum = 0x800;
            code &= 0x0f;
        }
        else if (code >= 0xf0 && code <= 0xf4)
        {
            count = 3;
            minimum = 0x10000;
            code &= 7;
        }
        else
        {
            return 0;
        }
        while (count--)
        {
            if (i == length || (data[i] & 0xc0) != 0x80)
            {
                return 0;
            }
            code = (code << 6) | (data[i++] & 0x3f);
        }
        if (code < minimum || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff))
        {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief 跳过JSON允许的四种空白。
 * @details 只前移当前位置，不读取length之外的字节。
 * @param[in,out] json 非空解析工作区。
 */
static void SCB_SkipSpace(scb_json_t *json)
{
    while (json->position < json->length)
    {
        uint8_t ch = json->input[json->position];
        if (ch != ' ' && ch != '\r' && ch != '\n' && ch != '\t')
        {
            break;
        }
        ++json->position;
    }
}

/**
 * @brief 读取四位Unicode转义。
 * @details 失败可能前移position，但整份文档随后拒绝，不发布部分命令。
 * @param[in,out] json 非空工作区，当前位置在首个十六进制位。
 * @return 0～65535码元，失败-1。
 */
static int32_t SCB_ReadHex(scb_json_t *json)
{
    uint8_t i;
    int32_t value = 0;
    for (i = 0; i < 4; ++i)
    {
        uint8_t ch;
        if (json->position == json->length)
        {
            return -1;
        }
        ch = json->input[json->position++];
        value <<= 4;
        if (ch >= '0' && ch <= '9')
        {
            value += ch - '0';
        }
        else if (ch >= 'a' && ch <= 'f')
        {
            value += ch - 'a' + 10;
        }
        else if (ch >= 'A' && ch <= 'F')
        {
            value += ch - 'A' + 10;
        }
        else
        {
            return -1;
        }
    }
    return value;
}

/**
 * @brief 将一个Unicode码点写到静态字符串池。
 * @details 预检最大所需空间；只编码有效非NUL标量，另留最终字符串终止位置。
 * @param[in,out] json 非空解析工作区。
 * @param[in] code 已合并代理对的码点。
 * @return 写入成功1，非法码点或容量不足0。
 */
static uint8_t SCB_AppendUtf8(scb_json_t *json, uint32_t code)
{
    uint8_t bytes = code < 0x80 ? 1 : (code < 0x800 ? 2 : (code < 0x10000 ? 3 : 4));
    uint8_t i;
    uint16_t start = json->used;
    if (!code || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff)
        || start + bytes >= sizeof(json->strings))
    {
        return 0;
    }
    json->used += bytes;
    for (i = bytes; i > 1; --i)
    {
        json->strings[start + i - 1] = (char)(0x80 | (code & 0x3f));
        code >>= 6;
    }
    json->strings[start] = (char)(code | (bytes == 1 ? 0 : (bytes == 2 ? 0xc0
        : (bytes == 3 ? 0xe0 : 0xf0))));
    return 1;
}

/**
 * @brief 解码字符串及JSON转义。
 * @details 输入UTF-8已整体验证，原始多字节直接复制；Unicode转义合并代理对再编码。
 * @param[in,out] json 非空解析工作区，当前位置为引号。
 * @param[out] token 非空已分配令牌，成功写入字符串池位置和长度。
 * @return 完整闭合字符串1，否则0。
 */
static uint8_t SCB_ParseString(scb_json_t *json, scb_token_t *token)
{
    token->type = SCB_JSON_STRING;
    token->text = json->used;
    ++json->position;
    while (json->position < json->length)
    {
        uint8_t ch = json->input[json->position++];
        if (ch == '"')
        {
            token->length = json->used - token->text;
            if (json->used >= sizeof(json->strings))
            {
                return 0;
            }
            json->strings[json->used++] = 0;
            return 1;
        }
        if (ch < 0x20)
        {
            return 0;
        }
        if (ch == '\\')
        {
            if (json->position == json->length)
            {
                return 0;
            }
            ch = json->input[json->position++];
            if (ch == 'u')
            {
                int32_t code = SCB_ReadHex(json);
                if (code < 0)
                {
                    return 0;
                }
                if (code >= 0xd800 && code <= 0xdbff)
                {
                    int32_t low;
                    if (json->position + 2 > json->length
                        || json->input[json->position++] != '\\'
                        || json->input[json->position++] != 'u')
                    {
                        return 0;
                    }
                    low = SCB_ReadHex(json);
                    if (low < 0xdc00 || low > 0xdfff)
                    {
                        return 0;
                    }
                    code = 0x10000 + ((code - 0xd800) << 10) + low - 0xdc00;
                }
                if (!SCB_AppendUtf8(json, (uint32_t)code))
                {
                    return 0;
                }
                continue;
            }
            switch (ch)
            {
                case 'b':
                    ch = '\b';
                    break;
                case 'f':
                    ch = '\f';
                    break;
                case 'n':
                    ch = '\n';
                    break;
                case 'r':
                    ch = '\r';
                    break;
                case 't':
                    ch = '\t';
                    break;
                case '"':
                case '\\':
                case '/':
                    break;
                default:
                    return 0;
            }
        }
        if (json->used + 1u >= sizeof(json->strings))
        {
            return 0;
        }
        json->strings[json->used++] = (char)ch;
    }
    return 0;
}

/**
 * @brief 检查JSON数字文法并转换有限值。
 * @details 拒绝前导零、空小数/指数以及非有限转换；整数标记用于速度风扇类型校验。
 * @param[in,out] json 非空解析工作区。
 * @param[out] token 非空令牌，成功写数字和整数标志。
 * @return 完整数字1，非法或超过63字节0。
 */
static uint8_t SCB_ParseNumber(scb_json_t *json, scb_token_t *token)
{
    char number[64];
    uint16_t start = json->position;
    uint16_t position = start;
    uint16_t i;
    int32_t exponent = 0;
    int32_t fraction = 0;
    uint8_t decimal = 0;
    uint8_t negative;
    uint8_t exponent_negative = 0;
    double value = 0;
    token->type = SCB_JSON_NUMBER;
    token->integer = 1;
    if (position < json->length && json->input[position] == '-')
    {
        ++position;
    }
    if (position == json->length || json->input[position] < '0' || json->input[position] > '9')
    {
        return 0;
    }
    if (json->input[position] == '0')
    {
        ++position;
    }
    else
    {
        while (position < json->length && json->input[position] >= '0'
            && json->input[position] <= '9')
        {
            ++position;
        }
    }
    if (position < json->length && json->input[position] == '.')
    {
        uint16_t digits = ++position;
        token->integer = 0;
        while (position < json->length && json->input[position] >= '0'
            && json->input[position] <= '9')
        {
            ++position;
        }
        if (digits == position)
        {
            return 0;
        }
    }
    if (position < json->length && (json->input[position] == 'e' || json->input[position] == 'E'))
    {
        uint16_t digits;
        token->integer = 0;
        ++position;
        if (position < json->length
            && (json->input[position] == '+' || json->input[position] == '-'))
        {
            ++position;
        }
        digits = position;
        while (position < json->length && json->input[position] >= '0'
            && json->input[position] <= '9')
        {
            ++position;
        }
        if (digits == position)
        {
            return 0;
        }
    }
    if ((size_t)(position - start) >= sizeof(number))
    {
        return 0;
    }
    memcpy(number, json->input + start, position - start);
    number[position - start] = 0;
    /* 自行转换，避免newlib strtod的Bigint路径隐式分配堆。
     * 输入最多63B，十进制尾数累加不会先溢出double；指数只做有界缩放。 */
    negative = number[0] == '-';
    for (i = negative; number[i] && number[i] != 'e' && number[i] != 'E'; ++i)
    {
        if (number[i] == '.')
        {
            decimal = 1;
        }
        else
        {
            value = value * 10 + number[i] - '0';
            fraction += decimal;
        }
    }
    if (number[i])
    {
        ++i;
        exponent_negative = number[i] == '-';
        if (number[i] == '-' || number[i] == '+')
        {
            ++i;
        }
        for (; number[i]; ++i)
        {
            if (exponent < 10000)
            {
                exponent = exponent * 10 + number[i] - '0';
            }
        }
    }
    exponent = (exponent_negative ? -exponent : exponent) - fraction;
    if (exponent > 400 && value != 0)
    {
        return 0;
    }
    if (exponent < -400)
    {
        value = 0;
        exponent = 0;
    }
    while (exponent > 0 && value != 0)
    {
        value *= 10;
        --exponent;
    }
    while (exponent < 0)
    {
        value /= 10;
        ++exponent;
    }
    if (value > DBL_MAX)
    {
        return 0;
    }
    token->number = negative ? -value : value;
    json->position = position;
    return 1;
}

/**
 * @brief 解析一个标量或容器起始令牌。
 * @details 只分配一个令牌；容器的子项由ParseJson使用静态显式栈推进，不递归调用本函数。
 * @param[in,out] json 非空工作区。
 * @param[in] parent 父令牌编号，根为-1。
 * @param[in] depth 当前深度，根为0，容器最多8层。
 * @return 成功令牌编号，任何语法或容量错误-1。
 */
static int16_t SCB_ParseValue(scb_json_t *json, int16_t parent, uint8_t depth)
{
    int16_t index;
    scb_token_t *token;
    uint8_t ch;
    SCB_SkipSpace(json);
    if (json->position == json->length || json->count == SCB_JSON_TOKENS
        || depth > SCB_JSON_DEPTH)
    {
        return -1;
    }
    index = (int16_t)json->count++;
    token = &json->tokens[index];
    memset(token, 0, sizeof(*token));
    token->parent = parent;
    ch = json->input[json->position];
    if (ch == '"')
    {
        return SCB_ParseString(json, token) ? index : -1;
    }
    if (ch == '{' || ch == '[')
    {
        token->type = ch == '{' ? SCB_JSON_OBJECT : SCB_JSON_ARRAY;
        ++json->position;
        return index;
    }
    if (ch == 't' || ch == 'f' || ch == 'n')
    {
        const char *literal = ch == 't' ? "true" : (ch == 'f' ? "false" : "null");
        uint16_t length = (uint16_t)strlen(literal);
        if (json->position + length > json->length
            || memcmp(json->input + json->position, literal, length))
        {
            return -1;
        }
        token->type = ch == 't' ? SCB_JSON_TRUE : (ch == 'f' ? SCB_JSON_FALSE : SCB_JSON_NULL);
        json->position += length;
        return index;
    }
    return SCB_ParseNumber(json, token) ? index : -1;
}

/**
 * @brief 校验并解析完整JSON文档。
 * @details 拒绝非法UTF-8、NUL、尾随数据与容量越界；失败工作区不得继续作为有效文档读取。
 * @param[in] input 非空无NUL负载。
 * @param[in] length 1～992B。
 * @param[out] json 非空调用方工作区，允许静态分配。
 * @return 完整解析成功1，否则0。
 * @note 工作区与输入不能重叠；无动态分配。
 */
uint8_t SCB_ParseJson(const uint8_t *input, uint16_t length, scb_json_t *json)
{
    int16_t root;
    uint8_t depth = 0;
    if (!input || !json || !length || length > SCB_PAYLOAD_MAX || !SCB_ValidateUtf8(input, length))
    {
        return 0;
    }
    json->count = json->used = json->position = 0;
    json->input = input;
    json->length = length;
    root = SCB_ParseValue(json, -1, 0);
    if (root < 0)
    {
        return 0;
    }
    if (json->tokens[root].type == SCB_JSON_OBJECT || json->tokens[root].type == SCB_JSON_ARRAY)
    {
        s_json_containers[depth] = root;
        s_json_phases[depth++] = 0;
    }
    /* 容器阶段0允许空结尾，阶段1是逗号后的必需子项，阶段2等待分隔符。
     * 每层状态静态存放，任一失败立即整份拒绝，调用栈不随嵌套深度增长。 */
    while (depth)
    {
        int16_t parent = s_json_containers[depth - 1];
        uint8_t object = json->tokens[parent].type == SCB_JSON_OBJECT;
        uint8_t close = object ? '}' : ']';
        uint8_t phase = s_json_phases[depth - 1];
        int16_t child;
        SCB_SkipSpace(json);
        if (json->position == json->length)
        {
            return 0;
        }
        if (json->input[json->position] == close && phase != 1)
        {
            ++json->position;
            --depth;
            continue;
        }
        if (phase == 2)
        {
            if (json->input[json->position++] != ',')
            {
                return 0;
            }
            s_json_phases[depth - 1] = 1;
            continue;
        }
        if (object)
        {
            int16_t old;
            int16_t key;
            if (json->input[json->position] != '"')
            {
                return 0;
            }
            key = SCB_ParseValue(json, parent, depth);
            if (key < 0)
            {
                return 0;
            }
            json->tokens[key].key = 1;
            for (old = parent + 1; old < key; ++old)
            {
                if (json->tokens[old].parent == parent && json->tokens[old].key
                    && !strcmp(json->strings + json->tokens[old].text,
                        json->strings + json->tokens[key].text))
                {
                    return 0;
                }
            }
            SCB_SkipSpace(json);
            if (json->position == json->length || json->input[json->position++] != ':')
            {
                return 0;
            }
        }
        child = SCB_ParseValue(json, parent, depth);
        if (child < 0)
        {
            return 0;
        }
        s_json_phases[depth - 1] = 2;
        if (json->tokens[child].type == SCB_JSON_OBJECT
            || json->tokens[child].type == SCB_JSON_ARRAY)
        {
            if (depth == SCB_JSON_DEPTH)
            {
                return 0;
            }
            s_json_containers[depth] = child;
            s_json_phases[depth++] = 0;
        }
    }
    SCB_SkipSpace(json);
    return json->position == length;
}

/**
 * @brief 查找对象的直接成员值。
 * @details 键已经解码并排重；值令牌紧跟对应键，不搜索嵌套对象。
 * @param[in] json 非空成功解析文档。
 * @param[in] object 目标对象编号。
 * @param[in] key 非空UTF-8键。
 * @return 值令牌编号，缺失或非对象-1。
 */
int16_t SCB_FindMember(const scb_json_t *json, int16_t object, const char *key)
{
    uint16_t i;
    if (object < 0 || object >= json->count || json->tokens[object].type != SCB_JSON_OBJECT)
    {
        return -1;
    }
    for (i = object + 1; i + 1 < json->count; ++i)
    {
        if (json->tokens[i].parent == object && json->tokens[i].key
            && !strcmp(json->strings + json->tokens[i].text, key))
        {
            return (int16_t)(i + 1);
        }
    }
    return -1;
}

/**
 * @brief 取得已解码字符串。
 * @param[in] json 非空成功解析文档。
 * @param[in] token 令牌编号。
 * @return 文档寿命内只读字符串，类型不符或越界为空。
 */
const char *SCB_GetString(const scb_json_t *json, int16_t token)
{
    if (token < 0 || token >= json->count || json->tokens[token].type != SCB_JSON_STRING)
    {
        return NULL;
    }
    return json->strings + json->tokens[token].text;
}

/**
 * @brief 复制已校验有限数字。
 * @param[in] json 非空成功解析文档。
 * @param[in] token 令牌编号。
 * @param[out] value 非空输出，失败不写。
 * @return 数字类型正确1，否则0。
 */
uint8_t SCB_GetNumber(const scb_json_t *json, int16_t token, double *value)
{
    if (token < 0 || token >= json->count || json->tokens[token].type != SCB_JSON_NUMBER)
    {
        return 0;
    }
    *value = json->tokens[token].number;
    return 1;
}

/**
 * @brief 返回公共业务状态名称。
 * @param[in] status 状态枚举。
 * @return 静态只读拼写，非法枚举按INVALID_ARG返回。
 */
const char *SCB_StatusName(scb_status_t status)
{
    /* 公共状态拼写只读表，所有调用上下文共享但不写入。 */
    static const char *const s_names[] =
    {
        "OK", "RECEIVED", "APPLIED", "SAVED", "INVALID_ARG", "OUT_OF_RANGE", "BUSY",
        "SPECIAL_ELEMENT", "NOT_FOUND", "IO_ERROR"
    };
    return status >= SCB_OK && status <= SCB_IO_ERROR ? s_names[status] : "INVALID_ARG";
}

/**
 * @brief 将完整JSON解析成命令快照。
 * @details 先验证整份JSON，随后检查必需字段类型；参数限值留给Tuning统一执行，
 *     整数在窄化前检查范围。未知命令是业务INVALID_ARG，不执行任何部分请求。
 * @param[in] input 非空命令字节。
 * @param[in] length 负载字节数。
 * @param[out] command 非空完整命令，只有status为OK时可执行。
 * @param[out] status 非空业务结果；语法错误返回0且不可ACK。
 * @return JSON语法完整1，否则0。
 * @note 前台串行调用，使用静态解析工作区。
 */
uint8_t SCB_ParseCommand(const uint8_t *input, uint16_t length,
    scb_command_t *command, scb_status_t *status)
{
    scb_json_t *json = &s_command_json;
    int16_t args;
    const char *op;
    double value;
    uint8_t i;
    *status = SCB_INVALID_ARG;
    memset(command, 0, sizeof(*command));
    if (!SCB_ParseJson(input, length, json))
    {
        return 0;
    }
    op = SCB_GetString(json, SCB_FindMember(json, 0, "op"));
    args = SCB_FindMember(json, 0, "args");
    if (!op || strlen(op) >= sizeof(command->op) || args < 0
        || json->tokens[args].type != SCB_JSON_OBJECT)
    {
        return 1;
    }
    for (i = 0; op[i]; ++i)
    {
        if ((op[i] < 'A' || op[i] > 'Z') && op[i] != '_')
        {
            return 1;
        }
    }
    strcpy(command->op, op);
    if (!strcmp(op, "GET_PARAMS") || !strcmp(op, "SET_PARAMS"))
    {
        if (!SCB_GetNumber(json, SCB_FindMember(json, args, "mode"), &value)
            || (value != 0 && value != 1))
        {
            return 1;
        }
        command->mode = (uint8_t)value;
        if (!SCB_GetNumber(json, SCB_FindMember(json, args, "gear"), &value)
            || (value != 0 && value != 1))
        {
            return 1;
        }
        command->gear = (uint8_t)value;
    }
    if (!strcmp(op, "SET_PARAMS"))
    {
        /* 完整八项键表，前台逐项校验，不能缺项执行。 */
        static const char *const s_keys[] =
        {
            "kp", "k2p", "kd", "kp_v", "ki_v", "base_speed", "fan_run", "fan_idle"
        };
        int16_t values = SCB_FindMember(json, args, "values");
        float gains[5];
        int32_t integers[3];
        for (i = 0; i < 8; ++i)
        {
            int16_t token = SCB_FindMember(json, values, s_keys[i]);
            if (!SCB_GetNumber(json, token, &value)
                || (i >= 5 && !json->tokens[token].integer))
            {
                return 1;
            }
            if (value < 0 || value > (i < 5 ? 100 : (i == 5 ? 300 : 10000)))
            {
                *status = SCB_OUT_OF_RANGE;
                return 1;
            }
            if (i < 5)
            {
                gains[i] = (float)value;
            }
            else
            {
                integers[i - 5] = (int32_t)value;
            }
        }
        command->values = (scb_values_t)
        {
            gains[0], gains[1], gains[2], gains[3], gains[4],
            integers[0], integers[1], integers[2]
        };
    }
    else if (!strcmp(op, "GET_REQUEST_RESULT") || !strcmp(op, "LOG_READ")
        || !strcmp(op, "LOG_END"))
    {
        const char *key = !strcmp(op, "GET_REQUEST_RESULT") ? "target_request_id" : "transfer_id";
        int16_t token = SCB_FindMember(json, args, key);
        if (!SCB_GetNumber(json, token, &value) || !json->tokens[token].integer
            || value < 1 || value > 4294967295.0)
        {
            return 1;
        }
        if (!strcmp(op, "GET_REQUEST_RESULT"))
        {
            command->target_request_id = (uint32_t)value;
        }
        else
        {
            command->transfer_id = (uint32_t)value;
        }
        if (!strcmp(op, "LOG_READ"))
        {
            token = SCB_FindMember(json, args, "offset");
            if (!SCB_GetNumber(json, token, &value) || !json->tokens[token].integer
                || value < 0 || value > 4294967295.0)
            {
                return 1;
            }
            command->offset = (uint32_t)value;
            token = SCB_FindMember(json, args, "length");
            if (!SCB_GetNumber(json, token, &value) || !json->tokens[token].integer
                || value < 1 || value > 960)
            {
                return 1;
            }
            command->length = (uint32_t)value;
        }
    }
    else if (strcmp(op, "GET_PARAMS") && strcmp(op, "GET_STATUS")
        && strcmp(op, "TUNE_ENABLE") && strcmp(op, "TUNE_DISABLE")
        && strcmp(op, "GET_LOG_META") && strcmp(op, "LOG_BEGIN"))
    {
        return 1;
    }
    *status = SCB_OK;
    return 1;
}
