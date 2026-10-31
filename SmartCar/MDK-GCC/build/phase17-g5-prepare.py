"""Prepare G5 source drafts only; never mutate product files or the Git index.

Functions are extracted using lexical tokens, so braces in strings/comments do
not terminate a definition. Inputs are read afresh each run after G4 freezes.
"""
from pathlib import Path
import re
import json
import hashlib

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / 'App'
OUT = ROOT / 'MDK-GCC/build/phase17-g5-draft'
TOKEN = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*|[^\s]', re.S)


def tokens(text):
    return [m for m in TOKEN.finditer(text) if not m[0].startswith(('/*', '//'))]


def functions(text):
    """Return unique whole definitions with their immediately preceding docs."""
    ts = tokens(text)
    result = {}
    level = 0
    for i, token in enumerate(ts):
        value = token[0]
        if value == '{' and level == 0 and i and ts[i - 1][0] == ')':
            depth = 1
            j = i - 2
            while depth:
                depth += (ts[j][0] == ')') - (ts[j][0] == '(')
                j -= 1
            name = ts[j][0]
            first = j
            while first and ts[first - 1][0] not in (';', '}', '{'):
                first -= 1
            # Preprocessor lines and earlier declarations cannot be signature.
            line = text.rfind('\n', 0, ts[j].start()) + 1
            start = line
            before = text[:line].rstrip()
            if before.endswith('*/'):
                start = before.rfind('/**')
                if start < 0:
                    start = line
            end = i + 1
            depth = 1
            while depth:
                depth += (ts[end][0] == '{') - (ts[end][0] == '}')
                end += 1
            if name in result:
                raise ValueError(f'duplicate definition: {name}')
            result[name] = text[start:ts[end - 1].end()]
        level += (value == '{') - (value == '}')
    return result


def replace_once(text, old, new):
    assert text.count(old) == 1, (old[:100], text.count(old))
    return text.replace(old, new)


def unit(name, brief, details, includes, declarations, definitions):
    return ('/**\n * @file ' + name + '\n * @brief ' + brief + '\n'
            + ' * @details ' + details.replace('\n', '\n *     ') + '\n */\n'
            + ''.join('#include ' + h + '\n' for h in includes) + '\n'
            + declarations.rstrip() + '\n\n' + '\n\n'.join(definitions) + '\n')


def signature(definition):
    value = definition[definition.index('*/') + 2:].strip() if definition.startswith('/**') else definition
    return value[:value.index('{')].strip()


def header(name, brief, details, includes, types, definitions, c_name=None):
    guard = 'SMARTCAR_APP_SERVICES_' + name[:-2].upper() + '_H'
    body = []
    for definition in definitions:
        sig = signature(definition)
        if sig.startswith('static '):
            continue
        desc = re.search(r'@brief ([^\n]+)', definition)
        desc = desc[1] if desc else brief
        function = re.search(r'(\w+)\(', sig)[1]
        body.append('/**\n * @brief ' + desc + '\n * @note 完整契约见 '
                    + (c_name or name[:-2] + '.c') + '::' + function + '。\n */\n' + sig + ';')
    return ('/**\n * @file ' + name + '\n * @brief ' + brief + '\n * @details ' + details + '\n */\n'
            + '#ifndef ' + guard + '\n#define ' + guard + '\n'
            + ''.join('#include ' + h + '\n' for h in includes) + '\n'
            + types.rstrip() + '\n\n' + '\n\n'.join(body) + '\n\n#endif /* ' + guard + ' */\n')


def type_block(text, name):
    match = re.search(r'typedef\s+(?:enum|struct)\s*\{[^}]*\}\s*' + re.escape(name) + r';', text, re.S)
    assert match, name
    return match[0]


def document(brief, details, params=(), result=None, pre=None, note=None):
    lines = ['/**', ' * @brief ' + brief]
    if details:
        lines.extend(' * ' + ('@details ' if i == 0 else '    ') + part
                     for i, part in enumerate(details.split('\n')))
    lines.extend(' * @param[' + direction + '] ' + name + ' ' + desc for direction, name, desc in params)
    if result:
        lines.append(' * @return ' + result)
    if pre:
        lines.append(' * @pre ' + pre)
    if note:
        lines.extend(' * ' + ('@note ' if i == 0 else '    ') + part
                     for i, part in enumerate(note.split('\n')))
    return '\n'.join(lines + [' */'])


