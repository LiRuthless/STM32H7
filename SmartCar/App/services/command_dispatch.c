/**
 * @file command_dispatch.c
 * @brief 通信业务路由、结果编码与事件适配。
 * @details 前台拥有原固定JSON工作区；ISR事件仅复制描述入原SCB队列，业务事务保持在control。
 */
#include "bsp.h"
#include "app.h"
#include "app_config.h"
#include "system/lifecycle.h"
#include "system/safety.h"
#include "system/tuning_application.h"
#include "system/parameter_application.h"
#include "control/tracking.h"
#include "control/tuning.h"
#include "control/electromagnetic_fsm.h"
#include "control/optical_fsm.h"
#include "control/motor.h"
#include "services/datalog.h"
#include "services/scb_link.h"
#include "services/telemetry.h"
#include "services/command_dispatch.h"
#include <string.h>

/* 仅前台使用的编码和缓存工作区，不放控制栈。 */
static void CommandDispatch_Set(uint32_t id, const scb_command_t *command);
static void CommandDispatch_Switch(uint32_t id, const scb_command_t *command);

static uint8_t s_json[SCB_PAYLOAD_MAX];
static uint8_t s_cached[SCB_PAYLOAD_MAX + 1u];
static scb_json_t s_query_json;
static datalog_meta_t s_log_meta;


/**
 * @brief 将业务状态显式映射为协议状态。
 * @details 逐枚举映射，不依赖两种枚举的数值相同。
 * @param[in] code 调参业务状态。
 * @return 相应SCB状态；未知业务值映射IO_ERROR。
 */
static scb_status_t CommandDispatch_MapResult(tuning_result_code_t code)
{
    switch (code)
    {
        case TUNING_OK:
            return SCB_OK;
        case TUNING_RECEIVED:
            return SCB_RECEIVED;
        case TUNING_APPLIED:
            return SCB_APPLIED;
        case TUNING_SAVED:
            return SCB_SAVED;
        case TUNING_INVALID_ARG:
            return SCB_INVALID_ARG;
        case TUNING_OUT_OF_RANGE:
            return SCB_OUT_OF_RANGE;
        case TUNING_BUSY:
            return SCB_BUSY;
        case TUNING_SPECIAL_ELEMENT:
            return SCB_SPECIAL_ELEMENT;
        case TUNING_NOT_FOUND:
            return SCB_NOT_FOUND;
        case TUNING_IO_ERROR:
            return SCB_IO_ERROR;
        default:
            return SCB_IO_ERROR;
    }
}

/**
 * @brief 逐字段复制业务与协议的八项参数。
 * @details 两类型各自拥有接口，禁止强转或别名共享布局。
 * @param[out] wire 非空完整输出。
 * @param[in] business 非空完整输入。
 */
static void CommandDispatch_ToWire(scb_values_t *wire, const tuning_values_t *business)
{
    wire->kp = business->kp;
    wire->k2p = business->k2p;
    wire->kd = business->kd;
    wire->kp_v = business->kp_v;
    wire->ki_v = business->ki_v;
    wire->base_speed = business->base_speed;
    wire->fan_run = business->fan_run;
    wire->fan_idle = business->fan_idle;
}

/**
 * @brief 逐字段复制业务与协议的八项参数。
 * @details 两类型各自拥有接口，禁止强转或别名共享布局。
 * @param[out] business 非空完整输出。
 * @param[in] wire 非空完整输入。
 */
static void CommandDispatch_ToBusiness(tuning_values_t *business, const scb_values_t *wire)
{
    business->kp = wire->kp;
    business->k2p = wire->k2p;
    business->kd = wire->kd;
    business->kp_v = wire->kp_v;
    business->ki_v = wire->ki_v;
    business->base_speed = wire->base_speed;
    business->fan_run = wire->fan_run;
    business->fan_idle = wire->fan_idle;
}

/**
 * @brief 初始化前台结果的固定JSON前缀。
 * @details 清writer绑定固定容量缓冲，操作名只来自解析后的已知枚举字符串。
 * @param[out] writer 非空writer，绑定静态s_json。
 * @param[in] op 固定合法操作名，无转义字符。
 * @param[in] status 公共状态枚举。
 * @note 前台独占；调用方添加字段后以CommandDispatch_Send结束对象。
 */
