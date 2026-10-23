# SPI/Wi-Fi 实机联调记录

日期：2026-10-20，北京时间；本轮从 22:48 开始。结论：**ESP32 Phase05 已实际烧录并通过写入校验；整链路未通过，当前首个有实测证据的失败层是 SPI 事务长度检查。**

后续进展：当日23:12已取得三项STM32业务有效响应，见[停车续测报告](../20261020-230648/README.md)；此前失败及稳定性问题仍保留。10月21日[新一轮前置检查](../20261021-093438/README.md)发现电脑尚未接回SmartCar热点，不能把上述历史成功当成本轮新实测。

## 范围与版本

- 开工 HEAD：`b01ae53`（完整值见 `git-head.txt`）；共享分支 `feature/02-workspace-flash`。开工已有 ESP32Bridge Phase05 的修改/新增文件、Keil 产物与日志、`.vscode/settings.json`，全部保留。本次仅新增本证据目录及定位指针，不修改实现、不合并分支、不 push。
- 已阅读根目录、SmartCar、ESP32Bridge 的 AGENTS，按各目录顺序阅读 mission/techstack/roadmap，另读代码标准、STM32 Phase16 及 ESP32 Phase03/04/05 的需求/计划/验证、SCB/1、Phase05 报告、主机工具和烧录说明。未启动 Phase06，也未将主机回归或构建当成实机验收。
- 烧录前 ESP32 的精确镜像未知。此前已烧录旧镜像的历史记录不足以证明当时仍在运行同一镜像；本次只记录实际表现：HELLO 返回 session `2690106270`、ready=true，但业务查询无 ACK/RESULT，串口截短计数持续增长。
- 用户在本轮明确要求重新烧录 ESP32。目标采用已有最终产物 `ESP32Bridge/artifacts/e86b5d91a3e84e3e91b91e17a049b510/`，全部 manifest 源码输入与当前工作区相符，全部回收产物 SHA256 重新核验相符。未重新编译、安装或切换 SDK。
- ESP32 目标对应阶段实现分支提交 `b45ef7e`，但共享检出的 HEAD 仍为上述固件烧录工具分支；不能只用共享 HEAD 标识 ESP32 源码。`firmware-audit.json` 保存逐项输入哈希。
- ESP32 BIN SHA256：`ab4bd7c19ed3253ba0a2dd87372aa6632d34763e67b3a8bbb08ac397e00a44c2`；ELF SHA256：`ba2d889ee3018eb7d55efbb0106aa31d33af1989b535ca80e41738ff74fc3899`。
- 现有 SDK 为 ESP-IDF 6.0.0（SDK tag `v6.0`），已有 Python3.11.15 / esptool5.5.0；实际激活脚本核验版本。启动实测为 `0.1.0-dev`、编译时间 `Oct 20 2026 22:05:43`、ELF前缀 `ba2d889ee...`，与目标一致。
- STM32 没有重新构建或烧录。已有 Keil AC5 V5.06 update5 build528 日志为 **0错误、5警告**，不能标为零警告。HEX SHA256 `db3877863e195a703dcd2d598ce8be9c4096526fdd1c09525ce2f10bf56c6a14`；两份 Keil 下载日志均报告失败。板上实际 STM32 镜像尚未精确确认。

## 接线、电气与 SPI

规约与工作区配置一致：CS PB12→GPIO10，SCK PB13→GPIO12，MOSI PB15→GPIO11，MISO PB14→GPIO13，READY PA15→GPIO14（低有效），EN PD15→EN，共地、3.3V 逻辑。用户已说明完成实际接线；本轮尚未逐线测量，不能认定实物引脚、电平和供电已全部核实。

静态 STM32 配置为 Mode0、8bit、MSB first、全双工、软件CS，PLL1Q 按25MHz HSE及M5/N192/Q2计算为480MHz，初始分频256，即 **配置推算1.875MHz**。未启用快速分频；没有运行中 RCC 读数或实测 SCK，不能将推算值写成测量值。ESP32 启动声明8192bit、Core1、优先级12、6144B栈。

用户确认现场没有逻辑分析仪。CS内8192个SCK、Mode0实际波形、READY一次许可一笔事务和实际频率均未验证。实机串口只证明驱动持续回收事务及长度检查失败，不能据此确认 DMA/接线正确。用户随后用万用表报告 ESP32 3V3 对 GND 为 **3.3V**；这是现场静态读数，不包含纹波或瞬态测量。

烧录前串口三个样点：completed `112527→112552→112577`，short `103649→103674→103699`；相邻样点相隔5000ms，每段新增25笔完成均为截短。对应 `preflash-esp32-passive-serial.txt`。