def write(name, text):
    target = OUT / 'services' / name
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(re.sub(r'\n{3,}', '\n\n', text), encoding='utf-8', newline='\n')


def prepare_logging(source, public):
    fs = functions(source)
    layout = source[source.index('#define DATALOG_MAGIC'):source.index('/* TIM15 单生产者')]
    layout = layout.replace('#define DATALOG_BATCH_RECORDS 8u\n', '')
    record = [fs[n] for n in ('Datalog_CalculateCrc16', 'Datalog_SetProfile')]
    record.append(document('清空完整控制快照。', None,
                           note='仅日志初始化调用；不清会话头，也不触碰日志队列或上传状态。')
                  + '\nvoid RunRecord_ResetSnapshot(void)\n{\n    memset(&s_snapshot, 0, sizeof(s_snapshot));\n}')
    record.append(document('检查当前灰度标定能否建立日志会话。', None,
                           result='标定完整且对比度满足原规则为1，否则0。',
                           note='起跑前台调用，只转发Gray_CheckCalibration(0)，不输出诊断文本。')
                  + '\nuint8_t RunRecord_CheckCalibration(void)\n{\n    return Gray_CheckCalibration(0);\n}')
    start = fs['Datalog_Start']
    build = start[start.index('    memset(&s_header'):start.index('    if (BSP_Flash_LogErase())')]
    record.append(document('组装本次会话的固定256字节头。',
                           '清零头部后依次复制模式/档位、标定、两套方向bank和活动速度PI。\n'
                           '固定预算、周期及容量写入后计算字节8..255的CRC16，保留区保持零。',
                           result='唯一静态会话头的只读地址；下次构造会覆盖内容。',
                           note='仅起跑前台在配置和标定门禁通过后调用；无Flash访问。')
                  + '\nconst datalog_header_t *RunRecord_BuildHeader(void)\n{\n'
                  + '    const tracking_state_t *t = Tracking_GetState();\n' + build
                  + '    return &s_header;\n}')
    record.append(document('借用当前内存会话头。', None,
                           result='同一静态256字节头的只读地址；有效性由logger单独维护。',
                           note='读取不扫描Flash、不构造头；调用方不得写入或保存另一份当前会话状态。')
                  + '\nconst datalog_header_t *RunRecord_GetHeader(void)\n{\n    return &s_header;\n}')
    record.append(fs['Datalog_PublishControl'].replace('s_snapshot.stop_reason = s_status.reason;',
                                                      's_snapshot.stop_reason = Logger_GetStopReason();'))
    push = fs['Datalog_Push']
    capture = push[push.index('    rec = s_snapshot;'):push.index('    /* 先完成槽内容')]
    capture = capture.replace('rec = s_snapshot;', '*record = s_snapshot;').replace('rec.', 'record->')
    capture = capture.replace('Telemetry_PushRecord(&rec)', 'Telemetry_PushRecord(record)')
    record.append(document('把完整控制快照与当前1ms采样合为记录。',
                           '先复制快照，再依次读tick、左右编码器和三轴陀螺原始值。\n'
                           '三轴逐个除14.3并截断到int16_t，完整记录交给独立上传队列后返回。',
                           [('out', 'record', '非空96字节目标，logger门禁全部通过后才写入。')],
                           note='仅TIM15单生产者；TIM6同级不抢占，不发布Flash队列head或计数。')
                  + '\nvoid RunRecord_CaptureSample(datalog_record_t *record)\n{\n'
                  + '    int16_t raw[3];\n' + capture + '}')
    record.append(fs['Datalog_IsHeaderValid'].replace('static uint8_t Datalog_IsHeaderValid',
                                                    'uint8_t RunRecord_IsHeaderValid'))
    write('run_record.c', unit('run_record.c', '固定日志头和完整控制记录组装。',
                              'TIM6发布唯一96B快照，TIM15在入队门禁后复制并补采样字段。\n'
                              '前台构造唯一256B会话头；布局、CRC16和标定校验在本模块集中。',
                              ['"services/run_record.h"', '"services/logger.h"', '"bsp.h"',
                               '"app_config.h"', '"sensing/battery.h"', '"system/lifecycle.h"',
                               '"control/tracking.h"', '"sensing/track_sensor.h"',
                               '"sensing/gray_sensor.h"', '"control/optical_fsm.h"',
                               '"control/electromagnetic_fsm.h"', '"services/parameter_bank.h"',
                               '"system/parameter_application.h"', '"control/pid.h"',
                               '"services/telemetry.h"', '<stddef.h>', '<string.h>'],
                              layout + '\n/* TIM6发布、TIM15复制，同级中断不交错。 */\n'
                              + 'static datalog_record_t s_snapshot;\n'
                              + '/* 前台唯一当前会话头；logger和导出只借用其地址。 */\n'
                              + 'static datalog_header_t s_header;', record))
    write('run_record.h', header('run_record.h', '日志记录组装内部接口。',
                                '声明run_record.c提供的布局校验、快照和头部接口；不暴露可写状态。',
                                ['"services/datalog.h"'], '', record))

    own = ('Datalog_LatchFault', 'Datalog_RequestFaultStop', 'Datalog_SetStopSummary',
           'Datalog_GetStatus', 'Datalog_IsActive', 'Datalog_GetCount', 'Datalog_Init',
           'Datalog_Start', 'Datalog_Clear', 'Datalog_GetMeta', 'Datalog_Push',
           'Datalog_Service', 'Datalog_Flush', 'Datalog_Stop')
    logger = []
    for name in own:
        text = fs[name].replace('Datalog_LatchFault', 'Logger_LatchFault')
        if name == 'Datalog_LatchFault':
            text = text.replace('static void Logger_LatchFault', 'void Logger_LatchFault')
        if name == 'Datalog_Init':
            text = text.replace('memset(&s_snapshot, 0, sizeof(s_snapshot));', 'RunRecord_ResetSnapshot();')
        if name == 'Datalog_Start':
            text = text.replace('    static datalog_header_t s_check;\n', '')
            text = text.replace('    const tracking_state_t *t = Tracking_GetState();',
                                '    const tracking_state_t *t = Tracking_GetState();\n'
                                '    const datalog_header_t *header;\n    (void)t;')
            text = text.replace('Gray_CheckCalibration(0)', 'RunRecord_CheckCalibration()')
            text = replace_once(text, build, '    header = RunRecord_BuildHeader();\n')
            text = replace_once(text, 'BSP_Flash_LogWrite(0, (const uint8_t *)&s_header, 256)\n'
                                '        || BSP_Flash_LogRead(0, (uint8_t *)&s_check, 256)'
                                ' || memcmp(&s_check, &s_header, 256)', 'LogStore_WriteHeader(header)')
        if name == 'Datalog_GetMeta':
            text = text.replace('memcpy(meta->header, &s_header, DATALOG_HEADER_SIZE);',
                                'memcpy(meta->header, RunRecord_GetHeader(), DATALOG_HEADER_SIZE);')
        if name == 'Datalog_Push':
            text = text.replace('    int16_t raw[3];\n', '')
            sample = push[push.index('    rec = s_snapshot;'):push.index('    /* 先完成槽内容')]
            text = replace_once(text, sample, '    RunRecord_CaptureSample(&rec);\n')
        text = text.replace('BSP_Flash_LogErase', 'LogStore_Erase').replace('BSP_Flash_LogWrite', 'LogStore_Write')
        logger.append(text)
    for name, field, brief in [('Logger_GetStopReason', 's_status.reason', '读取当前会话首次停车原因。'),
                                ('Logger_HasHeader', 's_header_valid', '读取当前会话头已写回校验标志。')]:
        logger.append(document(brief, None, result='对应原状态字节，不清状态也不更新pending。',
                               note='只执行一次标量读取，不代表整个会话原子快照。')
                      + '\nuint8_t ' + name + '(void)\n{\n    return ' + field + ';\n}')
    declarations = source[source.index('/* TIM15 单生产者'):source.index('/* 前台导出独占状态')]
    declarations = re.sub(r'/\* TIM6 发布.*?static datalog_record_t s_snapshot;\n', '', declarations, flags=re.S)
    declarations = re.sub(r'/\* 前台持有会话头.*?static datalog_header_t s_header;\n', '', declarations, flags=re.S)
    declarations = '#define DATALOG_BATCH_RECORDS 8u\n' + declarations
    write('logger.c', unit('logger.c', '日志会话和Flash单生产者单消费者队列。',
                          'TIM15门禁后生产1024槽环形缓冲，前台每次至多写八条。\n'
                          '会话有效位、首故障、首因及统计均在此持有；写失败后不重试、不提前释放槽。',
                          ['"services/logger.h"', '"services/run_record.h"', '"services/log_store.h"',
                           '"services/log_export.h"', '"bsp.h"', '"app.h"', '"app_config.h"',
                           '"control/tracking.h"', '<string.h>'], declarations, logger))
    write('logger.h', header('logger.h', '日志会话状态内部只读与故障接口。',
                            '完整契约在logger.c；公开Datalog入口仍由datalog.h声明。',
                            ['"services/datalog.h"'], '',
                            [f for f in logger if signature(f).split('(')[0].split()[-1].startswith('Logger_')]))

    store = []
    store.append(document('擦除日志首扇区并复位底层懒擦前沿。', None,
                          result='底层返回值，0成功，非零失败。',
                          note='仅前台调用，可阻塞；不维护会话、统计或故障。')
                 + '\nuint8_t LogStore_Erase(void)\n{\n    return BSP_Flash_LogErase();\n}')
    store.append(document('写头后回读并逐字节核对。',
                          '先写offset0的256字节，再读入唯一静态校验缓冲并比较。\n'
                          '写、读、比较沿用短路次序，任一步失败即返回，不重试或修改会话状态。',
                          [('in', 'header', '非空256字节会话头，整个调用期间可读。')],
                          result='全部成功0；任一步失败1。', note='仅起跑前台调用，可阻塞，不占256B栈。')
                 + '\nuint8_t LogStore_WriteHeader(const datalog_header_t *header)\n{\n'
                 + '    if (BSP_Flash_LogWrite(0, (const uint8_t *)header, 256)\n'
                 + '        || BSP_Flash_LogRead(0, (uint8_t *)&s_check, 256)\n'
                 + '        || memcmp(&s_check, header, 256))\n    {\n        return 1;\n    }\n'
                 + '    return 0;\n}')
    for action, direction, verb in [('Write', 'in', '写入'), ('Read', 'out', '读取')]:
        qualifier = 'const ' if action == 'Write' else ''
        store.append(document('按字节' + verb + '日志区。', None,
                              [('in', 'offset', '日志区内绝对字节偏移。'),
                               (direction, 'data', '非空且至少length字节；底层失败可能已经处理部分字节。'),
                               ('in', 'length', verb + '字节数，范围由BSP检查。')],
                              result='底层返回值，0成功，非零失败。',
                              note='仅前台调用，可阻塞；不缓存、重试或喂狗，不修改日志队列。')
                     + '\nuint8_t LogStore_' + action + '(uint32_t offset, ' + qualifier
                     + 'uint8_t *data, uint32_t length)\n{\n    return BSP_Flash_Log' + action
                     + '(offset, data, length);\n}')
    write('log_store.c', unit('log_store.c', '日志Flash字节存储后端。',
                             '唯一App日志Flash访问点，将前台请求一一转发BSP。\n'
                             '仅持写头回读校验缓冲，不拥有会话、队列、导出或协议状态。',
                             ['"services/log_store.h"', '"bsp.h"', '<string.h>'],
                             '/* 前台写头回读临时副本，不是当前会话头状态。 */\n'
                             + 'static datalog_header_t s_check;', store))
    write('log_store.h', header('log_store.h', '日志Flash字节后端接口。',
                               '实现见log_store.c；调用者负责会话状态、地址公式及错误处理。',
                               ['"services/datalog.h"'], '', store))

    export = []
    for name in ('Datalog_CancelExport', 'Datalog_ServiceExport', 'Datalog_IsExporting',
                 'Datalog_GetExportStatus', 'Datalog_BeginExport', 'Datalog_ReadExport',
                 'Datalog_EndExport', 'Datalog_Dump'):
        text = fs[name].replace('Datalog_LatchFault', 'Logger_LatchFault')
        text = text.replace('scb_status_t', 'datalog_result_t')
        text = re.sub(r'\bSCB_(OK|INVALID_ARG|OUT_OF_RANGE|BUSY|NOT_FOUND|IO_ERROR)\b',
                      lambda m: 'DATALOG_RESULT_' + m[1], text)
        text = text.replace('s_status.active', 'Datalog_IsActive()')
        text = text.replace('s_status.written', 'Datalog_GetCount()')
        text = text.replace('s_header_valid', 'Logger_HasHeader()')
        text = text.replace('Datalog_IsHeaderValid', 'RunRecord_IsHeaderValid')
        text = text.replace('BSP_Flash_LogRead', 'LogStore_Read')
        text = text.replace('静态缓冲避免占用 AC5 小栈', '静态大缓冲避免占用运行栈')
        text = text.replace('大缓冲静态分配，避开AC5的1KB栈。', '大缓冲静态分配，避免占用运行栈。')
        text = text.replace('会话取消旧传输缓存由SCB负责', '会话取消后的旧传输缓存由通信适配层负责')
        if name == 'Datalog_ReadExport':
            doc = document('顺序读取原始日志字节或借用最近成功块。',
                           '先检查租期、身份和减法边界；最近同offset/length块直接借用缓存。\n'
                           '新块只允许next_offset，读前后喂狗，成功才推进CRC/offset/租期。\n'
                           '读错锁READ并取消导出；失败不发布data，只先把data_length置零。',
                           [('in', 'id', '当前非零导出身份。'), ('in', 'offset', '日志绝对字节偏移。'),
                            ('in', 'length', '请求1..960字节。'),
                            ('out', 'data', '非空指针槽，成功借出唯一缓存+8处原始字节，失败不写。'),
                            ('out', 'data_length', '非空长度槽，先置0，成功写原始字节数。')],
                           result='OK成功；失效NOT_FOUND；边界OUT_OF_RANGE；乱序INVALID_ARG；读错IO_ERROR。',
                           note='仅前台串行；借用在下次新块读取覆盖缓存前有效，调用方须同次前台调用内复制。')
            body = text[text.index('{'):]
            body = body.replace('*payload_length', '*data_length')
            body = body.replace('memcpy(payload, s_export_block, length + 8u);',
                                '*data = s_export_block + 8;')
            body = body.replace('(uint16_t)(length + 8u)', '(uint16_t)length')
            body = body.replace('    SCB_WriteU32(s_export_block, offset);\n'
                                '    SCB_WriteU16(s_export_block + 4, (uint16_t)length);\n'
                                '    SCB_WriteU16(s_export_block + 6, 0);\n', '')
            body = body.replace('SCB_CalculateCrc', 'Checksum_UpdateCrc32')
            text = doc + '\ndatalog_result_t Datalog_ReadExport(uint32_t id, uint32_t offset, uint32_t length,\n'
            text += '    const uint8_t **data, uint16_t *data_length)\n' + body
        export.append(text)
    export_state = source[source.index('/* 前台导出独占状态'):source.index('static uint16_t Datalog_CalculateCrc16')]
    write('log_export.c', unit('log_export.c', '停车日志原始字节导出与CSV输出。',
                              '前台冻结成功落盘前缀，使用唯一968B块缓存与10秒无进展租期。\n'
                              '本模块返回独立业务结果，不编码通信头；CSV保持静态缓冲及原文本。',
                              ['"services/log_export.h"', '"services/logger.h"',
                               '"services/run_record.h"', '"services/log_store.h"',
                               '"services/checksum.h"', '"sensing/gray_sensor.h"',
                               '"app.h"', '"bsp.h"', '<stdio.h>', '<string.h>'], export_state, export))
    write('log_export.h', header('log_export.h', '日志导出模块内部依赖入口。',
                                'log_export.c持有前台导出状态；公共导出接口和独立结果见datalog.h。',
                                ['"services/datalog.h"'], '', []))
    public = public.replace('#include "services/scb_protocol.h"\n', '')
    public = public.replace('SMARTCAR_APP_DATALOG_H', 'SMARTCAR_APP_SERVICES_DATALOG_H')
    public = public.replace('scb_status_t', 'datalog_result_t')
    public = public.replace('    uint8_t *payload, uint16_t *payload_length);',
                            '    const uint8_t **data, uint16_t *data_length);')
    marker = 'typedef struct\n{\n    uint8_t active; /**< 前台导出互斥是否持有。 */'
    enum = 'typedef enum\n{\n'
    for code, brief in [('OK', '业务调用成功。'), ('INVALID_ARG', '乱序或参数形状无效。'),
                        ('OUT_OF_RANGE', '字节范围越界。'), ('BUSY', '当前状态不允许。'),
                        ('NOT_FOUND', '会话或导出身份不存在。'), ('IO_ERROR', '日志读取失败。')]:
        enum += '    DATALOG_RESULT_' + code + ', /**< ' + brief + ' */\n'
    enum += '} datalog_result_t;\n\n'
    public = replace_once(public, marker, enum + marker)
    for name in own:
        public = re.sub(r'datalog\.c::' + name + r'\b', 'logger.c::' + name, public)
    for name in ('Datalog_CancelExport', 'Datalog_ServiceExport', 'Datalog_IsExporting',
                 'Datalog_GetExportStatus', 'Datalog_BeginExport', 'Datalog_ReadExport',
                 'Datalog_EndExport', 'Datalog_Dump'):
        public = public.replace('datalog.c::' + name, 'log_export.c::' + name)
    public = public.replace('datalog.c::Datalog_PublishControl', 'run_record.c::Datalog_PublishControl')
    public = public.replace('@brief 顺序读取或重取最近块。', '@brief 顺序读取或重取最近原始日志块。')
    public = public.replace('@note 完整契约见log_export.c::Datalog_ReadExport。',
                            '@note 成功借出内部缓存，下次新读取可覆盖；同次前台调用内完成复制。\n'
                            ' *     完整契约见log_export.c::Datalog_ReadExport。')
    public = public.replace('完整并发和失败语义位于 datalog.c。',
                            '完整并发和失败语义分别位于run_record/logger/log_export实现。')
    write('datalog.h', public)
    return fs


