# 第一、二组：桥接检查与单请求槽交付

日期2026-11-07；纯规约0764779；实现前基线32b53ac及既有注释增量。依据[当前设计](../../specs/features/05-bridge-lifecycle/single-request-design.md)，本组覆盖P02检查、P05信用与电脑接入；SmartCar只更新公共说明与Phase16文档。第三组未实施，各阶段实机/整体验收保持打开。

## 实际变化

1. protocol::payload_check移除遥测前缀非零/首tick一致/记录reserved扫描及LOG_DATA reserved零阻断。头/记录数/实际长度/CRC、LOG_DATA长度1～960和offset加法仍检查；不新增诊断扫描。
2. json_guard::number只消费数字语法，删除binary64指数与309位边界比较。JSON字面量、UTF-8、重复键、完整消费、有界深度保持；u32继续精确转换/逐位溢出，负号仍拒绝。NONFINITE历史编号保留，不再产生。
3. bridge_state删除3类信用和command_class，改为单credit、单COMMAND、两个RESULT。命令预留/SPI借用改布尔；结果索引0中间、1最终。最终精确ACK释放信用，RECEIVED不释放；bitmap只用bit0。
4. 本地活动导出/10秒/GAP全部保持。expire_export只撤销当前导出请求，不能误清单槽中的普通请求；断线已交SPI的未明请求形成唯一墓碑，全部新转发命令背压到SPI新会话，迟答不解除，无自动复位/重放。
5. Tools/SmartCarLink统一全部命令的本地请求槽，原子分配编号后占用；编码先于占槽。请求超时仅结束调用等待，不解除未明请求；迟到最终仅匹配编号释放，断线清资格保留记录。客户端解析合法RESULT/EVENT/LOG_DATA后发送独立IDLE ACK，再解除对应请求并唤醒；重复帧只补ACK。坏负载关闭而不确认。Session构帧/接收/轮询/断线共享锁，socket和业务等待在锁外。

## 逐项静态审查

| 项目 | 具体结论 |
|---|---|
| 容量与索引 | COMMAND数据1024B，RESULT两槽2048B，GAP1024B，共4096B有编译断言；旧10240B减少6144B。结果遍历界BRIDGE_RESULT_SLOTS=2，唯一活动request匹配决定槽0/1，没有旧kind/class乘除索引 |
| 接管及错误 | 真实reserve→可靠receive→DELIVER同步commit，否则cancel；内存复制长度仍来自完整decode。COMMAND编码失败保留、撤销借用只置revoked，归还才释放。session/方向/request/ACK/DMA检查保留 |
| 普通请求 | active或tombstone即背压，不再承诺查询旁路。最后RESULT未被TCP精确ACK前不接受下个转发请求；CPU/UDP/EVENT独立，不在锁内等业务 |
| 导出超时 | 原三类结构有独立导出位置；新单槽先判断active且operation非0再撤销。只有导出被取消才清其结果待确认索引，普通GET_STATUS等不因之前导出到期被撤销 |
| 断线/重连 | 借用期间epoch/revoked检查保持。已发请求变唯一墓碑，旧请求响应只取消/孤儿计数，不交新连接；未发送命令可撤销，新SPI会话才清墓碑。重连查询可能无法前进是批准后的明确取舍 |
| 内容职责 | STM32生产继续规范零保留字段；车辆有限性/范围/Flash/停车实现不改。电脑原日志reserved与调参范围检查保留；桥接更宽的接受范围不宣称消费端必然接受 |
| 数字安全 | number不计算巨大指数，避免全局浮点转换；u32独立保留指数有界累加、有效位乘10前检查、power范围0～9，无舍入/截断。NaN/Infinity仍语法不合法 |
| 电脑ACK与单槽 | Session先识别可靠身份，客户端完整解析消费负载，再独立ACK成功后发布结果。匹配request才清pending；RECEIVED与超时保留。原始输入坏JSON/LOG关闭，避免已ACK但无法交付结果。Session锁内无socket/等待，读线程不等待应用最终结果 |
| 模块化 | 删除command_class、类别常量和多类遍历；保留command_metadata仅承担第三组尚未迁移的导出关联。已验证入口继续复用，未新增抽象层或任务 |
| 格式/命名/注释 | 新改C延续4空格/独立大括号、中文Doxygen和owner说明；旧数字有限性/保留字段注释同步修正。新行100字符范围及git diff --check核对；不清理无关既有源码注释工作 |
| Docs/Phase | 更新帧/JSON/资源/诊断/网络/桥接、公共communication、电脑README、追溯和阶段记录；旧实验/向量及历史验证保留，不用新接受行为改写旧结果 |

## 构建与静态验证证据

正式入口：既有scripts/activate.ps1及scripts/build.ps1；SDK6.0.0、esp32s3、双核、禁PSRAM，使用既有本地忽略密码，不读取/打印密码值。当前进程使用已安装SDK推荐ESP_ROM_ELF_DIR=C:/Espressif/tools/esp-rom-elfs/20241011/。受限环境启动编译器OS error623，获准后同一入口完成，未安装/换版。

run_id=e7a2d485bb2742f799a7b0e3a515c1c2；manifest success/complete，build、配置/依赖、size、产物回收均通过。SDK提交662a3be354759d9487bf4b1a629fadb766cb1800，Xtensa esp-15.2.0_20251204、Python3.11.15、CMake4.0.3、Ninja1.12.1；脱敏build.log未发现error:/warning:/CMake Warning。应用bin为0xC3640字节。

构建期间留存正式工作副本main到.local/group12-built-source。当前bridge_state.c仅空白整理、json_guard.c仅说明修正导致字节哈希不同；全部修改C/H去注释和空白后的可执行源码与正式副本一致，其他main源文件哈希一致。没有构建后逻辑变更，不把注释差异伪称字节一致。
电脑scb_client.py使用Python compile(source, path, 'exec')进行语法编译，不导入或执行模块、不产生模拟输入。全部验证是静态核对和目标正式构建，未新增/运行宿主、单元、Python/C驱动或模拟对端测试。

## 仍待真实设备验证

单命令→RECEIVED→最终结果→独立ACK→下一命令；参数非法的STM32拒绝；纯语法大指数经ESP32转交；遥测保留内容放行而结构/CRC错仍拒绝；普通命令占槽时导出到期不误取消；最终迟答、断线全局墓碑、新SPI会话恢复；日志offset/length关联、遥测丢旧保新和EVENT/GAP。以上没有实测，不宣称吞吐/延迟或三端联调通过。未烧录、复位或push。

## Git范围

开始既有20份main源码/头文件700行注释增量、未跟踪注释报告及Tools/STM32Flash保留。保存本轮main基线后，提交补丁剔除既有注释增量（旧JSON数字说明按新行为修正，但仍归既有未提交注释块）；只提交本轮五份C/H、电脑客户端和匹配文档。SmartCar无C/H/工程配置改动，无需本轮R08或STM32构建。
