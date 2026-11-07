/**
 * @file json_guard.h
 * @brief 有界UTF-8 JSON校验与值视图。
 * @details 声明有界UTF-8 JSON校验与值视图接口；实现及完整参数/错误语义位于json_guard.c。
 *     调用方按接口约束串行访问owner状态，头文件不定义线图像或运行期服务。
 *
 * @note Phase02 G4：已验证字段查询不重复验证全文；调用者保证原输入有效且不变。
 */
#ifndef BRIDGE_JSON_GUARD_H
#define BRIDGE_JSON_GUARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BRIDGE_JSON_MAX_BYTES 992u

/*
 * 每个容器至少占两字节，工作区覆盖合法负载；调用任务独占借用，
 * 不递归、不分配堆、不发布工作区指针。
 */
typedef struct
{
    uint16_t object_start[BRIDGE_JSON_MAX_BYTES / 2u
        + 1u]; /**< 各深度对象起始字节偏移，仅本次调用任务访问。 */
    uint8_t state[BRIDGE_JSON_MAX_BYTES / 2u
        + 1u]; /**< 各容器深度的解析阶段；与object_start共同覆盖有界输入。 */
} bridge_json_workspace_t;

typedef enum
{
    BRIDGE_JSON_OK = 0, /**< 全量校验或数值解析成功。 */
    BRIDGE_JSON_ARGUMENT, /**< 指针或输入长度无效。 */
    BRIDGE_JSON_SYNTAX, /**< JSON语法或尾随数据错误。 */
    BRIDGE_JSON_UTF8, /**< 字符串标量或转义校验失败。 */
    BRIDGE_JSON_DUPLICATE_KEY, /**< 解码后重复键。 */
    BRIDGE_JSON_NONFINITE /**< 历史有限性错误编号，保留兼容；桥接语法校验不再产生。 */
} bridge_json_error_t;

typedef struct
{
    const uint8_t *bytes; /**< 借用原始输入的值起点，不是工作区存储，可空表示无值。 */
    size_t length; /**< 原始值字节数，不含外部空白，不保证NUL结尾。 */
} bridge_json_view_t;

/**
 * @brief 验证完整有界UTF-8 JSON文本。
 * @note json_guard.c::bridge_json_validate；工作区独占，返回视图借用原输入。
 */
bridge_json_error_t bridge_json_validate(
    const uint8_t *bytes, size_t length, bridge_json_workspace_t *workspace);
/**
 * @brief 验证对象并查找顶层键的原始值视图。
 * @note json_guard.c::bridge_json_object_get；工作区独占，返回视图借用原输入。
 */
bool bridge_json_object_get(const uint8_t *bytes, size_t length, const char *name,
    bridge_json_workspace_t *workspace, bridge_json_view_t *view);

/**
 * @brief 将已验证的JSON字符串视图与ASCII文本比较。
 * @note json_guard.c::bridge_json_string_equal；工作区独占，返回视图借用原输入。
 */
bool bridge_json_string_equal(bridge_json_view_t view, const char *ascii);
/**
 * @brief 将已验证的JSON数值视图转换为精确无符号整数。
 * @note json_guard.c::bridge_json_u32；工作区独占，返回视图借用原输入。
 */
bool bridge_json_u32(bridge_json_view_t view, uint32_t *value);

/**
 * @brief 查询已验证且不变的JSON对象，不重复验证全文。
 * @note json_guard.c::bridge_json_object_get_validated；仅用于已验证根或子对象。
 */
bool bridge_json_object_get_validated(
    bridge_json_view_t object, const char *name, bridge_json_view_t *view);

#endif