static void CommandDispatch_Begin(scb_writer_t *writer, const char *op, scb_status_t status)
{
    SCB_JsonBegin(writer, s_json, sizeof(s_json));
    SCB_JsonAppend(writer, "{\"op\":\"");
    SCB_JsonAppend(writer, op);
    SCB_JsonAppend(writer, "\",\"status\":\"");
    SCB_JsonAppend(writer, SCB_StatusName(status));
    SCB_JsonAppend(writer, "\"");
}

/**
 * @brief 结束JSON并更新已预留缓存。
 * @details 整条成功才交给可靠层；溢出以错误对象取代，不能发送部分JSON。
 * @param[in] id 非零已预留请求。
 * @param[in] op 本次原操作名，错误结果仍保持该名。
 * @param[in,out] writer 前台writer。
 * @param[in] final 零保留请求未完成，否则允许最终ACK后淘汰。
 * @note 溢出不发送截断内容，改为有界IO_ERROR；本模块所有成功形状须由宿主验长度。
 */
static void CommandDispatch_Send(uint32_t id, const char *op, scb_writer_t *writer, uint8_t final)
{
    SCB_JsonAppend(writer, "}");
    if (writer->failed)
    {
        CommandDispatch_Begin(writer, op, SCB_IO_ERROR);
        SCB_JsonAppend(writer, "}");
        final = 1;
    }
    (void)SCB_QueueResult(id, SCB_RESULT, s_json, writer->length, final);
}

/**
 * @brief 附加实际发生位置和版本。
 * @details 使用发生处已捕获的三个标量，不从后续控制状态重建时间。
 * @param[in,out] writer 非空前台writer。
 * @param[in] tick 采样tick。
 * @param[in] sequence 完整控制拍。
 * @param[in] revision 有效版本。
 * @note 不重新读取可能已经改变的活动对象。
 */
static void CommandDispatch_Position(scb_writer_t *writer, uint32_t tick,
    uint32_t sequence, uint32_t revision)
{
    SCB_JsonAppend(writer, ",\"tick\":");
    SCB_JsonU32(writer, tick);
    SCB_JsonAppend(writer, ",\"control_sequence\":");
    SCB_JsonU32(writer, sequence);
    SCB_JsonAppend(writer, ",\"revision\":");
    SCB_JsonU32(writer, revision);
}

/**
 * @brief 取得目标bank的完整存储值。
 * @param[in] mode 合法模式。
 * @param[in] gear 合法档位。
 * @param[out] values 非空八项副本。
 * @note 仅前台；不把运行调试值回收到存储查询。
 */
static void CommandDispatch_ReadBank(uint8_t mode, uint8_t gear, scb_values_t *values)
{
    const param_direction_t *p = Param_GetDirection(mode, gear);
    values->kp = p->kp;
    values->k2p = p->k2p;
    values->kd = p->kd;
    values->base_speed = p->base_speed;
    Param_GetSpeedGains(&values->kp_v, &values->ki_v);
    values->fan_run = Param_GetFan(gear, 0);
    values->fan_idle = Param_GetFan(gear, 1);
}

/**
 * @brief 输出完整bank、当前有效快照和同源限值。
 * @details 先复制存储/有效对象，再逐字段编码八组min/max，不在临界区格式化。
 * @param[in] id 已预留请求。
 * @param[in] command 合法GET_PARAMS目标。
 * @note 先短临界区复制，格式化不屏蔽中断。
 */
