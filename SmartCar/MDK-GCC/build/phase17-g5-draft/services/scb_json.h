/**
 * @file scb_json.h
 * @brief 有界JSON令牌和输出接口。
 * @details scb_json.c实现UTF-8/数字/容器校验与有界writer；解析共享栈不可重入。
 */
#ifndef SMARTCAR_APP_SERVICES_SCB_JSON_H
#define SMARTCAR_APP_SERVICES_SCB_JSON_H
#include "services/scb_frame.h"

#define SCB_JSON_TOKENS 128u
#define SCB_JSON_DEPTH 8u

typedef enum
{
    SCB_JSON_OBJECT, /**< JSON 对象。 */
    SCB_JSON_ARRAY, /**< JSON 数组。 */
    SCB_JSON_STRING, /**< 解码后的 UTF-8 字符串。 */
    SCB_JSON_NUMBER, /**< 有限 double 数值。 */
    SCB_JSON_TRUE, /**< 真值。 */
    SCB_JSON_FALSE, /**< 假值。 */
    SCB_JSON_NULL /**< 空值。 */
} scb_json_type_t;

typedef struct
{
    scb_json_type_t type; /**< 令牌类型。 */
    int16_t parent; /**< 所属容器，根为-1。 */
    uint16_t text; /**< 字符串池起点。 */
    uint16_t length; /**< 解码字符串字节数。 */
    uint8_t key; /**< 本令牌是否为对象键。 */
    uint8_t integer; /**< 原文是否不含小数和指数。 */
    double number; /**< 数字转换结果。 */
} scb_token_t;

typedef struct
{
    scb_token_t tokens[SCB_JSON_TOKENS]; /**< 固定令牌区，前台独占。 */
    char strings[SCB_PAYLOAD_MAX + 1u]; /**< 解码文本池。 */
    uint16_t count; /**< 已分配令牌数。 */
    uint16_t used; /**< 字符串池占用。 */
    uint16_t position; /**< 解析字节位置。 */
    uint16_t length; /**< 输入总长。 */
    const uint8_t *input; /**< 仅解析期间借用的输入。 */
} scb_json_t;

typedef struct
{
    uint8_t *data; /**< 调用方静态目标。 */
    uint16_t capacity; /**< 可写字节数，不含隐式终止符。 */
    uint16_t length; /**< 当前完整输出长度。 */
    uint8_t failed; /**< 任何溢出后整条结果不可发送。 */
} scb_writer_t;

/**
 * @brief 校验并解析完整JSON文档。
 * @note 完整契约见 scb_json.c::SCB_ParseJson。
 */
uint8_t SCB_ParseJson(const uint8_t *input, uint16_t length, scb_json_t *json);

/**
 * @brief 查找对象的直接成员值。
 * @note 完整契约见 scb_json.c::SCB_FindMember。
 */
int16_t SCB_FindMember(const scb_json_t *json, int16_t object, const char *key);

/**
 * @brief 取得已解码字符串。
 * @note 完整契约见 scb_json.c::SCB_GetString。
 */
const char *SCB_GetString(const scb_json_t *json, int16_t token);

/**
 * @brief 复制已校验有限数字。
 * @note 完整契约见 scb_json.c::SCB_GetNumber。
 */
uint8_t SCB_GetNumber(const scb_json_t *json, int16_t token, double *value);

/**
 * @brief 初始化静态有界输出。
 * @note 完整契约见 scb_json.c::SCB_JsonBegin。
 */
void SCB_JsonBegin(scb_writer_t *writer, uint8_t *data, uint16_t capacity);

/**
 * @brief 追加完整可信JSON片段。
 * @note 完整契约见 scb_json.c::SCB_JsonAppend。
 */
void SCB_JsonAppend(scb_writer_t *writer, const char *text);

/**
 * @brief 写入完整十进制无符号整数。
 * @note 完整契约见 scb_json.c::SCB_JsonU32。
 */
void SCB_JsonU32(scb_writer_t *writer, uint32_t value);

/**
 * @brief 写入完整十进制有符号整数。
 * @note 完整契约见 scb_json.c::SCB_JsonI32。
 */
void SCB_JsonI32(scb_writer_t *writer, int32_t value);

/**
 * @brief 写入九位有效数字的有限binary32。
 * @note 完整契约见 scb_json.c::SCB_JsonFloat。
 */
void SCB_JsonFloat(scb_writer_t *writer, float value);

#endif /* SMARTCAR_APP_SERVICES_SCB_JSON_H */
