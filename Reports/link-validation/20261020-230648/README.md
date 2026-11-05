# 停车通信续测：2026-10-20

本目录接续 `../20261020-224842/` 的烧录与首次失败记录。**本次已取得三项来自 STM32 的有效业务响应，证明至少这些请求完成了电脑→Wi-Fi→ESP32→SPI→STM32及返回的往返；此前间歇故障没有被证明已修复。**

## 版本与运行状态证据

- 全仓开工状态见 `git-status-before.txt`，没有修改或烧录固件，没有改参数。
- 使用仓库包内 OpenOCD/ST-Link 读取板上 Flash。读取范围 `0x08000000..0x08013463`，78948字节；与 `SmartCar/MDK-ARM/SmartCar/SmartCar.hex` 中全部78948个定义字节一致，差异0。见 `stm32-firmware-compare.json`，原始读取 `stm32-flash-readback.bin` 本地保存。
- 调试配置明确覆盖默认会写DBGMCU/看门狗冻结寄存器的examine-end钩子，并禁用复位线配置；没有发送halt/reset/program或应用寄存器写入命令。连接前后目标均显示running。原始配置和输出保存在本目录，不将运行态分时读取声称为原子快照或波形。
- `stm32-registers.log` 首次读取到SPI错误累计较高；其后的 `stm32-sample.log` 中完成数增长，错误/超时为0，计数与此前发生变化。期间用户确认按过的是**启动键**，不是可确认的复位操作；当前不能解释计数变化的原因。
- ESP32随后两次串口样点间隔5000ms，completed `11678→12278`，short保持3300、crc保持0、session=2221876335、ready=1；证明该区间恢复完整事务，但不能据此消除之前的失败记录。
- 23:11读取实际 `g_app_key_flag/start_flag/run_flag/stop_pending/stop_reason` 均0；系统tick继续前进、目标running。当前收发缓冲均有合法SCB1头。见 `stm32-current-state.log`。

## 启动键结论

代码检查确认停车查询不需要按启动键：`App_Init`末尾执行`SCB_Init/Tuning_Init`；`App_Loop`运行、停车两个分支均执行`SCB_Service`，停车调用位于按键事件处理之前。PC13短按松开触发`App_TryStart`，长按1秒触发模式切换。当前灰度32项标定均为-1，短按起跑在标定检查处拒绝，定时运行链不会因此启动。按键不是通信启动开关，也没有重置SPI状态的分支。

## 实际测试

下列请求均使用现有SCB/1格式及标准库独立诊断脚本 `raw_queries.py`；收到可靠RESULT后显式发送IDLE ACK，没有修改正式电脑工具。

| 测试项 | 实际结果 | 状态 | 证据 |
|---|---|---|---|
| 既有客户端GET_STATUS（23:09） | session2221876335、ready=true，ESP返回帧ACK，约1268ms后TCP关闭，没有最终业务RESULT | 失败 | `api-wire.jsonl`、`query-results.json` |
| GET_STATUS（23:12） | request101，249.66ms，status=OK，running=false，mode1/gear0，run_id0，tick/control_sequence/revision均0，tune_state=DISABLED，log_valid=false | 通过 | `raw-queries.jsonl` |
| GET_PARAMS | request102，288.77ms，status=OK；完整values/effective/limits，overridden=false | 通过 | 同上 |
| GET_LOG_META | request103，646.83ms，status=OK；log_valid=false、written_records=0、header_hex空 | 通过 | 同上 |
| 板间SPI功能往返 | 三项STM32业务响应均返回，当前session294589555、ready=true | 通过（这些样本）；稳定性未通过验收 | 同上及串口日志 |
| 订阅保持、重连重订阅 | 最后一次计划执行被自动审批服务额度限制拦截，进程未启动 | 未执行 | 对话工具拒绝信息；没有生成explicit-ack通过证据 |
| 60秒运行遥测、完整Flash导出 | 未起跑；没有有效运行日志 | 未执行 | 状态/日志元信息响应 |
| SPI波形与实际SCK | 用户确认没有逻辑分析仪；仅有RCC及分频静态/寄存器依据 | 未验证 | `stm32-registers.log` |

实际参数：kp2、k2p0.00800000038、kd15、kp_v20、ki_v0.75、base_speed100、fan_run1600、fan_idle1100；values与effective相同。本轮只查询，没有发送SET_PARAMS、TUNE_ENABLE、TUNE_DISABLE或保存请求。

业务响应的归属依据：它们带正确op/request_id、可靠帧身份、有效CRC，并含STM32负责的运行状态、参数bank/limits及日志元信息；ESP32本地仅处理HELLO和SUBSCRIBE，阶段05实现对这些业务透明转发。它们不是单纯热点连接或ESP侧ACK。

2026-10-21续测时，独立struct/zlib复核本目录raw-queries.jsonl全部18个带原始字节的收发帧，0个CRC/长度/magic错误；三项结果均可定位。复核结果保存于 `../20261021-093438/previous-query-independent-audit.json`，这是历史字节复核，不是10月21日新的板卡通信结果。

## 限制与后续

前述截短及23:09业务中断的首因尚未关闭，不能将三次成功升级为完整稳定性验收。曾观察到异常RX头与SCB1的移位形态接近，但缺少波形和完整事务边界，未据此修改Mode或时序。正式客户端空闲可靠RESULT确认问题仍保留原证据，未修改实现。最后计划的显式确认订阅测试未执行；后续需在当前设备可达时重新进行。

本目录只保存调查、读取及实测证据，未启动ESP32 Phase06开发。未提交、未push；原有工作区改动保留。