static void CommandDispatch_QueryParams(uint32_t id, const scb_command_t *command)
{
    static const char *const s_keys[] =
    {
        "kp", "k2p", "kd", "kp_v", "ki_v", "base_speed", "fan_run", "fan_idle"
    };
    scb_values_t values;
    tuning_status_t status;
    scb_values_t effective;
    scb_writer_t writer;
    uint8_t i;
    uint32_t mask = BSP_Time_EnterCritical();
    CommandDispatch_ReadBank(command->mode, command->gear, &values);
    Tuning_GetStatus(&status);
    if (App_IsStopped())
    {
        Tuning_ReadActive(&status.effective);
    }
    BSP_Time_ExitCritical(mask);
    CommandDispatch_Begin(&writer, command->op, SCB_OK);
    SCB_JsonAppend(&writer, ",\"values\":");
    SCB_JsonValues(&writer, &values);
    SCB_JsonAppend(&writer, ",\"effective\":");
    CommandDispatch_ToWire(&effective, &status.effective);
    SCB_JsonValues(&writer, &effective);
    SCB_JsonAppend(&writer, ",\"overridden\":");
    SCB_JsonAppend(&writer, status.overridden ? "true" : "false");
    SCB_JsonAppend(&writer, ",\"limits\":{");
    for (i = 0; i < 8; ++i)
    {
        SCB_JsonAppend(&writer, i ? ",\"" : "\"");
        SCB_JsonAppend(&writer, s_keys[i]);
        SCB_JsonAppend(&writer, "\":{\"min\":");
        SCB_JsonI32(&writer, i < 5 ? TUNING_GAIN_MIN
            : (i == 5 ? TUNING_SPEED_MIN : TUNING_FAN_MIN));
        SCB_JsonAppend(&writer, ",\"max\":");
        SCB_JsonI32(&writer, i < 5 ? TUNING_GAIN_MAX
            : (i == 5 ? TUNING_SPEED_MAX : TUNING_FAN_MAX));
        SCB_JsonAppend(&writer, "}");
    }
    SCB_JsonAppend(&writer, "}");
    CommandDispatch_Send(id, command->op, &writer, 1);
}

/**
 * @brief 输出一致运行/调参/上传/日志摘要。
 * @details 在短临界区固定两套元素及运行版本，再用有界writer构造前台结果。
 * @param[in] id 已预留请求。
 * @note 只复制内存状态，日志有效位来自已确认缓存，不在查询时扫描Flash。
 */
static void CommandDispatch_QueryStatus(uint32_t id)
{
    static const char *const s_phases[] = { "DISABLED", "ARMED", "ACTIVE", "SUSPENDED" };
    static const char *const s_saves[] = { "idle", "pending", "saved", "failed" };
    tuning_status_t tune;
    telemetry_status_t upload;
    scb_writer_t writer;
    tracking_state_t track;
    uint8_t kernel;
    uint8_t round;
    uint8_t optical;
    uint8_t running;
    uint32_t tick;
    uint32_t written;
    uint32_t mask = BSP_Time_EnterCritical();
    Tuning_GetStatus(&tune);
    track = *Tracking_GetState();
    kernel = ElectromagneticFSM_GetState()->kernel_state;
    round = ElectromagneticFSM_GetState()->roundabout_state;
    optical = (uint8_t)OpticalFSM_GetState()->state;
    running = Lifecycle_IsStarted();
    tick = BSP_Sampler_GetTick();
    written = Datalog_GetCount();
    Telemetry_GetStatus(&upload);
    Datalog_GetMeta(&s_log_meta);
    BSP_Time_ExitCritical(mask);
    CommandDispatch_Begin(&writer, "GET_STATUS", SCB_OK);
    SCB_JsonAppend(&writer, ",\"running\":");
    SCB_JsonAppend(&writer, running ? "true" : "false");
    SCB_JsonAppend(&writer, ",\"mode\":");
    SCB_JsonU32(&writer, track.mode);
    SCB_JsonAppend(&writer, ",\"gear\":");
    SCB_JsonU32(&writer, track.gear);
    SCB_JsonAppend(&writer, ",\"kernel_state\":");
    SCB_JsonU32(&writer, kernel);
    SCB_JsonAppend(&writer, ",\"roundabout_state\":");
    SCB_JsonU32(&writer, round);
    SCB_JsonAppend(&writer, ",\"optical_state\":");
    SCB_JsonU32(&writer, optical);
    SCB_JsonAppend(&writer, ",\"tune_state\":\"");
    SCB_JsonAppend(&writer, s_phases[tune.phase]);
    SCB_JsonAppend(&writer, "\",\"save_state\":\"");
    SCB_JsonAppend(&writer, s_saves[tune.save_state]);
    SCB_JsonAppend(&writer, "\",\"run_id\":");
    SCB_JsonU32(&writer, upload.run_id);
    SCB_JsonAppend(&writer, ",\"dropped_records\":");
    SCB_JsonU32(&writer, upload.dropped);
    SCB_JsonAppend(&writer, ",\"written_records\":");
    SCB_JsonU32(&writer, written);
    SCB_JsonAppend(&writer, ",\"log_valid\":");
    SCB_JsonAppend(&writer, s_log_meta.valid ? "true" : "false");
    SCB_JsonAppend(&writer, ",\"export_active\":");
    SCB_JsonAppend(&writer, Datalog_IsExporting() ? "true" : "false");
    CommandDispatch_Position(&writer, tick, track.control_sequence, tune.revision);
    CommandDispatch_Send(id, "GET_STATUS", &writer, 1);
}

