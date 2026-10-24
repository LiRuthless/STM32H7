/**
 * @file scb_command_codec.c
 * @brief SCB业务命令与参数格式适配。
 * @details 前台复用唯一命令JSON工作区，将协议字段转换为业务值。
 *     校验调用纯业务validator，命令路由/结果缓存仍由dispatch/link持有。
 */
#include "services/scb_command_codec.h"
#include "control/tuning_values.h"
#include "app_config.h"
#include <string.h>
#include <float.h>

/* 命令解析由前台串行调用，避免大JSON工作区占栈。 */
static scb_json_t s_command_json;

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
 * @brief 输出完整八项参数对象。
 * @details 显式按公共字段顺序格式化，整数保留符号，输出实际float，不依赖结构体布局。
 * @param[in,out] writer 非空写入状态。
 * @param[in] values 非空参数快照，前台开始格式化前已经固定。
 */
void SCB_JsonValues(scb_writer_t *writer, const scb_values_t *values)
{
    SCB_JsonAppend(writer, "{\"kp\":");
    SCB_JsonFloat(writer, values->kp);
    SCB_JsonAppend(writer, ",\"k2p\":");
    SCB_JsonFloat(writer, values->k2p);
    SCB_JsonAppend(writer, ",\"kd\":");
    SCB_JsonFloat(writer, values->kd);
    SCB_JsonAppend(writer, ",\"kp_v\":");
    SCB_JsonFloat(writer, values->kp_v);
    SCB_JsonAppend(writer, ",\"ki_v\":");
    SCB_JsonFloat(writer, values->ki_v);
    SCB_JsonAppend(writer, ",\"base_speed\":");
    SCB_JsonI32(writer, values->base_speed);
    SCB_JsonAppend(writer, ",\"fan_run\":");
    SCB_JsonI32(writer, values->fan_run);
    SCB_JsonAppend(writer, ",\"fan_idle\":");
    SCB_JsonI32(writer, values->fan_idle);
    SCB_JsonAppend(writer, "}");
}

/**
 * @brief 将完整JSON解析成命令快照。
 * @details 先写INVALID_ARG并清command，再验证整份JSON，随后检查必需字段类型及同源限值，
 *     整数在窄化前检查范围。未知命令是业务INVALID_ARG，不执行任何部分请求。
 * @param[in] input 非空命令字节。
 * @param[in] length 负载字节数。
 * @param[out] command 非空命令，入口清零；失败可有部分字段，只有status为OK时可执行。
 * @param[out] status 非空业务结果，入口INVALID_ARG；语法错误返回0且不可ACK。
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
            if (value < (i < 5 ? TUNING_GAIN_MIN
                : (i == 5 ? TUNING_SPEED_MIN : TUNING_FAN_MIN))
                || value > (i < 5 ? TUNING_GAIN_MAX
                : (i == 5 ? TUNING_SPEED_MAX : TUNING_FAN_MAX)))
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

/**
 * @brief 将协议参数逐字段转换后调用唯一业务校验器。
 * @details 空输入直接返回INVALID_ARG；有效输入复制八字段到业务对象，由Tuning_ValidateValues
 *     校验后显式映射回SCB状态。这里不复制第二套限值算法，不修改调用方输入。
 * @param[in] values 非空八项快照；空指针INVALID_ARG。
 * @return 有限且各项在配置范围内OK，否则OUT_OF_RANGE。
 * @note 纯计算，前台或控制路径可调用，不钳位、不修改输入。
 */
scb_status_t SCB_ValidateValues(const scb_values_t *values)
{
    tuning_values_t business;
    if (!values)
    {
        return SCB_INVALID_ARG;
    }
    business.kp = values->kp;
    business.k2p = values->k2p;
    business.kd = values->kd;
    business.kp_v = values->kp_v;
    business.ki_v = values->ki_v;
    business.base_speed = values->base_speed;
    business.fan_run = values->fan_run;
    business.fan_idle = values->fan_idle;
    switch (Tuning_ValidateValues(&business))
    {
        case TUNING_OK:
            return SCB_OK;
        case TUNING_INVALID_ARG:
            return SCB_INVALID_ARG;
        default:
            return SCB_OUT_OF_RANGE;
    }
}

/**
 * @brief 编码LOG_DATA固定八字节头及原始日志块。
 * @details 依次写小端offset、length和零保留字段，再复制原始字节。
 *     调用方已完成范围和会话门禁，本函数不改变日志/链路状态。
 * @param[out] payload 非空至少968B目标，与data不重叠。
 * @param[in] offset 日志绝对字节偏移。
 * @param[in] data 非空且至少length字节的借用原始日志块。
 * @param[in] length 原始块长度，1..960字节。
 * @return 编码字节数8+length。
 * @note 前台在借用缓存被覆盖前立即调用；不分配第二个静态块。
 */
uint16_t SCB_EncodeLogData(uint8_t *payload, uint32_t offset,
    const uint8_t *data, uint16_t length)
{
    SCB_WriteU32(payload, offset);
    SCB_WriteU16(payload + 4, length);
    SCB_WriteU16(payload + 6, 0);
    memcpy(payload + 8, data, length);
    return (uint16_t)(8u + length);
}
