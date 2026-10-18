# Phase 02 Group 2：可靠窗口、会话与环境诊断交审

审查更新：用户已明确答复“确认 Group 2，提交并推进 Group 3”。本组经最终范围/暂存检查后本地提交；下文“待审”保留为原交审状态，不再构成当前门禁。

日期：2026-10-18，Asia/Shanghai。当前分支`codex/02-scb-protocol`。Group 1按本轮继续指令复核后提交`455f817`；本组内部接口及环境诊断先独立规约提交`16a5c92`，随后实施。G-01/G-03已分别获用户明确确认，先独立规约提交`4ed619c`，本组未越界实施其网络/事件逻辑。

## 代码与具体行为

| 文件 | 实际交付 | 边界 |
|---|---|---|
| [reliable_link.c](../../main/reliable_link.c) / [.h](../../main/reliable_link.h) | 独立实例seq/ack/计时、固定可靠TX与RX身份、窗口1、精确ACK、先预留再ACK、去重、200ms/三次重发、近回绕重建 | 首次和重发均须完整发送完成后起算；未完成发送不靠重发改写缓冲。输出重建动作，未执行真实socket关闭/SPI驱动重启 |
| [link_session.c](../../main/link_session.c) / [.h](../../main/link_session.h) | SPI HELLO→HELLO_ACK→新session IDLE、重复HELLO复用、旧会话拒绝；TCP HELLO状态快照；G-02立即撤销旧连接业务许可并置关闭待办 | 生产RNG由未来IDF适配注入，0/旧值拒绝；本组未用固定随机数冒充实机。回复供后续SPI事务发送 |
| [json_guard.c](../../main/json_guard.c) / [.h](../../main/json_guard.h) | 转义字符串相等、精确u32读取；识别等价的protocol=1（含小数/指数写法），不接受布尔或字符串 | 固定资源，无locale/动态分配，不解释车辆参数 |
| [check_config.py](../../scripts/check_config.py) | 工具启动失败报告工具名+OS错误号，保留异常链，不输出配置/原始异常内容 | SDK/工具版本与Python依赖检查全部保留；无自动安装/切版/绕过 |
| [main/CMakeLists.txt](../../main/CMakeLists.txt) | 将两个新模块加入真实目标组件编译 | app_main仍仅基础启动，后续03/04/05尚未接入生产收发路径；当前不是可用桥接固件 |

接管接口的`reserved`必须表示已取得固定槽位所有权，不能传容量快照。返回DELIVER后，调用者须在下一次ACK生成前完成该槽的复制/提交；重复/失败清空输出视图。重发图像包含原ACK，之后ACK推进只通过新帧传递。相同request_id但新可靠身份仍分发，业务幂等由STM32处理。

每实例固定身份/TX图像各1024B，其余为计时/序号元数据；不属于DMA/业务排队槽，最终生命周期资源预算须在Phase 05接入时核对。状态机实例由单一所有者串行调用，不假装线程安全。API不会访问GPIO、驱动或socket，不生成APPLIED/SAVED/日志内容/设备时序。

## 本次真实验证

从仓库根目录执行（均为本次结果）：

```powershell
. .\ESP32Bridge\scripts\activate.ps1
python -m unittest discover -s ESP32Bridge/tests -p test_reliable.py -v
python -m unittest discover -s ESP32Bridge/tests -p test_sessions.py -v
python -m unittest discover -s ESP32Bridge/tests -v
```

| 验证 | 实际结果 | 证据/限制 |
|---|---|---|
| 存量复核 | 104项全部通过、无跳过 | `.local/recheck-verified.log`；原历史104项结果保留，未冒充本次 |
| 可靠状态机 | 18项通过 | 生产C经现有Clang/LLD生成本机DLL；独立Python struct/zlib构帧、注入时钟 |
| 会话及字段 | 16项通过 | 同一生产C；独立解析输出字段/CRC，另含2000组Decimal整数差分 |
| 全量回归 | **139项全部通过，0失败/0跳过** | `.local/p2-g2-regression.log`；原104+34+1项工具失败诊断用例 |
| 目标 | **v6.0.0 / esp32s3 build与size均退出0** | 当前源码白名单隔离副本，正式build.ps1入口；run_id=`8939b9db003c483d86c48fb07a99d0cb` |
| 构建范围 | 两个新模块真实目标编译及归档 | 最终基础app未引用，链接器可剔除；BIN大小不是未来协议服务RAM/Flash预算 |
| 实机 | **未执行** | 未烧录；无实板DMA、READY、堆/栈、Wi-Fi、电气SPI、吞吐或三端联调数据 |

目标命令为：

```powershell
& .\ESP32Bridge\.local\p2-g2-target\scripts\build.ps1 `
  -WorkRoot 'D:\STM32H7\ESP32Bridge\.local\p2w2' -KeepWork
```

该隔离副本由当前工程白名单复制，使用一次性私有测试配置；部署密码未提供、未更改。SDK commit=`662a3be354759d9487bf4b1a629fadb766cb1800`、dirty=false；Python3.11.15、CMake4.0.3、Ninja1.12.1、Xtensa`esp-15.2.0_20251204`。BIN272800B、size image272677B；完整产物索引及SHA256见[证据JSON](phase-02-group-2-evidence.json)，产物本体保持忽略、不提交。

## 环境入口的实际失败原因

本轮受限环境中`activate.ps1`失败，可重复追踪到`subprocess.run`启动推荐Xtensa工具时抛`WinError 623`（Windows进程/DLL装载拒绝），没有进入工具版本比较。旧入口将OSError合并为`Environment check failed`，隐藏了失败位置。修复后同一受限入口明确输出`Cannot start build tool: xtensa-esp-elf; OS error=623`及保留检查的提示。

同一个激活脚本、同一SDK与工具在沙箱外通过全部版本和依赖检查；本次139项回归与正式目标构建均在该核验环境执行。无需替换SDK、关闭版本检查或篡改工具。受限进程的首次104项复跑为1失败/7跳过（junction创建受限、未激活时CMake测试跳过），保留`.local/recheck-before.log`，不计为通过。Windows加载器具体DLL内部原因未独立确认，不推断工具损坏。

## 审查、文件清单与未完成范围

13个代码/测试文件：main/CMakeLists.txt、main/json_guard.c/.h、main/reliable_link.c/.h、main/link_session.c/.h、scripts/check_config.py、tests/protocol_host.py、tests/test_config.py、tests/link_host.py、tests/test_reliable.py、tests/test_sessions.py；配套报告、证据和各入口/规约回填清单见证据JSON及`git diff --stat`（新文件须一并检查）。

拟提交信息：`feat(esp32bridge): 完成 Phase 02 Group 2 可靠窗口与会话`。本组实现未暂存、未提交，等待用户审查；允许后再提交并推进Group 3。依据[目录规则](../../AGENTS.md)“每组验证后交用户审查，确认后方可提交并推进下一组”，不把139项通过解释为02～06完成。

Group 3固定候选向量尚未建立；Phase 03真实SPI、04网络、05队列/透明桥接/故障恢复、06客户端/电气夹具/60秒负载及最终中文实机手册均未完成。G-01/G-03方案已确认，不再列为设计未决；本组剩余推进依赖逐组审查。STM32完整能力与所有实机项继续待验，主机成功不能替代。中文实机手册必须跟最终适配及真实工具一起交付，本组未用虚构命令填充一份“完整手册”。