/**
 * @brief 查询直接业务结果，避免查询自身产生递归包装。
 * @details 命中复制原缓存；JSON包装展开一层自身结果，二进制块返回独立定位信息。
 * @param[in] id 已预留查询请求。
 * @param[in] target 被查询缓存编号。
 * @note JSON保留完整对象；LOG_DATA仅回块定位信息，原始块由原LOG_READ去重重取。
 */
static void CommandDispatch_QueryResult(uint32_t id, uint32_t target)
{
    uint8_t type;
    uint16_t length;
    scb_writer_t writer;
    if (!SCB_GetRequestResult(target, &type, s_cached, &length))
    {
        CommandDispatch_Begin(&writer, "GET_REQUEST_RESULT", SCB_NOT_FOUND);
    }
    else
    {
        CommandDispatch_Begin(&writer, "GET_REQUEST_RESULT", SCB_OK);
        SCB_JsonAppend(&writer, ",\"target_request_id\":");
        SCB_JsonU32(&writer, target);
        if (type == SCB_LOG_DATA && length >= 8)
        {
            SCB_JsonAppend(&writer, ",\"offset\":");
            SCB_JsonU32(&writer, SCB_ReadU32(s_cached));
            SCB_JsonAppend(&writer, ",\"length\":");
            SCB_JsonU32(&writer, SCB_ReadU16(s_cached + 4));
        }
        else if (type == SCB_RESULT && SCB_ParseJson(s_cached, length, &s_query_json))
        {
            const char *op = SCB_GetString(&s_query_json,
                SCB_FindMember(&s_query_json, 0, "op"));
            char *result;
            s_cached[length] = 0;
            /* 仅展开本模块生成的查询包装；result始终最后一项且原对象完整。 */
            result = op && !strcmp(op, "GET_REQUEST_RESULT")
                ? strstr((char *)s_cached, ",\"result\":") : 0;
            if (result)
            {
                s_cached[length - 1u] = 0;
                result += 10;
            }
            SCB_JsonAppend(&writer, ",\"result\":");
            SCB_JsonAppend(&writer, result ? result : (char *)s_cached);
        }
        else
        {
            CommandDispatch_Begin(&writer, "GET_REQUEST_RESULT", SCB_IO_ERROR);
        }
    }
    CommandDispatch_Send(id, "GET_REQUEST_RESULT", &writer, 1);
}

/**
 * @brief 输出内存元信息和完整512位头文本。
 * @details 先复制已确认缓存，再逐字节转小写十六进制；未建会话输出空串，不扫描Flash。
 * @param[in] id 已预留请求编号。
 * @note 前台独占静态meta及JSON工作区，不修改日志和导出租期。
 */
static void CommandDispatch_QueryLogMeta(uint32_t id)
{
    static const char s_hex[] = "0123456789abcdef";
    scb_writer_t writer;
    uint16_t i;
    Datalog_GetMeta(&s_log_meta);
    CommandDispatch_Begin(&writer, "GET_LOG_META", SCB_OK);
    SCB_JsonAppend(&writer, ",\"log_valid\":");
    SCB_JsonAppend(&writer, s_log_meta.valid ? "true" : "false");
    SCB_JsonAppend(&writer, ",\"written_records\":");
    SCB_JsonU32(&writer, s_log_meta.written_records);
    SCB_JsonAppend(&writer, ",\"header_hex\":\"");
    for (i = 0; s_log_meta.valid && i < DATALOG_HEADER_SIZE; ++i)
    {
        char hex[3];
        hex[0] = s_hex[s_log_meta.header[i] >> 4];
        hex[1] = s_hex[s_log_meta.header[i] & 15u];
        hex[2] = 0;
        SCB_JsonAppend(&writer, hex);
    }
    SCB_JsonAppend(&writer, "\"");
    CommandDispatch_Send(id, "GET_LOG_META", &writer, 1);
}