def prepare_protocol(source, public):
    fs = functions(source)
    frame_names = ('SCB_ReadU16', 'SCB_ReadU32', 'SCB_WriteU16', 'SCB_WriteU32',
                   'SCB_CalculateCrc', 'SCB_BuildFrame', 'SCB_EncodeFrame', 'SCB_DecodeFrame')
    json_names = ('SCB_ValidateUtf8', 'SCB_SkipSpace', 'SCB_ReadHex', 'SCB_AppendUtf8',
                  'SCB_ParseString', 'SCB_ParseNumber', 'SCB_ParseValue', 'SCB_ParseJson',
                  'SCB_FindMember', 'SCB_GetString', 'SCB_GetNumber', 'SCB_JsonBegin',
                  'SCB_JsonAppend', 'SCB_JsonU32', 'SCB_JsonI32', 'SCB_JsonFloat')
    codec_names = ('SCB_StatusName', 'SCB_JsonValues', 'SCB_ParseCommand', 'SCB_ValidateValues')
    checksum = fs['SCB_CalculateCrc'].replace('SCB_CalculateCrc', 'Checksum_UpdateCrc32')
    write('checksum.c', unit('checksum.c', '通用可续算反射CRC32。',
                            '按字节更新zlib兼容CRC32，不依赖协议、日志、硬件或共享状态。',
                            ['"services/checksum.h"'], '', [checksum]))
    write('checksum.h', header('checksum.h', '可续算CRC32接口。',
                              '实现见checksum.c；初次previous传0，后续传上次结果。',
                              ['<stdint.h>', '<stddef.h>'], '', [checksum]))
    frame = [fs[n] for n in frame_names]
    frame[4] = document('转发通用CRC32续算。', None,
                        [('in', 'previous', '上一次结果，初次传0。'),
                         ('in', 'data', '至少length字节，length为0时允许空指针。'),
                         ('in', 'length', '需要累计的字节数。')],
                        result='与通用Checksum_UpdateCrc32相同，空长度返回previous。',
                        note='无共享状态；帧层保留原公共名称，公式仅在checksum.c定义。')
    frame[4] += ('\nuint32_t SCB_CalculateCrc(uint32_t previous, const uint8_t *data, size_t length)\n'
                 '{\n    return Checksum_UpdateCrc32(previous, data, length);\n}')
    defines = '\n'.join(re.findall(r'^#define SCB_.*$', public, re.M))
    frame_defines = '\n'.join(line for line in defines.splitlines() if not re.search('SCB_JSON_', line))
    frame_types = frame_defines + '\n\n' + type_block(public, 'scb_frame_t')
    write('scb_frame.c', unit('scb_frame.c', 'SCB帧字段与字节编解码。',
                             '仅验证/编码帧字段与CRC，失败保留调用方输出；负载借用输入缓冲。\n'
                             '帧构造不依赖业务命令、JSON解析、会话状态或硬件。',
                             ['"services/scb_frame.h"', '"services/checksum.h"', '<string.h>'],
                             'typedef char scb_header_assert_t[(SCB_HEADER_SIZE + SCB_PAYLOAD_MAX == SCB_SPI_SIZE) ? 1 : -1];',
                             frame))
    write('scb_frame.h', header('scb_frame.h', 'SCB帧格式、字段与字节接口。',
                               'scb_frame.c实现纯帧编解码，借用负载受调用方缓冲寿命约束。',
                               ['<stdint.h>', '<stddef.h>'], frame_types, frame))
    json = [fs[n] for n in json_names]
    json_types = '\n'.join(line for line in defines.splitlines() if 'SCB_JSON_' in line)
    json_types += '\n\n' + '\n\n'.join(type_block(public, n) for n in
                                        ('scb_json_type_t', 'scb_token_t', 'scb_json_t', 'scb_writer_t'))
    write('scb_json.c', unit('scb_json.c', '有界UTF-8 JSON解析及数字输出。',
                            '前台共享八层显式容器栈，解析至调用方令牌/字符串池。\n'
                            'writer只操作调用方缓冲并锁存溢出；不识别业务命令或访问硬件。',
                            ['"services/scb_json.h"', '<string.h>', '<float.h>'],
                            'typedef char scb_float_assert_t[(sizeof(float) == 4 && FLT_RADIX == 2) ? 1 : -1];\n'
                            + '/* 前台串行共享解析栈，不可重入；每次解析复用同一八层工作区。 */\n'
                            + 'static int16_t s_json_containers[SCB_JSON_DEPTH];\n'
                            + 'static uint8_t s_json_phases[SCB_JSON_DEPTH];\n\n'
                            + 'static int16_t SCB_ParseValue(scb_json_t *json, int16_t parent, uint8_t depth);', json))
    write('scb_json.h', header('scb_json.h', '有界JSON令牌和输出接口。',
                              'scb_json.c实现UTF-8/数字/容器校验与有界writer；解析共享栈不可重入。',
                              ['"services/scb_frame.h"'], json_types, json))
    codec = [fs[n] for n in codec_names]
    codec.append(document('编码LOG_DATA固定八字节头及原始日志块。',
                          '依次写小端offset、length和零保留字段，再复制原始字节。\n'
                          '调用方已完成范围和会话门禁，本函数不改变日志/链路状态。',
                          [('out', 'payload', '非空至少968B目标，与data不重叠。'),
                           ('in', 'offset', '日志绝对字节偏移。'),
                           ('in', 'data', '非空且至少length字节的借用原始日志块。'),
                           ('in', 'length', '原始块长度，1..960字节。')],
                          result='编码字节数8+length。',
                          note='前台在借用缓存被覆盖前立即调用；不分配第二个静态块。')
                 + '\nuint16_t SCB_EncodeLogData(uint8_t *payload, uint32_t offset,\n'
                 + '    const uint8_t *data, uint16_t length)\n{\n'
                 + '    SCB_WriteU32(payload, offset);\n    SCB_WriteU16(payload + 4, length);\n'
                 + '    SCB_WriteU16(payload + 6, 0);\n    memcpy(payload + 8, data, length);\n'
                 + '    return (uint16_t)(8u + length);\n}')
    codec_types = '\n\n'.join(type_block(public, n) for n in ('scb_status_t', 'scb_values_t', 'scb_command_t'))
    write('scb_command_codec.c', unit('scb_command_codec.c', 'SCB业务命令与参数格式适配。',
                                     '前台复用唯一命令JSON工作区，将协议字段转换为业务值。\n'
                                     '校验调用纯业务validator，命令路由/结果缓存仍由dispatch/link持有。',
                                     ['"services/scb_command_codec.h"', '"control/tuning_values.h"',
                                      '"app_config.h"', '<string.h>', '<float.h>'],
                                     '/* 命令解析由前台串行调用，避免大JSON工作区占栈。 */\n'
                                     + 'static scb_json_t s_command_json;', codec))
    write('scb_command_codec.h', header('scb_command_codec.h', 'SCB业务命令类型和格式接口。',
                                       '实现见scb_command_codec.c；只提供值转换/格式，不拥有业务事务状态。',
                                       ['"services/scb_frame.h"', '"services/scb_json.h"',
                                        '"control/tuning_values.h"'], codec_types, codec))
    assert set(fs) == set(frame_names + json_names + codec_names), set(fs) - set(frame_names + json_names + codec_names)
    return fs


