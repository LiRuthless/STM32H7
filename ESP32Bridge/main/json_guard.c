/**
 * @file json_guard.c
 * @brief 有界UTF-8 JSON校验与值视图。
 * @details codec和会话/订阅调用本模块；输入显式字节范围，输出错误或借用值视图。
 *     解析状态只在调用任务独占工作区和局部变量中，无递归、堆分配或共享可写状态。
 */
#include "json_guard.h"

enum
{
    ROOT_VALUE = 1, /**< 根值尚未消费。 */
    ROOT_END, /**< 根值已结束，之后只能空白。 */
    OBJECT_FIRST, /**< 对象首键或空对象结束。 */
    OBJECT_KEY, /**< 对象逗号后必须有键。 */
    OBJECT_COLON, /**< 等待键后的冒号。 */
    OBJECT_VALUE, /**< 等待对象值。 */
    OBJECT_COMMA, /**< 对象值后等待逗号或结束。 */
    ARRAY_FIRST, /**< 数组首值或空数组结束。 */
    ARRAY_VALUE, /**< 数组逗号后必须有值。 */
    ARRAY_COMMA /**< 数组值后等待逗号或结束。 */
};

/**
 * @brief 判断JSON允许的空白字节。
 * @details 只接受空格、Tab、CR和LF。
 * @param[in] c 当前输入字节。
 * @return 属于JSON空白返回true，否则false。
 */