/**
 * @brief 在前台执行单窗口日志导出命令。
 * @details Begin冻结前缀，Read成功直接缓存二进制块，End只有完整交付才释放；
 *     错误使用RESULT公共状态，容量已经由SCB在分发前预留。
 * @param[in] id 已预留请求编号。
 * @param[in] command 非空已校验的LOG_BEGIN/READ/END命令。
 * @note 前台可能进行有界Flash读；不改调试开关或车辆停车，不放整份日志到RAM。
 */
static void CommandDispatch_LogCommand(uint32_t id, const scb_command_t *command)
{
    scb_writer_t writer;
    scb_status_t status;
    datalog_export_status_t export;
    uint32_t total;
    uint32_t crc;
    uint16_t length;
    if (!strcmp(command->op, "LOG_READ"))
    {
        status = Datalog_ReadExport(command->transfer_id, command->offset, command->length,
            s_json, &length);
        if (status == SCB_OK)
        {
            (void)SCB_QueueResult(id, SCB_LOG_DATA, s_json, length, 1);
            return;
        }
    }
    else if (!strcmp(command->op, "LOG_BEGIN"))
    {
        status = Datalog_BeginExport(&export);
    }
    else
    {
        status = Datalog_EndExport(command->transfer_id, &total, &crc);
    }
    CommandDispatch_Begin(&writer, command->op, status);
    if (status == SCB_OK && !strcmp(command->op, "LOG_BEGIN"))
    {
        SCB_JsonAppend(&writer, ",\"transfer_id\":");
        SCB_JsonU32(&writer, export.transfer_id);
        SCB_JsonAppend(&writer, ",\"total_bytes\":");
        SCB_JsonU32(&writer, export.total_bytes);
        SCB_JsonAppend(&writer, ",\"record_count\":");
        SCB_JsonU32(&writer, export.record_count);
        SCB_JsonAppend(&writer, ",\"header_size\":256,\"record_size\":96");
    }
    else if (status == SCB_OK && !strcmp(command->op, "LOG_END"))
    {
        SCB_JsonAppend(&writer, ",\"total_bytes\":");
        SCB_JsonU32(&writer, total);
        SCB_JsonAppend(&writer, ",\"crc32\":");
        SCB_JsonU32(&writer, crc);
    }
    CommandDispatch_Send(id, command->op, &writer, 1);
}

/**
 * @brief 分发已被SCB接管并预留最终结果的业务命令。
 * @details 根据完整操作名转给查询/调参路径，未知操作最终INVALID_ARG，不做部分更新。
 * @param[in] id 非零已预留请求。
 * @param[in] command 非空已完整解析命令。
 * @note 前台串行；日志读取同样只停车执行，不在ISR执行JSON或Flash。
 */
static void CommandDispatch_HandleCommand(uint32_t id, const scb_command_t *command)
{
    scb_writer_t writer;
    if (!strcmp(command->op, "SET_PARAMS"))
    {
        CommandDispatch_Set(id, command);
    }
    else if (!strcmp(command->op, "TUNE_ENABLE") || !strcmp(command->op, "TUNE_DISABLE"))
    {
        CommandDispatch_Switch(id, command);
    }
    else if (!strcmp(command->op, "GET_PARAMS"))
    {
        CommandDispatch_QueryParams(id, command);
    }
    else if (!strcmp(command->op, "GET_STATUS"))
    {
        CommandDispatch_QueryStatus(id);
    }
    else if (!strcmp(command->op, "GET_REQUEST_RESULT"))
    {
        CommandDispatch_QueryResult(id, command->target_request_id);
    }
    else if (!strcmp(command->op, "GET_LOG_META"))
    {
        CommandDispatch_QueryLogMeta(id);
    }
    else if (!strcmp(command->op, "LOG_BEGIN") || !strcmp(command->op, "LOG_READ")
        || !strcmp(command->op, "LOG_END"))
    {
        CommandDispatch_LogCommand(id, command);
    }
    else
    {
        CommandDispatch_Begin(&writer, command->op, SCB_INVALID_ARG);
        CommandDispatch_Send(id, command->op, &writer, 1);
    }
}


