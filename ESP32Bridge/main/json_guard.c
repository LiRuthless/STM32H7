#include "json_guard.h"

enum { ROOT_VALUE = 1, ROOT_END, OBJECT_FIRST, OBJECT_KEY, OBJECT_COLON,
       OBJECT_VALUE, OBJECT_COMMA, ARRAY_FIRST, ARRAY_VALUE, ARRAY_COMMA };

static bool space(uint8_t c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
static bool digit(uint8_t c) { return c >= '0' && c <= '9'; }
static int hex(uint8_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
static bool u16(const uint8_t *b, size_t n, size_t *p, uint32_t *value)
{
    if (n - *p < 4u) return false;
    *value = 0;
    for (unsigned i = 0; i < 4u; ++i) {
        int h = hex(b[(*p)++]);
        if (h < 0) return false;
        *value = (*value << 4) | (unsigned)h;
    }
    return true;
}
/* Decode one scalar, including escaped UTF-16 pairs, without normalizing keys.
 * JSON equality uses decoded scalars, not Unicode canonical normalization. */
static bool scalar(const uint8_t *b, size_t n, size_t *p, uint32_t *value)
{
    if (*p >= n) return false;
    uint32_t c = b[(*p)++];
    if (c == '\\') {
        if (*p >= n) return false;
        c = b[(*p)++];
        switch (c) {
        case '"': case '\\': case '/': *value = c; return true;
        case 'b': *value = 8; return true;
        case 'f': *value = 12; return true;
        case 'n': *value = 10; return true;
        case 'r': *value = 13; return true;
        case 't': *value = 9; return true;
        case 'u':
            if (!u16(b, n, p, &c)) return false;
            if (c >= 0xd800 && c <= 0xdbff) {
                uint32_t low;
                if (n - *p < 6u || b[*p] != '\\' || b[*p + 1u] != 'u') return false;
                *p += 2u;
                if (!u16(b, n, p, &low) || low < 0xdc00 || low > 0xdfff) return false;
                c = 0x10000u + ((c - 0xd800u) << 10) + low - 0xdc00u;
            } else if (c >= 0xdc00 && c <= 0xdfff) return false;
            *value = c; return true;
        default: return false;
        }
    }
    if (c < 0x20u || c == '"') return false;
    if (c < 0x80u) { *value = c; return true; }
    unsigned remaining;
    uint32_t minimum;
    if (c >= 0xc2u && c <= 0xdfu) { remaining = 1; minimum = 0x80; c &= 0x1f; }
    else if (c >= 0xe0u && c <= 0xefu) { remaining = 2; minimum = 0x800; c &= 0xf; }
    else if (c >= 0xf0u && c <= 0xf4u) { remaining = 3; minimum = 0x10000; c &= 7; }
    else return false;
    if (n - *p < remaining) return false;
    while (remaining--) {
        uint8_t tail = b[(*p)++];
        if ((tail & 0xc0) != 0x80) return false;
        c = (c << 6) | (tail & 0x3f);
    }
    if (c < minimum || c > 0x10ffffu || (c >= 0xd800 && c <= 0xdfff)) return false;
    *value = c; return true;
}
static bool string_end(const uint8_t *b, size_t n, size_t *p)
{
    ++*p;
    while (*p < n) {
        uint32_t ignored;
        if (b[*p] == '"') { ++*p; return true; }
        if (!scalar(b, n, p, &ignored)) return false;
    }
    return false;
}
static bool equal_keys(const uint8_t *b, size_t a, size_t ae, size_t c, size_t ce)
{
    ++a; ++c; --ae; --ce;
    while (a < ae && c < ce) {
        uint32_t x, y;
        if (!scalar(b, ae, &a, &x) || !scalar(b, ce, &c, &y) || x != y) return false;
    }
    return a == ae && c == ce;
}
/* Rescan only earlier direct keys of this object. Bounded by 992 bytes;
 * O(n^2) comparisons avoid heap storage and recursive parser stack growth. */
static bool duplicate(const uint8_t *b, size_t start, size_t key, size_t end)
{
    size_t p = start + 1u;
    unsigned depth = 0;
    bool wants_key = true;
    while (p < key) {
        if (b[p] == '"') {
            size_t begin = p;
            if (!string_end(b, key, &p)) return false; /* earlier bytes already valid */
            if (depth == 0 && wants_key) {
                if (equal_keys(b, begin, p, key, end)) return true;
                wants_key = false;
            }
        } else {
            uint8_t c = b[p++];
            if (c == '{' || c == '[') ++depth;
            else if (c == '}' || c == ']') --depth;
            else if (c == ',' && depth == 0) wants_key = true;
        }
    }
    return false;
}
static bridge_json_error_t number(const uint8_t *b, size_t n, size_t *p)
{
    size_t begin = *p;
    if (b[*p] == '-') ++*p;
    if (*p == n) return BRIDGE_JSON_SYNTAX;
    if (b[*p] == '0') ++*p;
    else {
        if (b[*p] < '1' || b[*p] > '9') return BRIDGE_JSON_SYNTAX;
        while (*p < n && digit(b[*p])) ++*p;
    }
    size_t fractional = 0;
    if (*p < n && b[*p] == '.') {
        ++*p;
        if (*p == n || !digit(b[*p])) return BRIDGE_JSON_SYNTAX;
        while (*p < n && digit(b[*p])) { ++*p; ++fractional; }
    }
    size_t mantissa_end = *p;
    int exponent = 0, sign = 1;
    if (*p < n && (b[*p] == 'e' || b[*p] == 'E')) {
        ++*p;
        if (*p < n && (b[*p] == '+' || b[*p] == '-')) {
            if (b[*p] == '-') sign = -1;
            ++*p;
        }
        if (*p == n || !digit(b[*p])) return BRIDGE_JSON_SYNTAX;
        while (*p < n && digit(b[*p])) {
            if (exponent < 10000) exponent = exponent * 10 + b[*p] - '0';
            ++*p;
        }
    }
    /* Only need to reject binary64 overflow, not convert vehicle parameters.
     * The round-to-nearest overflow boundary is exactly 2^1024 - 2^970.
     * Compare normalized decimal digits to that integer; ties overflow.
     * Integer-only comparison avoids libc conversion, locale, recursion and
     * potential hidden allocations. Underflow to signed zero is finite. */
    static const char overflow[] = "179769313486231580793728971405303415079934132710037826936173778980444968292764750946649017977587207096330286416692887910946555547851940402630657488671505820681908902000708383676273854845817711531764475730270069855571366959622842914819860834936475292719074168444365510704342711559699508093042880177904174497792";
    _Static_assert(sizeof(overflow) == 310, "binary64 overflow boundary digits");
    size_t significant = 0, first = mantissa_end;
    for (size_t i = begin; i < mantissa_end; ++i) {
        if (!digit(b[i])) continue;
        if (first == mantissa_end && b[i] != '0') first = i;
        if (first != mantissa_end) ++significant;
    }
    if (significant == 0) return BRIDGE_JSON_OK;
    int magnitude = (int)significant + sign * exponent - (int)fractional - 1;
    if (magnitude < 308) return BRIDGE_JSON_OK;
    if (magnitude > 308) return BRIDGE_JSON_NONFINITE;
    size_t cursor = first;
    for (size_t i = 0; i < 309; ++i) {
        while (cursor < mantissa_end && !digit(b[cursor])) ++cursor;
        uint8_t c = cursor < mantissa_end ? b[cursor++] : '0';
        if (c < (uint8_t)overflow[i]) return BRIDGE_JSON_OK;
        if (c > (uint8_t)overflow[i]) return BRIDGE_JSON_NONFINITE;
    }
    return BRIDGE_JSON_NONFINITE;
}
static bool literal(const uint8_t *b, size_t n, size_t *p, const char *s)
{
    while (*s) {
        if (*p == n || b[(*p)++] != (uint8_t)*s++) return false;
    }
    return true;
}
bridge_json_error_t bridge_json_validate(const uint8_t *b, size_t n, bridge_json_workspace_t *w)
{
    if (!b || !w || n == 0 || n > BRIDGE_JSON_MAX_BYTES) return BRIDGE_JSON_ARGUMENT;
    size_t p = 0, depth = 0;
    w->state[0] = ROOT_VALUE;
    while (true) {
        while (p < n && space(b[p])) ++p;
        uint8_t state = w->state[depth];
        if (state == ROOT_END) return p == n ? BRIDGE_JSON_OK : BRIDGE_JSON_SYNTAX;
        if (p == n) return BRIDGE_JSON_SYNTAX;
        uint8_t c = b[p];
        if (state == OBJECT_FIRST || state == OBJECT_KEY) {
            if (state == OBJECT_FIRST && c == '}') { ++p; --depth; continue; }
            if (c != '"') return BRIDGE_JSON_SYNTAX;
            size_t begin = p;
            if (!string_end(b, n, &p)) return BRIDGE_JSON_UTF8;
            if (duplicate(b, w->object_start[depth], begin, p)) return BRIDGE_JSON_DUPLICATE_KEY;
            w->state[depth] = OBJECT_COLON; continue;
        }
        if (state == OBJECT_COLON) {
            if (c != ':') return BRIDGE_JSON_SYNTAX;
            ++p; w->state[depth] = OBJECT_VALUE; continue;
        }
        if (state == OBJECT_COMMA || state == ARRAY_COMMA) {
            if (c == (state == OBJECT_COMMA ? '}' : ']')) { ++p; --depth; continue; }
            if (c != ',') return BRIDGE_JSON_SYNTAX;
            ++p; w->state[depth] = state == OBJECT_COMMA ? OBJECT_KEY : ARRAY_VALUE; continue;
        }
        if (state == ARRAY_FIRST && c == ']') { ++p; --depth; continue; }
        w->state[depth] = state == ROOT_VALUE ? ROOT_END :
                           state == OBJECT_VALUE ? OBJECT_COMMA : ARRAY_COMMA;
        if (c == '{' || c == '[') {
            if (depth == BRIDGE_JSON_MAX_BYTES / 2u) return BRIDGE_JSON_SYNTAX;
            ++depth; w->object_start[depth] = (uint16_t)p++;
            w->state[depth] = c == '{' ? OBJECT_FIRST : ARRAY_FIRST;
        } else if (c == '"') {
            if (!string_end(b, n, &p)) return BRIDGE_JSON_UTF8;
        } else if (c == '-' || digit(c)) {
            bridge_json_error_t error = number(b, n, &p);
            if (error != BRIDGE_JSON_OK) return error;
        } else if (c == 't') { if (!literal(b, n, &p, "true")) return BRIDGE_JSON_SYNTAX; }
        else if (c == 'f') { if (!literal(b, n, &p, "false")) return BRIDGE_JSON_SYNTAX; }
        else if (c == 'n') { if (!literal(b, n, &p, "null")) return BRIDGE_JSON_SYNTAX; }
        else return BRIDGE_JSON_SYNTAX;
    }
}
bool bridge_json_object_get(const uint8_t *b, size_t n, const char *name,
                            bridge_json_workspace_t *w, bridge_json_view_t *view)
{
    if (!view) return false;
    view->bytes = NULL; view->length = 0;
    if (!name || bridge_json_validate(b, n, w) != BRIDGE_JSON_OK) return false;
    size_t p = 0;
    while (p < n && space(b[p])) ++p;
    if (b[p++] != '{') return false;
    while (p < n) {
        while (p < n && space(b[p])) ++p;
        if (b[p] == '}') return false;
        size_t key = ++p, name_index = 0;
        bool match = true;
        --p; (void)string_end(b, n, &p);
        size_t key_end = p - 1u;
        while (key < key_end) {
            uint32_t c;
            (void)scalar(b, key_end, &key, &c);
            if (!name[name_index] || c != (uint8_t)name[name_index]) match = false;
            if (name[name_index]) ++name_index;
        }
        if (name[name_index]) match = false;
        while (space(b[p])) ++p;
        ++p; /* validated colon */
        while (space(b[p])) ++p;
        size_t begin = p, nesting = 0;
        while (p < n) {
            uint8_t c = b[p];
            if (c == '"') (void)string_end(b, n, &p);
            else if (c == '{' || c == '[') { ++nesting; ++p; }
            else if (c == '}' || c == ']') { if (nesting == 0) break; --nesting; ++p; }
            else if (c == ',' && nesting == 0) break;
            else ++p;
        }
        size_t end = p;
        while (end > begin && space(b[end - 1u])) --end;
        if (match) { view->bytes = b + begin; view->length = end - begin; return true; }
        if (b[p++] == '}') return false;
    }
    return false;
}

bool bridge_json_string_equal(bridge_json_view_t v, const char *ascii)
{
    if (!v.bytes || !ascii || v.length < 2u || v.bytes[0] != '"' || v.bytes[v.length - 1u] != '"') return false;
    size_t p = 1, i = 0;
    while (p < v.length - 1u) {
        uint32_t c;
        if (!scalar(v.bytes, v.length - 1u, &p, &c) || !ascii[i] || c != (uint8_t)ascii[i++]) return false;
    }
    return p == v.length - 1u && !ascii[i];
}

bool bridge_json_u32(bridge_json_view_t v, uint32_t *out)
{
    if (!out) return false;
    *out = 0;
    if (!v.bytes || !v.length || v.length > BRIDGE_JSON_MAX_BYTES || v.bytes[0] == '-') return false;
    size_t p = 0;
    if (number(v.bytes, v.length, &p) != BRIDGE_JSON_OK || p != v.length) return false;
    size_t end = 0, fractional = 0, trailing = 0;
    bool fraction = false;
    while (end < v.length && v.bytes[end] != 'e' && v.bytes[end] != 'E') {
        uint8_t c = v.bytes[end++];
        if (c == '.') { fraction = true; continue; }
        if (fraction) ++fractional;
        trailing = c == '0' ? trailing + 1u : 0;
    }
    int exponent = 0, sign = 1;
    p = end;
    if (p < v.length) {
        ++p;
        if (v.bytes[p] == '+' || v.bytes[p] == '-') { if (v.bytes[p] == '-') sign = -1; ++p; }
        while (p < v.length) {
            if (exponent < 10000) exponent = exponent * 10 + v.bytes[p] - '0';
            ++p;
        }
    }
    size_t significant_end = end;
    while (trailing) {
        --significant_end;
        if (v.bytes[significant_end] != '.') --trailing;
    }
    int power = sign * exponent - (int)fractional;
    for (size_t i = significant_end; i < end; ++i) if (v.bytes[i] != '.') ++power;
    uint32_t value = 0;
    for (size_t i = 0; i < significant_end; ++i) {
        if (v.bytes[i] == '.') continue;
        uint32_t d = v.bytes[i] - '0';
        if (value > (UINT32_MAX - d) / 10u) return false;
        value = value * 10u + d;
    }
    if (!value) return true;
    if (power < 0 || power > 9) return false;
    while (power--) { if (value > UINT32_MAX / 10u) return false; value *= 10u; }
    *out = value;
    return true;
}