static bool space(uint8_t c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/**
 * @brief 判断ASCII十进制数字。
 * @details 不依赖locale或字符分类库。
 * @param[in] c 当前输入字节。
 * @return 在0至9字符范围返回true。
 */
static bool digit(uint8_t c)
{
    return c >= '0' && c <= '9';
}

/**
 * @brief 将ASCII十六进制字符转换为数值。
 * @details 大小写A至F等价，不接受其他字符。
 * @param[in] c 输入字符字节。
 * @return 合法返回0至15，其他返回-1。
 */
static int hex(uint8_t c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    return -1;
}

/**
 * @brief 消费四位JSON转义十六进制数。
 * @details 依次前移游标并累积数值；遇坏字符时保留已经发生的游标/输出变化，调用方必须放弃该解析分支。
 * @param[in] b 非空输入，n字节可读。
 * @param[in] n 输入字节数。
 * @param[in,out] p 非空游标，进入时不大于n；不足四字节时不前移。
 * @param[out] value 非空；有足够字节时先清零，失败可能留下部分值。
 * @return 四位均合法返回true，否则false。
 */
static bool u16(const uint8_t *b, size_t n, size_t *p, uint32_t *value)
{
    if (n - *p < 4u)
    {
        return false;
    }
    *value = 0;
    for (unsigned i = 0; i < 4u; ++i)
    {
        int h = hex(b[(*p)++]);
        if (h < 0)
        {
            return false;
        }
        *value = (*value << 4) | (unsigned)h;
    }
    return true;
}

/* 按解码标量处理转义UTF-16代理对；键相等不包含Unicode规范化。 */
/**
 * @brief 消费一个JSON字符串标量。
 * @details 处理转义、UTF-16代理对和UTF-8，拒绝控制字符、过长编码和非法码点；按解码值比较，不做Unicode规范化。失败可已前移游标，调用方不再使用部分输出。
 * @param[in] b 非空输入，n字节可读。
 * @param[in] n 当前解析范围字节数。
 * @param[in,out] p 非空游标，进入时不大于n；逐字节消费。
 * @param[out] value 非空；成功写Unicode标量，失败不承诺输出值。
 * @return 成功消费一个标量返回true，截断或非法返回false。
 */
static bool scalar(const uint8_t *b, size_t n, size_t *p, uint32_t *value)
{
    if (*p >= n)
    {
        return false;
    }
    uint32_t c = b[(*p)++];
    if (c == '\\')
    {
        if (*p >= n)
        {
            return false;
        }
        c = b[(*p)++];
        switch (c)
        {
            case '"':
            case '\\':
            case '/':
                *value = c;
                return true;
            case 'b':
                *value = 8;
                return true;
            case 'f':
                *value = 12;
                return true;
            case 'n':
                *value = 10;
                return true;
            case 'r':
                *value = 13;
                return true;
            case 't':
                *value = 9;
                return true;
            case 'u':
                if (!u16(b, n, p, &c))
                {
                    return false;
                }
                if (c >= 0xd800 && c <= 0xdbff)
                {
                    uint32_t low;
                    if (n - *p < 6u || b[*p] != '\\' || b[*p + 1u] != 'u')
                    {
                        return false;
                    }
                    *p += 2u;
                    if (!u16(b, n, p, &low) || low < 0xdc00 || low > 0xdfff)
                    {
                        return false;
                    }
                    c = 0x10000u + ((c - 0xd800u) << 10) + low - 0xdc00u;
                }
                else if (c >= 0xdc00 && c <= 0xdfff)
                {
                    return false;
                }
                *value = c;
                return true;
            default:
                return false;
        }
    }
    if (c < 0x20u || c == '"')
    {
        return false;
    }
    if (c < 0x80u)
    {
        *value = c;
        return true;
    }
    unsigned remaining;
    uint32_t minimum;
    if (c >= 0xc2u && c <= 0xdfu)
    {
        remaining = 1;
        minimum = 0x80;
        c &= 0x1f;
    }
    else if (c >= 0xe0u && c <= 0xefu)
    {
        remaining = 2;
        minimum = 0x800;
        c &= 0xf;
    }
    else if (c >= 0xf0u && c <= 0xf4u)
    {
        remaining = 3;
        minimum = 0x10000;
        c &= 7;
    }
    else
    {
        return false;
    }
    if (n - *p < remaining)
    {
        return false;
    }
    while (remaining--)
    {
        uint8_t tail = b[(*p)++];
        if ((tail & 0xc0) != 0x80)
        {
            return false;
        }
        c = (c << 6) | (tail & 0x3f);
    }
    if (c < minimum || c > 0x10ffffu || (c >= 0xd800 && c <= 0xdfff))
    {
        return false;
    }
    *value = c;
    return true;
}

/**
 * @brief 跳过含转义的完整JSON字符串。
 * @details 调用方已定位到开引号；逐标量前进，闭引号被消费，坏标量或截断结束解析。
 * @param[in] b 非空输入，n字节可读。
 * @param[in] n 范围字节数。
 * @param[in,out] p 非空游标，进入时指向开引号；失败可部分前移。
 * @return 找到合法闭引号返回true，否则false。
 */
static bool string_end(const uint8_t *b, size_t n, size_t *p)
{
    ++*p;
    while (*p < n)
    {
        uint32_t ignored;
        if (b[*p] == '"')
        {
            ++*p;
            return true;
        }
        if (!scalar(b, n, p, &ignored))
        {
            return false;
        }
    }
    return false;
}

/**
 * @brief 按解码后的标量比较两个键。
 * @details 两个范围包含引号，比较时跳过首尾；不做Unicode规范化，避免转义写法绕过重复键检查。
 * @param[in] b 非空且覆盖两个完整键范围的输入。
 * @param[in] a 第一个键开引号偏移，字节。
 * @param[in] ae 第一个键闭引号后一字节偏移。
 * @param[in] c 第二个键开引号偏移。
 * @param[in] ce 第二个键闭引号后一字节偏移。
 * @return 两键标量序列完全一致返回true，否则false。
 */
static bool equal_keys(const uint8_t *b, size_t a, size_t ae, size_t c, size_t ce)
{
    ++a;
    ++c;
    --ae;
    --ce;
    while (a < ae && c < ce)
    {
        uint32_t x;
        uint32_t y;
        if (!scalar(b, ae, &a, &x) || !scalar(b, ce, &c, &y) || x != y)
        {
            return false;
        }
    }
    return a == ae && c == ce;
}

/* 只重扫对象此前的直接键，输入上限992字节；二次比较避免堆存储和递归栈增长。 */
/**
 * @brief 检查对象此前的直接键是否重复。
 * @details 只重扫同一对象较早的直接键，深层键跳过；输入受992字节上限约束，以二次扫描换取无堆、无递归的解析。
 * @param[in] b 非空已解析输入，覆盖end。
 * @param[in] start 当前对象左花括号偏移。
 * @param[in] key 当前键开引号偏移，此前字节已合法。
 * @param[in] end 当前键闭引号后一字节偏移。
 * @return 发现重复返回true；未发现或此前字符串解析异常返回false。
 */
static bool duplicate(const uint8_t *b, size_t start, size_t key, size_t end)
{
    size_t p = start + 1u;
    unsigned depth = 0;
    bool wants_key = true;
    while (p < key)
    {
        if (b[p] == '"')
        {
            size_t begin = p;
            if (!string_end(b, key, &p))
            {
                return false; /* 此前字节已由主解析路径验证。 */
            }
            if (depth == 0 && wants_key)
            {
                if (equal_keys(b, begin, p, key, end))
                {
                    return true;
                }
                wants_key = false;
            }
        }
        else
        {
            uint8_t c = b[p++];
            if (c == '{' || c == '[')
            {
                ++depth;
            }
            else if (c == '}' || c == ']')
            {
                --depth;
            }
            else if (c == ',' && depth == 0)
            {
                wants_key = true;
            }
        }
    }
    return false;
}

/**
 * @brief 消费JSON数值并拒绝binary64溢出。
 * @details 先验证符号/整数/小数/指数，再用十进制有效位与精确溢出边界比较；不转换车辆参数，允许有限的下溢零。失败可能已推进游标。
 * @param[in] b 非空输入，n字节可读，初始游标处是负号或数字。
 * @param[in] n 范围字节数。
 * @param[in,out] p 非空且初始小于n；成功指向数值后一字节，失败不回滚。
 * @return 合法有限数BRIDGE_JSON_OK，语法错误或溢出返回对应错误。
 */
static bridge_json_error_t number(const uint8_t *b, size_t n, size_t *p)
{
    size_t begin = *p;
    if (b[*p] == '-')
    {
        ++*p;
    }
    if (*p == n)
    {
        return BRIDGE_JSON_SYNTAX;
    }
    if (b[*p] == '0')
    {
        ++*p;
    }
    else
    {
        if (b[*p] < '1' || b[*p] > '9')
        {
            return BRIDGE_JSON_SYNTAX;
        }
        while (*p < n && digit(b[*p]))
        {
            ++*p;
        }
    }
    size_t fractional = 0;
    if (*p < n && b[*p] == '.')
    {
        ++*p;
        if (*p == n || !digit(b[*p]))
        {
            return BRIDGE_JSON_SYNTAX;
        }
        while (*p < n && digit(b[*p]))
        {
            ++*p;
            ++fractional;
        }
    }
    size_t mantissa_end = *p;
    int exponent = 0;
    int sign = 1;
    if (*p < n && (b[*p] == 'e' || b[*p] == 'E'))
    {
        ++*p;
        if (*p < n && (b[*p] == '+' || b[*p] == '-'))
        {
            if (b[*p] == '-')
            {
                sign = -1;
            }
            ++*p;
        }
        if (*p == n || !digit(b[*p]))
        {
            return BRIDGE_JSON_SYNTAX;
        }
        while (*p < n && digit(b[*p]))
        {
            if (exponent < 10000)
            {
                exponent = exponent * 10 + b[*p] - '0';
            }
            ++*p;
        }
    }
    /*
     * 只拒绝binary64溢出，不转换车辆参数。最近舍入溢出边界为2^1024 - 2^970，
     * 十进制规范有效位与该整数精确比较，相等也溢出。纯整数比较不依赖locale、
     * libc转换或隐式分配；下溢到带符号零仍为有限数。
     */
    static const char overflow[]
        = "179769313486231580793728971405303415079934132710037826936173778980444968292764750946649017977587207096330286416692887910946555547851940402630657488671505820681908902000708383676273854845817711531764475730270069855571366959622842914819860834936475292719074168444365510704342711559699508093042880177904174497792";
    _Static_assert(sizeof(overflow) == 310, "binary64 overflow boundary digits");
    size_t significant = 0;
    size_t first = mantissa_end;
    for (size_t i = begin; i < mantissa_end; ++i)
    {
        if (!digit(b[i]))
        {
            continue;
        }
        if (first == mantissa_end && b[i] != '0')
        {
            first = i;
        }
        if (first != mantissa_end)
        {
            ++significant;
        }
    }
    if (significant == 0)
    {
        return BRIDGE_JSON_OK;
    }
    int magnitude = (int)significant + sign * exponent - (int)fractional - 1;
    if (magnitude < 308)
    {
        return BRIDGE_JSON_OK;
    }
    if (magnitude > 308)
    {
        return BRIDGE_JSON_NONFINITE;
    }
    size_t cursor = first;
    for (size_t i = 0; i < 309; ++i)
    {
        while (cursor < mantissa_end && !digit(b[cursor]))
        {
            ++cursor;
        }
        uint8_t c = cursor < mantissa_end ? b[cursor++] : '0';
        if (c < (uint8_t)overflow[i])
        {
            return BRIDGE_JSON_OK;
        }
        if (c > (uint8_t)overflow[i])
        {
            return BRIDGE_JSON_NONFINITE;
        }
    }
    return BRIDGE_JSON_NONFINITE;
}

/**
 * @brief 消费固定JSON字面量。
 * @details 逐字符比较并推进；不检查后继分隔符，后继由外层状态机校验。
 * @param[in] b 非空输入，n字节可读。
 * @param[in] n 范围字节数。
 * @param[in,out] p 非空游标且不大于n；不匹配时可能已经前移。
 * @param[in] s 非空NUL结尾期望字面量，只读借用。
 * @return 完整匹配返回true，截断或不匹配返回false。
 */
static bool literal(const uint8_t *b, size_t n, size_t *p, const char *s)
{
    while (*s)
    {
        if (*p == n || b[(*p)++] != (uint8_t)*s++)
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief 验证完整有界UTF-8 JSON文本。
 * @details 用调用方工作区保存容器状态，拒绝重复键、非法数值、截断及尾随数据；无递归、堆分配或全局解析状态。工作区只是临时状态，不发布部分业务。
 * @param[in] b 非空且n字节可读的文本，无需尾NUL；调用期间不变。
 * @param[in] n 字节数，范围1至BRIDGE_JSON_MAX_BYTES。
 * @param[in,out] w 非空独占工作区；成功或失败均可能被改写。
 * @return 合法BRIDGE_JSON_OK，否则返回参数、语法、UTF-8、重复键或非有限数错误。
 */
bridge_json_error_t bridge_json_validate(const uint8_t *b, size_t n, bridge_json_workspace_t *w)
{
    if (!b || !w || n == 0 || n > BRIDGE_JSON_MAX_BYTES)
    {
        return BRIDGE_JSON_ARGUMENT;
    }
    size_t p = 0;
    size_t depth = 0;
    w->state[0] = ROOT_VALUE;
    while (true)
    {
        while (p < n && space(b[p]))
        {
            ++p;
        }
        uint8_t state = w->state[depth];
        if (state == ROOT_END)
        {
            return p == n ? BRIDGE_JSON_OK : BRIDGE_JSON_SYNTAX;
        }
        if (p == n)
        {
            return BRIDGE_JSON_SYNTAX;
        }
        uint8_t c = b[p];
        if (state == OBJECT_FIRST || state == OBJECT_KEY)
        {
            if (state == OBJECT_FIRST && c == '}')
            {
                ++p;
                --depth;
                continue;
            }
            if (c != '"')
            {
                return BRIDGE_JSON_SYNTAX;
            }
            size_t begin = p;
            if (!string_end(b, n, &p))
            {
                return BRIDGE_JSON_UTF8;
            }
            if (duplicate(b, w->object_start[depth], begin, p))
            {
                return BRIDGE_JSON_DUPLICATE_KEY;
            }
            w->state[depth] = OBJECT_COLON;
            continue;
        }
        if (state == OBJECT_COLON)
        {
            if (c != ':')
            {
                return BRIDGE_JSON_SYNTAX;
            }
            ++p;
            w->state[depth] = OBJECT_VALUE;
            continue;
        }
        if (state == OBJECT_COMMA || state == ARRAY_COMMA)
        {
            if (c == (state == OBJECT_COMMA ? '}' : ']'))
            {
                ++p;
                --depth;
                continue;
            }
            if (c != ',')
            {
                return BRIDGE_JSON_SYNTAX;
            }
            ++p;
            w->state[depth] = state == OBJECT_COMMA ? OBJECT_KEY : ARRAY_VALUE;
            continue;
        }
        if (state == ARRAY_FIRST && c == ']')
        {
            ++p;
            --depth;
            continue;
        }
        w->state[depth] = state == ROOT_VALUE ? ROOT_END
            : state == OBJECT_VALUE           ? OBJECT_COMMA
                                              : ARRAY_COMMA;
        if (c == '{' || c == '[')
        {
            if (depth == BRIDGE_JSON_MAX_BYTES / 2u)
            {
                return BRIDGE_JSON_SYNTAX;
            }
            ++depth;
            w->object_start[depth] = (uint16_t)p++;
            w->state[depth] = c == '{' ? OBJECT_FIRST : ARRAY_FIRST;
        }
        else if (c == '"')
        {
            if (!string_end(b, n, &p))
            {
                return BRIDGE_JSON_UTF8;
            }
        }
        else if (c == '-' || digit(c))
        {
            bridge_json_error_t error = number(b, n, &p);
            if (error != BRIDGE_JSON_OK)
            {
                return error;
            }
        }
        else if (c == 't')
        {
            if (!literal(b, n, &p, "true"))
            {
                return BRIDGE_JSON_SYNTAX;
            }
        }
        else if (c == 'f')
        {
            if (!literal(b, n, &p, "false"))
            {
                return BRIDGE_JSON_SYNTAX;
            }
        }
        else if (c == 'n')
        {
            if (!literal(b, n, &p, "null"))
            {
                return BRIDGE_JSON_SYNTAX;
            }
        }
        else
        {
            return BRIDGE_JSON_SYNTAX;
        }
    }
}

/**
 * @brief 验证对象并查找顶层键的原始值视图。
 * @details 先清空有效view并验证全量JSON，再按解码键匹配ASCII名称；输出借用输入而非工作区，不附加尾NUL。
 * @param[in] b 非空有界JSON文本，返回视图使用期间保持有效。
 * @param[in] n 字节数，交由全量校验器检查。
 * @param[in] name 非空NUL结尾ASCII键名。
 * @param[in,out] w 非空且独占的解析工作区。
 * @param[out] view 非空；进入时清空，找到时写原始值范围，失败保持空。
 * @return 找到匹配顶层键返回true，非法文本、非对象或缺键返回false。
 */
bool bridge_json_object_get(const uint8_t *b, size_t n, const char *name,
    bridge_json_workspace_t *w, bridge_json_view_t *view)
{
    if (!view)
    {
        return false;
    }
    view->bytes = NULL;
    view->length = 0;
    if (!name || bridge_json_validate(b, n, w) != BRIDGE_JSON_OK)
    {
        return false;
    }
    size_t p = 0;
    while (p < n && space(b[p]))
    {
        ++p;
    }
    if (b[p++] != '{')
    {
        return false;
    }
    while (p < n)
    {
        while (p < n && space(b[p]))
        {
            ++p;
        }
        if (b[p] == '}')
        {
            return false;
        }
        size_t key = ++p;
        size_t name_index = 0;
        bool match = true;
        --p;
        (void)string_end(b, n, &p);
        size_t key_end = p - 1u;
        while (key < key_end)
        {
            uint32_t c;
            (void)scalar(b, key_end, &key, &c);
            if (!name[name_index] || c != (uint8_t)name[name_index])
            {
                match = false;
            }
            if (name[name_index])
            {
                ++name_index;
            }
        }
        if (name[name_index])
        {
            match = false;
        }
        while (space(b[p]))
        {
            ++p;
        }
        ++p; /* 冒号已在全量校验时验证。 */
        while (space(b[p]))
        {
            ++p;
        }
        size_t begin = p;
        size_t nesting = 0;
        while (p < n)
        {
            uint8_t c = b[p];
            if (c == '"')
            {
                (void)string_end(b, n, &p);
            }
            else if (c == '{' || c == '[')
            {
                ++nesting;
                ++p;
            }
            else if (c == '}' || c == ']')
            {
                if (nesting == 0)
                {
                    break;
                }
                --nesting;
                ++p;
            }
            else if (c == ',' && nesting == 0)
            {
                break;
            }
            else
            {
                ++p;
            }
        }
        size_t end = p;
        while (end > begin && space(b[end - 1u]))
        {
            --end;
        }
        if (match)
        {
            view->bytes = b + begin;
            view->length = end - begin;
            return true;
        }
        if (b[p++] == '}')
        {
            return false;
        }
    }
    return false;
}

/**
 * @brief 将已验证的JSON字符串视图与ASCII文本比较。
 * @details 逐标量解码转义后精确比较；不分配内存，不改变输入。
 * @param[in] v 来自已验证JSON的有界字符串视图；空或缺引号拒绝。
 * @param[in] ascii 非空NUL结尾ASCII文本，只读借用。
 * @return 内容完全相等返回true，否则false。
 */
bool bridge_json_string_equal(bridge_json_view_t v, const char *ascii)
{
    if (!v.bytes || !ascii || v.length < 2u || v.bytes[0] != '"' || v.bytes[v.length - 1u] != '"')
    {
        return false;
    }
    size_t p = 1;
    size_t i = 0;
    while (p < v.length - 1u)
    {
        uint32_t c;
        if (!scalar(v.bytes, v.length - 1u, &p, &c) || !ascii[i] || c != (uint8_t)ascii[i++])
        {
            return false;
        }
    }
    return p == v.length - 1u && !ascii[i];
}

/**
 * @brief 将已验证的JSON数值视图转换为精确无符号整数。
 * @details 接受小数/指数表示，但仅在结果精确、非负且不超过UINT32_MAX时发布；不做舍入。失败不发布累积值，输出仍为零。
 * @param[in] v 有界数值视图，字节可读且调用期间不变；拒绝负号和非法数值。
 * @param[out] out 非空；进入即清零，成功后写精确整数。
 * @return 转换成功返回true，否则false。
 */
bool bridge_json_u32(bridge_json_view_t v, uint32_t *out)
{
    if (!out)
    {
        return false;
    }
    *out = 0;
    if (!v.bytes || !v.length || v.length > BRIDGE_JSON_MAX_BYTES || v.bytes[0] == '-')
    {
        return false;
    }
    size_t p = 0;
    if (number(v.bytes, v.length, &p) != BRIDGE_JSON_OK || p != v.length)
    {
        return false;
    }
    size_t end = 0;
    size_t fractional = 0;
    size_t trailing = 0;
    bool fraction = false;
    while (end < v.length && v.bytes[end] != 'e' && v.bytes[end] != 'E')
    {
        uint8_t c = v.bytes[end++];
        if (c == '.')
        {
            fraction = true;
            continue;
        }
        if (fraction)
        {
            ++fractional;
        }
        trailing = c == '0' ? trailing + 1u : 0;
    }
    int exponent = 0;
    int sign = 1;
    p = end;
    if (p < v.length)
    {
        ++p;
        if (v.bytes[p] == '+' || v.bytes[p] == '-')
        {
            if (v.bytes[p] == '-')
            {
                sign = -1;
            }
            ++p;
        }
        while (p < v.length)
        {
            if (exponent < 10000)
            {
                exponent = exponent * 10 + v.bytes[p] - '0';
            }
            ++p;
        }
    }
    size_t significant_end = end;
    while (trailing)
    {
        --significant_end;
        if (v.bytes[significant_end] != '.')
        {
            --trailing;
        }
    }
    int power = sign * exponent - (int)fractional;
    for (size_t i = significant_end; i < end; ++i)
    {
        if (v.bytes[i] != '.')
        {
            ++power;
        }
    }
    uint32_t value = 0;
    for (size_t i = 0; i < significant_end; ++i)
    {
        if (v.bytes[i] == '.')
        {
            continue;
        }
        uint32_t d = v.bytes[i] - '0';
        if (value > (UINT32_MAX - d) / 10u)
        {
            return false;
        }
        value = value * 10u + d;
    }
    if (!value)
    {
        return true;
    }
    if (power < 0 || power > 9)
    {
        return false;
    }
    while (power--)
    {
        if (value > UINT32_MAX / 10u)
        {
            return false;
        }
        value *= 10u;
    }
    *out = value;
    return true;
}