/**
 * @brief 校验并接收完整设置，运行态只发布一个待应用快照。
 * @param[in] id 已完成缓存容量预留的非零编号。
 * @param[in] command 完整设置，调用方仍由本函数二次校验。
 * @note 前台；停车配置先APPLIED后Service保存，所有失败不修改任一参数。
 */
static void CommandDispatch_Set(uint32_t id, const scb_command_t *command)
{
    tuning_request_t request;
    tuning_reply_t reply;
    scb_writer_t writer;
    scb_status_t status;
    request.mode = command->mode;
    request.gear = command->gear;
    CommandDispatch_ToBusiness(&request.values, &command->values);
    TuningApplication_Set(id, &request, &reply);
    status = CommandDispatch_MapResult(reply.code);
    CommandDispatch_Begin(&writer, command->op, status);
    if (status == SCB_APPLIED)
    {
        SCB_JsonAppend(&writer, ",\"values\":");
        SCB_JsonValues(&writer, &command->values);
        CommandDispatch_Position(&writer, reply.position.tick,
            reply.position.control_sequence, reply.position.revision);
    }
    CommandDispatch_Send(id, command->op, &writer, status != SCB_RECEIVED);
}

/**
 * @brief 开启或关闭当前目标覆盖。
 * @details 同目标重复开启不重捕获；停车目标改变后重新绑定并捕获基线，运行不切目标。
 *     禁用取消未生效请求，同目标在恢复拍前重开沿用旧基线。
 * @param[in] id 已预留请求编号。
 * @param[in] command TUNE_ENABLE或TUNE_DISABLE。
 * @note 前台只复制基线/改许可；运行恢复在下个完整拍，停车立即恢复共享值。
 */
static void CommandDispatch_Switch(uint32_t id, const scb_command_t *command)
{
    tuning_reply_t reply;
    scb_writer_t writer;
    TuningApplication_Switch((uint8_t)!strcmp(command->op, "TUNE_ENABLE"), &reply);
    CommandDispatch_Begin(&writer, command->op, CommandDispatch_MapResult(reply.code));
    CommandDispatch_Position(&writer, reply.position.tick,
        reply.position.control_sequence, reply.position.revision);
    CommandDispatch_Send(id, command->op, &writer, 1);
}
/**
 * @brief 取消会话尚未完成请求和导出。
 * @details 先清事务pending/final，再取消导出；已成功参数和开关不清。
 */
void CommandDispatch_ResetSession(void)
{
    Tuning_ResetSession();
    Datalog_CancelExport();
}

/**
 * @brief 注册业务请求及会话回调。
 * @details 在InitLink和Tuning_Init之后执行，不再次初始化链路或事务。
 */
void CommandDispatch_Init(void)
{
    SCB_SetHandlers(CommandDispatch_HandleCommand, CommandDispatch_ResetSession);
}

/**
 * @brief 初始化原可靠通信链路。
 * @details 仅在原初始化调用点转发一次SCB_Init。
 */
void CommandDispatch_InitLink(void)
{
    SCB_Init();
}

/**
 * @brief 执行原前台链路服务。
 * @details 在业务Service之后单独调用；不得合并检查点或增加重试。
 */
void CommandDispatch_ServiceLink(void)
{
    SCB_Service();
}

/**
 * @brief 前台输出事务结果并尝试停车保存。
 * @details 先服务导出租期，后取最终结果编码，再处理停车保存。
 */