新镜像启动后：completed `25→50`，short `25→50`；session=0、ready=0、协议接收数SPI=0、crc=0。恢复PD15→EN后：completed `1050→1075`，short `1050→1075`，仍session=0/ready=0。对应启动日志及 `en-restored/esp32-passive-serial.txt`。工作区 `spi_bridge.c` 的 short 仅在 `trans_len<8192` 时递增；具体实际位数未记录，不能猜为8191、512B或归因于某一根线。

## 烧录经过

第一次 COM3 下载握手失败：`No serial data received`，未写Flash。随后用户按步骤断开PD15→EN，第二次使用同一配套 `flash_args` 成功写入bootloader、分区表、应用，三项均 `Hash of data verified`，退出0，工具执行授权更新后的复位运行。

第二次实测芯片为ESP32-S3 revision v0.2，MAC `80:45:6b:2d:22:00`。AP实际BSSID尾字节为2201，SSID为SmartCar-2201。未根据芯片检测的PSRAM特征更改项目禁用PSRAM的配置。

用户随后确认接回PD15→EN；恢复接线后的串口及网络均已复测。没有反复复位以掩盖SPI故障。被动串口采集为115200、DTR/RTS预置false、不写任何字节；保留原始bin及文本。烧录时的复位与这些被动读取分别记录。

## 测试表

本表证据路径均相对于本目录。失败的业务请求没有有效响应内容。

| 测试项 | 实际结果 | 判定 | 证据 | 剩余问题 |
|---|---|---|---|---|
| 仓库状态/版本取证 | 保存已有状态、源/产物哈希；原有在途文件保留 | 通过 | `git-status-before.txt`、`firmware-audit.json` | STM32板上版本未知 |
| ESP32更新 | 第二次写入及三项校验成功，启动版本匹配目标 | 通过 | `esp32-flash-attempt2.log`、`esp32-passive-serial.txt`、`preflash-artifact-hashes.json` | 第一次握手失败未写入；EN影响仅为线索 |
| Wi-Fi/地址 | SmartCar-2201、WPA2-Personal/CCMP，PC192.168.4.2/24，网关/DHCP192.168.4.1 | 通过 | `wlan-native.txt`、`ipconfig.txt`、`postflash-wlan.txt` | 与SPI成功无关 |
| TCP/HELLO | 原固件非零session/true；更新后及恢复EN后session0/false，CRC有效 | 通过（只限网络握手） | `preflash-tcp-smoke.json`、`postflash-tcp-smoke.json`、`en-restored/tcp-smoke.json` | 新镜像SPI尚无有效会话 |
| 连接保持/BUSY/重连 | 主连接至少2.2秒保持；第二连接BUSY后EOF；主连接仍开；主动断开后再次HELLO可用 | 通过（只限网络） | `en-restored/tcp-smoke.json` | 未证明主连接的STM32业务可用 |
| 板间SPI | 连续5秒区间25笔完成全部截短；新镜像无协议有效接收/会话 | **失败** | `preflash-esp32-passive-serial.txt`、`esp32-passive-serial.txt`、`en-restored/esp32-passive-serial.txt` | 需电气/波形或STM32运行态证据，具体trans_len未知 |
| GET_STATUS（更新前） | request1，995.28ms，三次重发后LinkLost；request3约1016.37ms；重订阅后request6约982.83ms，同样无ACK/RESULT | **失败** | `query-results.json`、`diagnostic-results.json`、对应wire文件 | 无可归属于STM32的业务结果 |
| GET_PARAMS（更新前） | 显式只读mode1/gear0，request1，998.55ms，三次重发后LinkLost | **失败** | `diagnostic-results.json`、`diagnostic-wire.jsonl` | 未得到values/limits/effective |
| GET_LOG_META（更新前） | request2，1012.76ms（包含重连），三次重发后LinkLost | **失败** | 同上 | log_valid及written_records未知 |
| 更新后既有params CLI | 握手得到session0/false，客户端拒绝进入业务，退出2 | **失败（前置条件未满足）** | `postflash-params-cli.txt` | 没有强行发送未就绪业务 |
| SUBSCRIBE/UDP5001（更新前） | request4返回OK，约27.58ms（包含重连）；再次连接request5返回OK，约5.46ms | 通过（仅本地订阅应答） | `diagnostic-results.json`、`diagnostic-wire.jsonl` | ACK保持存在独立电脑端缺陷；不能证明上传路径通过 |
| 停车UDP观察 | 监听10.166秒收0包，客户端坏包/溢出等计数均0；订阅后约0.86秒TCP被关闭 | 未完成上传验收 | `diagnostic-wire.jsonl` | 无STM32状态，未确认实际产生遥测；监听全时段不等于有效订阅全时段 |
| 实时遥测60秒/1kHz | 未启动运行，未取得有效遥测记录、区间或各端丢弃对照 | **未执行** | 本报告 | 标定表仍32个-1；现场运行未获确认，SPI未通过 |
| 完整日志dump/总长/CRC32 | 未进行LOG_BEGIN/READ/END，没有Flash导出产物 | **未执行** | 本报告 | 无有效会话/停车收尾与日志元信息，不能验证256+96×N及整体CRC |
| 同运行实时/Flash对照 | 无共同运行样本，不能按run/index/tick对照 | **未执行** | 本报告 | 前两项未完成 |
| 恢复后的SPI推进 | 网络连接/断开期间SPI计数继续增长；均截短 | 通过（仅继续推进） | 串口日志及握手时间线 | 有效通信未恢复 |
| 不重放/不回滚参数 | 本轮没有发送SET_PARAMS/TUNE_ENABLE/TUNE_DISABLE/SAVE；无生效变更可比较 | **未执行行为验收** | 独立帧审计 | 不能以未修改参数证明真实回滚测试通过 |
| 供电/电平/逐线/波形 | 用户实测3V3对GND为3.3V；确认无逻辑分析仪，未采波形 | 静态电压通过；其余**未执行** | 本报告中的现场反馈 | 未测信号电平、共地及供电瞬态；不将规约接线当实物测量 |