def prepare_adapters():
    dispatch = (APP / 'services/command_dispatch.c').read_text(encoding='utf-8')
    mapper = document('把日志业务结果显式转换为SCB状态。',
                      '逐项转换独立结果枚举，不依赖二者数值相同；未知业务结果统一IO_ERROR。',
                      [('in', 'result', '日志开始、读取或结束操作的业务结果。')],
                      result='对应SCB公共结果，用于原RESULT编码或LOG_DATA成功分支。',
                      note='前台纯适配，不改变日志、请求表或调参事务。')
    mapper += '\nstatic scb_status_t CommandDispatch_MapLogResult(datalog_result_t result)\n{\n    switch (result)\n    {\n'
    for name in ('OK', 'INVALID_ARG', 'OUT_OF_RANGE', 'BUSY', 'NOT_FOUND', 'IO_ERROR'):
        mapper += '        case DATALOG_RESULT_' + name + ':\n            return SCB_' + name + ';\n'
    mapper += '        default:\n            return SCB_IO_ERROR;\n    }\n}'
    fs = functions(dispatch)
    old = fs['CommandDispatch_LogCommand']
    new = old.replace('    uint16_t length;', '    uint16_t length;\n    const uint8_t *data;')
    new = replace_once(new,
                       'status = Datalog_ReadExport(command->transfer_id, command->offset, command->length,\n'
                       '            s_json, &length);',
                       'status = CommandDispatch_MapLogResult(Datalog_ReadExport(command->transfer_id,\n'
                       '            command->offset, command->length, &data, &length));')
    new = replace_once(new, '            (void)SCB_QueueResult(id, SCB_LOG_DATA, s_json, length, 1);',
                       '            length = SCB_EncodeLogData(s_json, command->offset, data, length);\n'
                       '            (void)SCB_QueueResult(id, SCB_LOG_DATA, s_json, length, 1);')
    new = new.replace('status = Datalog_BeginExport(&export);',
                      'status = CommandDispatch_MapLogResult(Datalog_BeginExport(&export));')
    new = new.replace('status = Datalog_EndExport(command->transfer_id, &total, &crc);',
                      'status = CommandDispatch_MapLogResult(\n'
                      '            Datalog_EndExport(command->transfer_id, &total, &crc));')
    dispatch = replace_once(dispatch, old, mapper + '\n\n' + new)
    dispatch = dispatch.replace('#include "services/command_dispatch.h"',
                                '#include "services/command_dispatch.h"\n'
                                '#include "services/scb_command_codec.h"\n'
                                '#include "services/scb_json.h"')
    write('command_dispatch.c', dispatch)
    link_header = (APP / 'services/scb_link.h').read_text(encoding='utf-8')
    link_header = replace_once(link_header, '#include "services/scb_protocol.h"',
                               '#include "services/scb_frame.h"\n#include "services/scb_command_codec.h"')
    write('scb_link.h', link_header)
    link = (APP / 'services/scb_link.c').read_text(encoding='utf-8')
    link = replace_once(link, '#include "services/scb_link.h"',
                       '#include "services/scb_link.h"\n#include "services/scb_json.h"')
    write('scb_link.c', link)


def main():
    sources = {}
    for name in ('datalog.c', 'datalog.h', 'scb_protocol.c', 'scb_protocol.h'):
        raw = (APP / 'services' / name).read_bytes()
        sources[name] = {'sha256': hashlib.sha256(raw).hexdigest(), 'bytes': len(raw)}
    logging = prepare_logging((APP / 'services/datalog.c').read_text(encoding='utf-8'),
                              (APP / 'services/datalog.h').read_text(encoding='utf-8'))
    protocol = prepare_protocol((APP / 'services/scb_protocol.c').read_text(encoding='utf-8'),
                                (APP / 'services/scb_protocol.h').read_text(encoding='utf-8'))
    prepare_adapters()
    manifest = {'inputs': sources, 'datalog_functions': list(logging), 'protocol_functions': list(protocol),
                'drafts': sorted(p.name for p in (OUT / 'services').iterdir())}
    (OUT / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print(json.dumps({'output': str(OUT), 'functions': len(logging) + len(protocol),
                      'drafts': len(manifest['drafts'])}))


if __name__ == '__main__':
    main()