void CommandDispatch_Service(void)
{
    tuning_reply_t reply;
    uint32_t id;
    scb_writer_t writer;
    scb_values_t values;
    Datalog_ServiceExport();
    if (Tuning_TakeResult(&reply, &id))
    {
        CommandDispatch_Begin(&writer, "SET_PARAMS", CommandDispatch_MapResult(reply.code));
        if (reply.code == TUNING_APPLIED)
        {
            SCB_JsonAppend(&writer, ",\"values\":");
            CommandDispatch_ToWire(&values, &reply.values);
            SCB_JsonValues(&writer, &values);
            CommandDispatch_Position(&writer, reply.position.tick,
                reply.position.control_sequence, reply.position.revision);
        }
        CommandDispatch_Send(id, "SET_PARAMS", &writer, 1);
    }
    TuningApplication_ServiceSave();
}

/**
 * @brief 把即时业务事件映射为SCB事件描述。
 * @details 先取上传run_id，再取采样tick和完整序号，随后显式映射事件种类和状态；
 *     在原位置读取版本及成功/有效值，字段复制后入原事件队列，队满仍忽略返回。
 * @param[in] business 非空即时业务事件，只含kind/code，当前上下文同步适配。
 * @note 允许ISR，仅复制固定对象；队满由SCB发布gap，不挤占最终结果邮箱。
 */
void CommandDispatch_QueueTuningEvent(const tuning_event_t *business)
{
    telemetry_status_t upload;
    scb_event_t event = { 0 };
    tuning_values_t values;
    Telemetry_GetStatus(&upload);
    event.run_id = upload.run_id;
    event.tick = BSP_Sampler_GetTick();
    event.control_sequence = Tracking_GetState()->control_sequence;
    switch (business->kind)
    {
        case TUNING_EVENT_TUNE_APPLIED:
            event.kind = SCB_EVENT_TUNE_APPLIED;
            break;
        case TUNING_EVENT_TUNE_SUSPENDED:
            event.kind = SCB_EVENT_TUNE_SUSPENDED;
            break;
        case TUNING_EVENT_TUNE_RESUMED:
            event.kind = SCB_EVENT_TUNE_RESUMED;
            break;
        case TUNING_EVENT_PARAM_SAVED:
            event.kind = SCB_EVENT_PARAM_SAVED;
            break;
        case TUNING_EVENT_PARAM_SAVE_FAILED:
            event.kind = SCB_EVENT_PARAM_SAVE_FAILED;
            break;
    }
    event.status = CommandDispatch_MapResult(business->code);
    Tuning_CopyEventValues(business->kind, &event.revision, &values);
    event.has_values = 1;
    CommandDispatch_ToWire(&event.values, &values);
    (void)SCB_QueueEvent(&event);
}

/**
 * @brief 将原运行起跑事件入通信队列。
 * @details 按原顺序复制上传身份、采样tick和完整序号，入队失败仍忽略。
 * @note 原运行位置调用，不格式化JSON，不增加事件缓存。
 */
void CommandDispatch_PublishRunStarted(void)
{
    telemetry_status_t upload;
    scb_event_t event = { 0 };
    Telemetry_GetStatus(&upload);
    event.kind = SCB_EVENT_RUN_STARTED;
    event.run_id = upload.run_id;
    event.revision = upload.revision;
    event.tick = BSP_Sampler_GetTick();
    event.control_sequence = Tracking_GetState()->control_sequence;
    (void)SCB_QueueEvent(&event);
}

/**
 * @brief 将原运行停车事件入通信队列。
 * @details 按原顺序复制上传身份、采样tick和完整序号，入队失败仍忽略。
 * @param[in] reason 已锁存且本次运行不变的首次停车原因。
 * @note 原运行位置调用，不格式化JSON，不增加事件缓存。
 */
void CommandDispatch_PublishRunStopped(uint8_t reason)
{
    telemetry_status_t upload;
    scb_event_t event = { 0 };
    Telemetry_GetStatus(&upload);
    event.kind = SCB_EVENT_RUN_STOPPED;
    event.run_id = upload.run_id;
    event.revision = upload.revision;
    event.tick = BSP_Sampler_GetTick();
    event.control_sequence = Tracking_GetState()->control_sequence;
    event.stop_reason = reason;
    (void)SCB_QueueEvent(&event);
}