## 定位与独立问题

1. **当前最靠下的已证实失败层：READY/SPI/DMA中的事务长度检查。** 从机驱动不断回收，但返回长度小于8192bit，帧在SCB解码前被拒绝；crc=0不代表线数据CRC全部正确，而是这些截短帧未进入正常解析。供电、连线、CS/SCK实际长度、主机错误/10ms中止、从机采样及DMA仍需进一步区分。没有同时改动两端，也没有提高SCK。
2. **更新前ESP32转发不可确认。** 三项STM32业务均无ACK/RESULT，而ESP本地SUBSCRIBE可返回OK。旧镜像是否缺少Phase05消费者只是与历史/静态实现相符的线索，不能代替旧固件身份核验或排除同时存在的SPI故障。重新烧录后已确认消费者启动，但SPI仍失败。
3. **已有PC客户端未独立ACK可靠RESULT。** 线证据中SUBSCRIBE的同一可靠RESULT(seq1)收到四次，PC在仅观察期间没有发ACK IDLE，ESP随后关闭连接。`scb_session.py::Session.feed_frame`仅记录ack_seq，`poll`仅重发本端在途帧；没有独立生成IDLE。下一条业务可携带ACK，但空闲订阅会失败。此问题与SPI截短分别处理；本轮只取证，未修改工具实现。`explicit_ack.py`仅为待执行诊断草稿，**没有运行，不计入证据通过项**。
4. **PC零会话握手限制。** 网络HELLO_ACK的session0/false是有效连接状态回复，但既有DeviceClient.connect会以无有效session_id拒绝，所以params CLI在新固件下还未发出业务请求。不能把这个提示当成ESP没有TCP监听。

`independent-frame-audit.json`用标准库struct/zlib独立复核两份API线日志：39帧、0个CRC错误；RESULT五次均为两次SUBSCRIBE的原发/重发，**没有任何STM32业务RESULT**。请求编号以wire帧为准；一次新的客户端采用不同会话时会重置编号，本轮不同进程的request1不能视为同一个业务请求。

## 工具边界与下一步

最初沙箱网络操作返回Windows10013，系统WLAN/CIM查询也受限；获准本机访问后实际网络测试成功，这些环境错误不是板卡故障。恢复EN后的首次被动串口复测遇到自动审批超时，配套网络脚本因依赖目录未创建而未保存；已独立创建目录并顺序重试完成，详见 `en-restored-retry-note.txt`，不将工具故障算作板卡失败。

现场已完成3.3V静态测量并确认没有逻辑分析仪。后续使用现有日志、STM32运行状态和逐线核对继续定位，波形项目保持未验证，不凭现有证据推断具体截短位数。涉及修改实现时，先按所属目录编写并独立提交可判定规约；公共引脚/协议/外部行为歧义先交用户决定。未经额外指令不更新STM32固件、不启动运行、不改参数、不做故障注入。本轮证据已落盘，尚未提交。
