# Phase 03：SPI 从机软件交付与阶段审查

> 历史证据说明（2026-11-05）：本文按原日期/镜像/环境保存；旧“本轮/当前”不作为现行进度。现行状态见[roadmap](../../specs/roadmap.md)，方法见[验证指南](../guides/validation.md)。本次未重跑本文检查；本副本缺原artifacts/.local时仅保留历史报告与JSON，不声称重新核验产物。

2026-10-18，Asia/Shanghai，分支`codex/03-spi-slave`。用户已将审查节奏改为每Phase一次，阶段内连续实现/验证；同时确认G-06回收后SPI任务有界解析。Group 1按已确认资源提交`712fb41`；本批Group 2/3设计先纯规约提交`82613f6`再实施。**本端SPI服务软件已实现并接入生产启动，真实硬件验收未执行，Phase 03总完成仍不勾选。** 本批代码/证据待本次阶段审查提交。

## 生产功能及调用路径

真实调用路径为`app_main → bridge_startup_idf_run → bridge_spi_start → Core1静态任务 → spi_slave_initialize/queue_trans/get_trans_result → spi_protocol → 生产codec/session/reliable → 已配置的固定资源端口`，不靠未调用模块声明完成。

| 功能 | 生产符号 / 文件 | 已实现行为 |
|---|---|---|
| 从机与资源 | [spi_bridge.c](../../main/spi_bridge.c)、[接口](../../main/spi_bridge.h) | Core1、优先级12、6144B静态栈；SPI2_HOST/DMA_AUTO、Mode0、10/12/11/13、深度1；固定内部DMA与静态8192bit描述，不自动重分配 |
| READY/所有权 | `ready_loaded`、`ready_finished`、`run_transactions` | 应用准备→池交给驱动→排队；装载回调低、完成回调高；成功返回原描述后才回收/检查/解析/重排。ISR仅GPIO，不解析/日志/锁/网络 |
| 长度与恢复 | `run_transactions` | 严格检查驱动返回trans_len==8192；0/不足/可观察超长不解析不ACK并重排。排队失败撤回确知未接受的所有权；get失败/错描述保留驱动所有权，READY高、限定停止SPI服务，无自动复位 |
| 独立计时 | `bridge_spi_protocol_poll` | get每次阻塞至少1 tick、约10ms；超时只推进逻辑计时/发布快照，不读写描述/TX/RX。重发待办等回收后才复制到DMA；无时钟不会忙等或伪造已完成重发 |
| 握手/会话 | [spi_protocol.c](../../main/spi_protocol.c)、[接口](../../main/spi_protocol.h)，既有link_session | esp_random产生非零会话，最多4次无固定值回退；重复HELLO复用、后续事务HELLO_ACK、新会话IDLE后ready、旧帧拒绝、重握手通知。HELLO前空IDLE为session/seq=0，不冒充已建链路 |
| 可靠接管/发送 | 既有reliable_link及端口reserve/commit/cancel/peek_tx/tx_consumed | 预留槽后ACK、重复仅确认不交付、精确ACK、200ms/最多3次重发且图像字节不变；实际完整发送才起算，截短不算完成；IDLE独立确认，不制造业务结果 |
| 业务透明边界 | `bridge_spi_protocol_receive` | 正常入站IDLE/TELEMETRY/RESULT/EVENT/LOG_DATA；方向错误/非法编号不接管。request_id/业务负载原样交给固定资源端口，不生成APPLIED/SAVED/日志/设备时序 |
| 可观测状态 | `bridge_spi_snapshot`及app_main/本地串口 | 启动状态快照先发布再通知调用者；只读临界区快照含事务、截短、CRC/无效、去重、背压、遥测包/记录及丢弃、重试/重建/session/ready/栈余量。无时钟等待时每5秒串口报告内部空闲/最小堆与栈；不向业务socket打印 |

生产当前没有Phase 04/05网络消费者：默认端口**不ACK无法存储的可靠业务**，离线遥测直接丢弃并计包/记录；这是明确背压，不是假成功转发。真实固定池生命周期端口在后续阶段启动前绑定，运行期拒绝换表。测试绑定有界接管夹具验证实际接口，不把夹具当成生产网络。SPI链路本身独立于电脑/热点继续握手与交换。

## 本次实际命令与结果

仓库根目录：

```powershell
. .\ESP32Bridge\scripts\activate.ps1
python -m unittest discover -s ESP32Bridge/tests -p 'test_spi_*.py' -v
python -m unittest discover -s ESP32Bridge/tests -v
```

最终全量结果为**178项通过，0失败/0跳过**，日志`.local/p3-final-regression.log`。其中SPI边界/生命周期32项（原8项初始化更新后保留＋24项实际任务循环）；其余146项覆盖基础配置/启动/固定池、协议/JSON/独立可靠/会话、固定候选向量及Git检出哈希等。主机使用现有Clang/LLD直接编译生产C；Python只提供独立struct/zlib输入、事务/时钟调度和结果核验，没有另写一套Python桥接器代替生产代码。

覆盖：READY顺序、唯一排队描述、600次无时钟超时且描述/TX/RX哨兵不变、0/8191/8192/可见8193bit、CRC/JSON/方向错误、随机数0/重复HELLO/换会话、预留后ACK、背压后重发、可靠去重不被后续遥测覆盖、实际接口EVENT/LOG_DATA逐字节一致、未订阅丢弃归因、完整TX与坏RX独立计账、精确ACK取消待重发、三次原图像重发/耗尽重握手、截短可靠TX、排队失败撤回及get错误不回收、服务失败撤销逻辑ready。SDK调度/GPIO/驱动边界为显式替身，不能证明电气或ISR延迟。

