/**
 * @file scb_json.c
 * @brief 有界UTF-8 JSON解析及数字输出。
 * @details 前台共享八层显式容器栈，解析至调用方令牌/字符串池。
 *     writer只操作调用方缓冲并锁存溢出；不识别业务命令或访问硬件。
 */
#include "services/scb_json.h"
#include <string.h>
#include <float.h>

typedef char scb_float_assert_t[(sizeof(float) == 4 && FLT_RADIX == 2) ? 1 : -1];
/* 前台串行共享解析栈，不可重入；每次解析复用同一八层工作区。 */
static int16_t s_json_containers[SCB_JSON_DEPTH];
static uint8_t s_json_phases[SCB_JSON_DEPTH];

static int16_t SCB_ParseValue(scb_json_t *json, int16_t parent, uint8_t depth);

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
 * @brief 初始化静态有界输出。
 * @param[out] writer 非空写入状态。
 * @param[out] data 非空调用方缓冲，不自动写NUL。
 * @param[in] capacity 缓冲容量，1～992B。
 * @note 前台使用，JSON片段必须由可信常量或数字接口生成。
 */
void SCB_JsonBegin(scb_writer_t *writer, uint8_t *data, uint16_t capacity)
{
    writer->data = data;
    writer->capacity = capacity;
    writer->length = 0;
    writer->failed = !data || !capacity || capacity > SCB_PAYLOAD_MAX;
}

/**
 * @brief 追加完整可信JSON片段。
 * @details 先检查全部长度再复制，容量不足锁存失败，不能发送已生成的前缀。
 * @param[in,out] writer 非空已初始化状态。
 * @param[in] text 非空NUL结尾可信片段，调用方负责字符串转义。
 * @note 不分配堆，不截断，不写缓冲外字节。
 */
void SCB_JsonAppend(scb_writer_t *writer, const char *text)
{
    size_t length = strlen(text);
    if (writer->failed || length > (size_t)(writer->capacity - writer->length))
    {
        writer->failed = 1;
        return;
    }
    memcpy(writer->data + writer->length, text, length);
    writer->length += (uint16_t)length;
}

/**
 * @brief 写入完整十进制无符号整数。
 * @details 在11B局部缓冲中从末尾构造，不调用printf或堆分配。
 * @param[in,out] writer 非空写入状态。
 * @param[in] value 0～UINT32_MAX。
 */
void SCB_JsonU32(scb_writer_t *writer, uint32_t value)
{
    char digits[11];
    uint8_t position = 10;
    digits[position] = 0;
    do
    {
        digits[--position] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    SCB_JsonAppend(writer, digits + position);
}

/**
 * @brief 写入完整十进制有符号整数。
 * @details 负数通过无符号减法求幅值，INT32_MIN不发生有符号溢出。
 * @param[in,out] writer 非空写入状态。
 * @param[in] value 任意int32_t。
 */
void SCB_JsonI32(scb_writer_t *writer, int32_t value)
{
    if (value < 0)
    {
        SCB_JsonAppend(writer, "-");
    }
    SCB_JsonU32(writer, value < 0 ? 0u - (uint32_t)value : (uint32_t)value);
}

/**
 * @brief 写入九位有效数字的有限binary32。
 * @details 先归一到[1,10)，以double舍入九位十进制整数，再写科学计数法；
 *     九位有效数足以还原binary32。缩放次数由float指数界限限制，不走libc浮点格式化。
 * @param[in,out] writer 非空写入状态，非有限输入锁存失败。
 * @param[in] value 待公布的实际float值。
 * @note 前台调用，极小非零值也保留；零输出0，负零按零表示。
 */
void SCB_JsonFloat(scb_writer_t *writer, float value)
{
    double normalized = value;
    int32_t exponent = 0;
    uint32_t digits;
    char text[11];
    uint8_t i;
    if (!(value <= FLT_MAX && value >= -FLT_MAX))
    {
        writer->failed = 1;
        return;
    }
    if (value == 0)
    {
        SCB_JsonAppend(writer, "0");
        return;
    }
    if (normalized < 0)
    {
        SCB_JsonAppend(writer, "-");
        normalized = -normalized;
    }
    while (normalized >= 10)
    {
        normalized /= 10;
        ++exponent;
    }
    while (normalized < 1)
    {
        normalized *= 10;
        --exponent;
    }
    digits = (uint32_t)(normalized * 100000000.0 + 0.5);
    if (digits == 1000000000u)
    {
        digits = 100000000u;
        ++exponent;
    }
    text[10] = 0;
    for (i = 10; i > 0; --i)
    {
        if (i == 2)
        {
            text[1] = '.';
        }
        else
        {
            text[i - 1] = (char)('0' + digits % 10u);
            digits /= 10u;
        }
    }
    SCB_JsonAppend(writer, text);
    SCB_JsonAppend(writer, "e");
    SCB_JsonI32(writer, exponent);
}
