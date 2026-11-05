# Phase 02 Group 1：编解码与 JSON 校验

## 2026-10-18本轮复核

按用户本轮继续维护指令复核存量后提交本组；下文为原交审时记录，不将其历史构建当作本轮构建。当前核验后的同一v6.0.0环境重新运行104项全回归，全部通过、无跳过，日志`.local/recheck-verified.log`。此前受限执行重跑为104项中1失败、7跳过（junction创建受限、未激活时CMake用例跳过），日志`.local/recheck-before.log`；直接环境入口失败的具体异常为启动工具进程时WinError 623。相同入口在沙箱外所有版本和Python依赖检查通过，不换SDK、不跳过检查。后续诊断修复单独规约先行。

记录日期：2026-10-18，Asia/Shanghai。当前分支 `codex/02-scb-protocol`，从已包含全部规约及Phase 01的基线继续，未使用旧main。Phase 01 Group 3已按本次继续指令提交为 `1b5b9fe`；用户明确确认的G-02决定随后独立纯规约提交为 `c524638`。本组实现基于原已提交02/FR-1～FR-3、TC-1及plan允许的内部签名细化，不改变共享线格式，不包含G-02对应实现。

## 实际交付

| 需求 / 验证 | 文件及符号 | 行为与边界 |
|---|---|---|
| FR-1 / V-1 | [protocol.c](../../main/protocol.c) `scb_encode`、`scb_decode` | 显式小端字段、独立CRC、完整帧检查；SPI严格1024B并发送零填充；TCP/UDP不带填充 |
| FR-2 / V-2 | `header_check`、`payload_check` | 8种类型及RELIABLE/record_count、IDLE空负载、1/10条遥测、每条记录14字节reserved、run_id和首tick；LOG_DATA长度/保留字/偏移加长度溢出；不解释车辆参数 |
| FR-3 / V-3 | [json_guard.c](../../main/json_guard.c) `bridge_json_validate`、`bridge_json_object_get` | 完整UTF-8、语法、转义后重复键、数值溢出和全量消费；全部合法后才返回字段视图 |
| TC-1 / V-8本端 | `record_layout_witness_t`及编译断言 | 头长/全部字段偏移、binary32、96B记录的24项字段偏移；只作布局见证，从不强转或发送结构体 |
| V-8对端只读取证 | 当前 `SmartCar/App/datalog.h` | 原头文件主机编译24项完整记录字段偏移及96B/256B尺寸通过；不修改对端，不代表STM32目标构建、SCB能力或实机兼容性 |

内部接口和所有权见 [protocol.h](../../main/protocol.h)、[json_guard.h](../../main/json_guard.h)。输入视图借用调用者缓冲，调用者在归还驱动/重用socket缓冲前必须复制或移交所有权。失败清空view/used，不返回部分帧；编码失败不改目标缓冲，拒绝输入负载与输出重叠。每次解析独占调用者固定工作区，主机实测 `sizeof(bridge_json_workspace_t)=1492` 字节（类型大小，不是实机堆测量）；允许992B以内最深496层空容器，无递归/全局状态/动态分配。重复键有界扫描，最坏输入受992B限制，耗时尚未实机测量。

数值只按binary64有限范围校验，不转换车辆值。使用十进制位与精确溢出舍入边界 `2^1024-2^970` 比较，无libc数值转换、locale或隐式分配依赖；允许有限下溢到零。所有控制含义、命令字段校验/执行、ACK推进、可靠重发、会话与socket动作留在对应组/阶段。无空实现或固定成功替代。

## 本次实际执行

```powershell
. .\ESP32Bridge\scripts\activate.ps1
python -m unittest discover -s ESP32Bridge/tests -p test_protocol.py -v
python -m unittest discover -s ESP32Bridge/tests -v
# 在本组白名单隔离副本执行正式入口；真实路径和命令见证据JSON。
& '<隔离副本>\scripts\build.ps1' `
    -WorkRoot 'D:\STM32H7\ESP32Bridge\.local\p2w' -KeepWork
```

| 类别 | 结果 | 证据与限制 |
|---|---|---|
| 协议生产C主机检查 | 27项通过，0失败/跳过 | 真实本机Clang18.1.7编译生产C、LLD20.1.1链接DLL；Python独立struct/zlib参照；不是C编解码器自证往返 |
| 差分子场景 | JSON 1200组、数值1000组通过 | 固定随机种子；另含精确溢出边界±1、UTF-8/转义重复键、496层容器、全部遥测reserved字节、逐字节坏帧等；是本组测试输入，尚非共同固定向量基线 |
| 全量主机回归 | 104项通过，0失败/跳过 | 原77＋新增27；本次真实执行，日志 `.local/p2-g1-regression.log` |
| ESP32目标 | v6.0.0 / esp32s3 build与size均退出0 | SDK `662a3be354759d9487bf4b1a629fadb766cb1800`，Python3.11.15、CMake4.0.3、Ninja1.12.1、Xtensa esp-15.2.0_20251204；SDK未改动 |
| 目标对象检查 | 新模块真实编译且归档，全部静态断言通过 | nm确认scb_encode/decode、JSON入口；未定义符号仅JSON入口/memcpy/memset，无malloc/strtod；最终应用尚未调用协议模块，链接器剔除未引用代码，不能据BIN大小推断未来协议服务内存 |
| 本次产物 | BIN272800B；报告image272677B | ELF/map/bootloader/分区表/烧录元数据/size/log/manifest已回收并留哈希；见[证据清单](phase-02-group-1-evidence.json) |
| 实机 | 未执行 | 未烧录、无堆/栈/时序/吞吐、无真实网络或三端联调 |

目标构建使用被忽略目录中的一次性测试配置；实际项目部署密码仍缺失，不输出或提交密码。全部构建/产物/本地日志留在ESP32Bridge忽略目录，不复制其他维护目录，不修改SDK、不安装工具、不push。

本次最初坏CRC测试夹具误将CRC重算恢复，修正为保留损坏值后27项及104项全回归通过。首次目标构建实际失败：深层WorkRoot使SDK mbedTLS依赖文件路径过长；诊断和工作区保留。只改用正式入口已有的短WorkRoot参数后完整构建通过，没有删检查或修改SDK。SDK缺ESP_ROM_ELF_DIR的gdbinit警告仍存在，不影响本次build/size；不宣称完整SDK export/调试环境通过。

## 审查与后续

Group 1代码与本次验证回填尚未暂存/提交。完整文件清单、源码/产物哈希、实际命令、成功及失败记录见 [证据JSON](phase-02-group-1-evidence.json)。建议中文提交信息：`feat(esp32bridge): 完成 Phase 02 Group 1 编解码与 JSON 校验`。

按保留的分组/逐阶段审查门禁，确认本组后再提交并推进Group 2可靠窗口/握手；G-02方案虽已确认并独立提交，关闭动作尚未实现。Group 3固定候选向量及三端对齐未完成；G-01/G-03仍待分别确认。Phase 02整体、Phase 03～06均不标完成。Phase 01实机验收继续待验，不以本组独立软件验证冲抵。
