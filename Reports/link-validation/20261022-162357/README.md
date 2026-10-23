# 实机续测：2026-10-22

本轮北京时间16:23开始。**当前整链路仍未通过；持续截短定位到STM32 SPI/DMA异常恢复路径。** 首轮只读调查后，用户人工RESET一次仍失败；随后按先规约独立提交、再实现顺序准备STM32 R2修复。新HEX已构建，尚未烧录。未改参数、未按启动键、未自动复位。

## 当前连接与版本

- 电脑连接SmartCar-2201，WPA2-Personal/CCMP，IPv4为192.168.4.2/24、网关192.168.4.1；ESP32 CP210x COM3已枚举。见 `wlan.txt`、`ipconfig.txt`、`serial-inventory.json`。
- STM32通过同一包内OpenOCD/ST-Link读取，配置覆盖原target脚本的examine-end写钩子；没有发送halt/reset/program或应用寄存器写入命令，读取前后目标均显示running。配置为 `stm32-diagnosis.cfg`，日志为 `stm32-diagnosis.log`。
- 本轮再次读取STM32 Flash的78948字节，与现有Keil HEX全部定义字节逐字节相同，差异0；HEX SHA256仍为 `db3877863e195a703dcd2d598ce8be9c4096526fdd1c09525ce2f10bf56c6a14`。比较见 `stm32-firmware-compare.json`。
- 当前起停五个标志 `key/start/run/stop_pending/stop_reason` 均0，处于停车状态。上电板卡不以按启动键作为通信前提。
- ESP32本轮没有读取新的启动版本字符串；历史10月20日已烧录Phase05最终产物及校验证据继续保留，当前不能仅凭AP名称重证镜像身份。实际串口为Phase05桥接统计格式。
- 全仓已有ESP32Bridge等在途改动保留，见 `git-status-before.txt`；后续仅修改本任务STM32 BSP、夹具及其规约，Reports保存证据和对话导出。共享检出及原暂存区保持，独立本地功能分支存放提交。

## 实际测试表

| 测试项 | 本轮实际结果 | 判定 | 证据 | 剩余问题 |
|---|---|---|---|---|
| Wi-Fi/电脑地址/串口枚举 | 正确连接目标热点，192.168.4.2，COM3存在 | 通过 | `wlan.txt`、`ipconfig.txt`、`serial-inventory.json` | 不等于SPI或业务通过 |
| HELLO/HELLO_ACK | CRC有效，ESP报告session2666712842、link_ready=true | 通过（仅网络握手） | `recovery-wire.jsonl` | STM32读到session=0，表明ESP标志不能证明当前仍有有效SPI通信 |
| GET_STATUS | request_id=1792657479；约9ms收到ESP传输ACK，等待5秒没有最终业务RESULT | 失败 | `recovery-wire.jsonl`、`recovery-summary.json` | 仅证明ESP接管，不证明STM32收到或执行 |
| GET_PARAMS/GET_LOG_META | 首个状态查询失败，停止依赖停车业务前提的后续步骤 | 未执行 | `recovery-summary.json` | 10月20日三项历史成功另见旧报告，不能算本轮成功 |
| SPI服务推进/截短 | 5秒内completed59201→59226，short564→589，合法协议接收34310保持不变；crc=0、retry=0 | 失败 | `esp32-passive-serial.txt`、原始bin与meta | 新增25笔回收全部截短，未进入合法SCB接收 |
| STM32异常状态采样 | 100个分时样点中TX DMA EN一直1、NDTR一直493；RX EN一直0、NDTR一直1024；SPI TX/RX DMA请求均关；成功完成增量0、错误增量12、超时增量0 | 失败 | `stm32-diagnosis.log`、`stm32-diagnosis-summary.json` | 证明存在残留TX DMA及重复失败；初次异常触发尚未确认 |
| RX头异常形态 | 当前头字节a6 86 84 62与SCB1的53 43 42 31左移一位形态相符 | 异常证据；成因未定 | `stm32-diagnosis.log` | 不能据此直接改Mode、分频或归因某根线 |
| SUBSCRIBE/保持/BUSY/主动重连 | 本次预定脚本在首个GET_STATUS失败后停止 | 未执行 | `parked_recovery_probe.py`来自上一证据目录，运行结果见本目录 | 没有把准备脚本或历史BUSY成功算作本次完成 |
| 60秒运行遥测 | 保持停车，没有触发运行 | 未执行 | 本报告 | 仍受真实标定、既有起跑条件及现场运行确认约束 |
| 完整日志dump/整体CRC/实时对照 | 没有当前有效业务状态或新日志样本 | 未执行 | 本报告 | 未产生导出文件，不虚报256B头/96B记录/整体CRC实机通过 |
| 人工STM32复位后的恢复测试 | 用户确认已复位；新session1051912663，GET_STATUS传输ACK约5.3ms但5秒无业务返回；5秒新增25笔全部short | 失败 | `post-reset-163734/recovery-wire.jsonl`、串口原始证据 | 复位未排除持续截短，不再反复复位 |
| 对话导出 | 提取截至“导出以上对话”的37条已保存可见文字消息，保留原文和后续修正 | 完成 | `../../conversation-exports/20261022-数据链联调对话.md`及同名JSON | 不包含内部推理、系统指令和工具原始输出 |