首轮新主机夹具链接曾因遗漏test_lock/test_unlock边界定义失败，未运行用例；补齐夹具后SPI29项及全量175项通过。启动快照发布顺序复核修正后重跑175项及最终目标构建通过；随后仅补3项透明接管/坏RX计账测试（生产源码未变），最终178项通过，不重复无必要构建。

目标通过正式入口，对当前源码白名单隔离副本实际执行：

```powershell
& .\ESP32Bridge\.local\p3-g4-target\scripts\build.ps1 `
  -WorkRoot 'D:\STM32H7\ESP32Bridge\.local\p3w4' -KeepWork
```

2026-10-18 复核：对同一未提交工作树按文档入口重跑，**178项通过，0失败/0跳过**，日志`.local/p3-verify2.log`。环境入口另查明一种失败形态：从 MSYS2/Git Bash 派生的进程会被运行时重新注入`MSYSTEM`变量，SDK 的`idf_tools.py check-python-dependencies`遇此变量即拒绝（`idf_tools.py:3515`），并非 SDK 或工程缺陷；从干净 PowerShell 窗口执行或在进程内移除该变量后，激活脚本全部检查通过。测试还要求`powershell.exe`位于 PATH（标准系统目录，正常 PowerShell 窗口默认存在）。不改 SDK、不跳过任何版本检查。

使用一次性私有测试配置，部署密码未提供/未更改。最终run_id=`daaf966d99694c0b8c3197273c04436f`，**v6.0.0/esp32s3 build和size均退出0**。SDK commit=`662a3be354759d9487bf4b1a629fadb766cb1800`、dirty=false；Python3.11.15、CMake4.0.3、Ninja1.12.1、Xtensa`esp-15.2.0_20251204`。BIN304880B、image304769B、DIRAM64791B、.data12940B、.bss14272B；这些是构建静态数据，不能当成运行堆测量。

最终ELF含SPI任务/排队/回收、协议/codec/可靠/握手以及bridge_spi_snapshot入口。两个READY回调均位于`.iram0.text`、各16B；实际反汇编仅IRAM常量加载与GPIO寄存器写入，无call指令/Flash调用。静态栈6144B、SPI协议状态2216B；未把某个入口栈帧大小当作完整调用链上界。实板缓存关闭场景、DMA地址能力、内部堆及栈高水位仍须执行。

[最终证据JSON/文件清单/产物SHA256](phase-03-evidence.json)索引了ELF审计日志及所有本地产物，源码与回收文件哈希重新核对。产物本体、完整构建工作区和密码均被忽略、不提交。中间版本构建`f8dfa53c61a34b529c49f719bb891026`同样成功，但不是最终烧录索引；以最终run_id为准。构建元数据的2MB Flash配置不等于实板容量已核实，本次没有烧录。

2026-10-20补充：用户实机烧录Phase 04构建`bf24b81807e842f8b66fded4d9186e17`（dio/2MB/80m，COM3 CP210x），启动日志确认`Stage=SPI state=SUCCEEDED`、驱动初始化于Core1/优先级12/6144B栈、首个8192bit事务已排队待主机时钟、READY守护生效；实板Flash实测8192k（构建配置2048k）。运行期心跳实测内部堆`internal_free≈212892`、`internal_min≈195776`、SPI栈最低余量3296→3232B。上述为用户实机读数；波形/电气项仍待验。

## 实机待验与超长检测限制

1. 逻辑分析仪必须验证真实Mode0、READY/CS顺序、每CS8192个时钟及1024B，先约1.875MHz后约7.5MHz；频率由真实主机内核时钟核算并测量。没有波形，不标电气通过。
2. `trans_len==8192`是本端必要检查，**不能证明CS内没有额外时钟**。当前SDK HAL从S3接收长度寄存器取值，并对“配置长度-1”的完成读数作修正；硬件/驱动可能在超长时饱和。测试注入8191/8193证明的是对驱动返回值的处理，不能冒充物理少1拍/多1拍一定可检测。需真实主机超长注入与波形计数。
3. 停止时钟时，已经交给驱动的TX/RX及描述保持原状；待重发只在逻辑缓存登记，必须等实际回收后才能换TX。无时钟不能声称完成三次重发，也不能因定时到期强制收回DMA。
4. 当前SmartCar原头文件96B记录/256B头的全部24/29项偏移已只读主机编译核对，但未发现自维护代码具备SCB1/HELLO_ACK/SUBSCRIBE/LOG_BEGIN等处理入口，不能以布局相同当作SCB对端可用；不修改SmartCar。真实端到端测试受G-05能力核实约束；普通Python/socket模拟不能替代电气SPI主机，电气夹具/完整中文步骤由Phase 06交付。

## 本次阶段审查清单

新增生产spi_protocol.c/.h；修订spi_bridge.c/.h、link_session.c/.h、app_main、组件CMake；SDK边界头/实际任务C夹具与spi_host.py/test_spi_lifecycle.py；同步全部阶段状态、README、CHANGELOG、追溯和证据，完整列表见JSON及git diff（含未跟踪文件）。拟提交：`feat(esp32bridge): 完成 Phase 03 SPI 事务与协议接入`。

本阶段软件交付后统一请用户审查一次，再推进Phase 04。Phase 04 Wi-Fi/TCP/UDP、Phase 05完整队列/透明桥接/导出生命周期、Phase 06独立客户端/电气夹具/60秒负载/最终中文实机手册仍未完成；所有真实硬件项目未执行，不勾选整体阶段完成，不安装替代SDK、不push、不合并、不烧录。