## 已证实的失败机制与限制

1. 当前STM32 `BSP_WiFi_Service` 在错误/超时路径仅调用 `HAL_SPI_Abort(&hspi2)`，随后无条件发布空闲态；没有显式保证两个DMA流均停止。
2. 锁定HAL V1.11.2中的 `HAL_SPI_Abort` 只有在SPI CFG1的TXDMAEN/RXDMAEN置位时，才对相应DMA调用 `HAL_DMA_Abort`。本轮实际读到两个SPI DMA请求都已关闭，而TX DMA流仍使能、剩余493不前进。这与该恢复路径跳过残留TX流的机制一致。
3. 同一HAL的 `HAL_SPI_TransmitReceive_DMA` 先启动RX，再启动TX；TX启动失败会中止RX并返回HAL_ERROR。BSP会拉高CS、增加错误计数。这与本轮“TX残留、RX未接收、完成数停止、错误持续增长、ESP回收截短”的组合相符。**这是有源码和运行状态支持的故障机制；首次传输为什么出错仍未被证明。**
4. `HAL_DMA_ERROR_NO_XFER=0x80` 表示无正在进行的DMA可供中止；本轮RX句柄出现此值，不能误读为DMA传输错误码或当作RX正常接收证明。
5. ESP仍报告ready=true，而STM32已重新处于HELLO/session0状态；完整事务未成功交付时，ESP缓存的链路标志可以滞后。不能用其替代STM32业务响应。
6. 没有逻辑分析仪，未验证实际SCK、每CS时钟数、READY许可和MISO时序；寄存器可确认Mode0/8bit/分频256配置，不能替代波形。用户10月20日的ESP32 3.3V静态读数属于历史，不是本轮新供电测量。

源码定位：`SmartCar/BSP/bsp_wifi.c` 的Service及TryExchange；锁定HAL的 `stm32h7xx_hal_spi.c` 中HAL_SPI_Abort和HAL_SPI_TransmitReceive_DMA。上述调查基于板上旧固件和源码；后续修复仅改STM32这一端，不同时改ESP32或电脑正式客户端、不更改公共协议/引脚/Mode。

## R2修复准备与交付

纯规约提交d28c4a4、12fe38b在先，实施提交f857145在后，均位于独立本地
`feature/16-spi-dma-recovery`；共享HEAD仍为b01ae53，未切换检出、不push。
新增BSP_WiFi_AbortDma并让Service仅在SPI中止成功、双DMA均READY且EN=0后发布空闲；
失败保持错误态、共同恢复IRQ，避免重复使用仍被DMA占有的缓冲。未修改其他端或HAL。

| 新验证项 | 实际结果 | 判定 | 证据 |
|---|---|---|---|
| 同一生产C故障夹具 | 残留TX/RX/双向、中止失败、EN未清、SPI失败、迟到回调、重试、新READY、单次超时计数及原断言通过 | 替代Clang宿主通过 | `r2-build/clang-host-summary.json`、各变体`build.json` |
| 隔离负对照 | 去启动去重和去DMA回收分别在预定断言失败 | 通过 | 同上；保留各变体源码 |
| 原native GCC/ARM GCC | gcc不存在，旧ARM GCC绝对路径不存在 | 未通过 | `r2-build/native-gcc-host-localtemp.log`、`arm-gcc-build-unsandboxed.log` |
| Keil完整构建 | AC5 5.06u5 build528，0错误/5原有警告，警告与旧版逐行一致；Code增加92B，RW/ZI不变 | 构建通过 | `r2-build/keil-build-unsandboxed.log`、`firmware-manifest.json` |
| map/风格/来源核对 | 两DMA缓冲地址各1024B及24B状态布局不变，生产C≤100字符/LF/无Tab；源文件匹配实施提交 | 通过 | `r2-build/style-check.json`、`after-SmartCar.map`、manifest |
| flash.ps1 -CheckOnly | 路径和依赖通过，不访问设备 | 通过 | `r2-build/flash-checkonly.log` |
| 新固件板上验证 | 仍未烧录 | 未执行 | 新HEX只作为待验证产物 |

旧板匹配HEX SHA256：`db3877863e195a703dcd2d598ce8be9c4096526fdd1c09525ce2f10bf56c6a14`。
新Keil HEX SHA256：`c208713e70f895fb04cbbcb8588cb2ac6831d16eaed404b914c0004e08273c0d`，
默认路径`SmartCar/MDK-ARM/SmartCar/SmartCar.hex`，备份为`r2-build/after-SmartCar.hex`。
旧HEX、旧日志及本批源码/产物哈希均已保存。受限环境首次Keil调用失败的日志也保留，
正常本机环境重试构建成功，不将失败尝试混为成功。

目前四项判定：板间SPI失败；本轮电脑双向查询失败（10月20日仅有历史成功样本）；
实时遥测未执行；完整日志导出未执行。需要先获得STM32固件更新授权，再通过统一入口
写入/校验/复位，随后实际重跑停车查询与DMA推进；不能把本节构建/宿主通过当作实机通过。
