# Phase 16 — 实施计划

> 仅计划，所有目标符号均为后续实现契约；不表示文件或接口已经存在。
> 需求见[requirements.md](requirements.md)，验收见[validation.md](validation.md)。

## 调查基线与关键证据

基线 `84adaf0`，开工全仓库状态干净，当前分支 `feature/16-spi-wifi-link`。

| 现状位置（均相对SmartCar） | 只读观察 | 计划影响 |
|---|---|---|
| Core/Src/spi.c::MX_SPI2_Init/HAL_SPI_MspInit | 已有Mode0/8bit/软件NSS、分频32，无SPI2 DMA接入 | 增量接DMA、IRQ及两个联调速率 |
| Core/Src/main.c::SystemClock_Config | HSE25MHz，PLL M5/N192/Q2，按源码计算PLL1Q=480MHz | SPI2核实源及分频256/64对应1.875/7.5MHz；运行寄存器与测量确认 |
| SmartCar.ioc::RCC.SPI123Freq_Value | 记录240000000，与上项源码计算不符 | 校正派生时钟记录，不通过改共享PLL凑频率 |
| Core/Src/dma.c::MX_DMA_Init、adc.c | DMA1 Stream0用于ADC，源码未见Stream1/2实际分配 | 实施前重查，TX1/RX2优先级3 |
| Core/Src/gpio.c::MX_GPIO_Init | PA15下降沿上拉，EXTI15_10优先级4；PB12与PD15初始化高 | 复用READY回调，保持EN，不自动复位 |
| App/app.c::App_Loop | 运行态Datalog_Service后直接return，停车存在顶部及分支内收尾检查 | 两态接通信，保留竞态防护 |
| App/app.c::App_ControlISR | 保护→观察→执行→轮速/运动→CompleteControl→日志快照；风扇当前早于观察 | 调试生效需覆盖本拍计算及风扇，不能仅在末尾覆写 |
| App/electromagnetic_fsm.c::ElectromagneticFSM_Update | 普通分支先算方向，再作十字/预环岛判定；私有history | 必须拆分选择/方向计算，元素切换拍不能多算一次history |
| App/control.c::Tracking_Observe、optical_fsm.c::OpticalFSM_Update | 光电直接读取Param方向bank，在FSM里算方向 | 同步拆分，保留两套独立历史及无调试路径 |
| App/datalog.c/h | 256B头/96B记录、1024槽、87336条，已有尺寸/部分偏移断言 | 补齐全字段断言；完整记录构造后分流，不改Flash指针 |
| App/param.c/h | 参数v3，方向按mode/gear，速度PI/风扇共享，保存前台执行 | 调试RAM隔离，停车合并，保持88B布局及迁移 |
| MDK-GCC/STM32H743VITx.ld | 默认RAM=AXI512KB，ADC独立D2；最小栈0x400 | DMA显式对齐/地址证据，核算嵌套栈 |
| MDK-ARM/SmartCar.uvprojx、startup_stm32h743xx.s | 工程列有DTCM/AXI，栈0x400，未指定scatter文件 | 不假定普通static在AXI；明确DMA段定位并查map |
| Tools/SmartCarLink | 既有电脑实现和固定向量；仓库根64a0cc2已提交 | 仅读参考，不修改，不当作STM32独立验证结果 |

本轮未读硬件设计文件，不推断电气实测事实；不要求修改Hardware/引脚分配。

## 需求—文件—符号—验证映射

新前缀固定为 `BSP_WiFi`、`SCB`、`Telemetry`、`Tuning`；类型分别用bsp_wifi/scb/telemetry/tuning
小写前缀和 `_t`。全部公共/私有名称、注释遵守CS标准，固定HAL/IRQ名保留。

| FR | 目标文件/关键符号 | V |
|---|---|---|
| 01 | app.c既有ISR/保护；bsp.h；app_config.h；main.c USER CODE | 01、02、12、16、18 |
| 02～04 | 新BSP/bsp_wifi.c/h::BSP_WiFi_Init/TryExchange/Service/GetStatus；HAL_SPI_TxRxCpltCallback/ErrorCallback；HAL_GPIO_EXTI_Callback；spi.c/h、dma.c、stm32h7xx_it.c/h、SmartCar.ioc | 02、03、14 |
| 05 | 新App/scb_protocol.c/h::SCB_EncodeFrame/DecodeFrame/ParseCommand/CalculateCrc | 04、05 |
| 06～08 | 新App/scb_link.c/h::SCB_Init/Service/ResetSession/QueueResult/GetRequestResult；scb_request_entry_t | 06、07、08 |
| 09 | scb_link.c命令分发；Tuning_GetParams/GetStatus；Datalog_GetMeta/GetExportStatus | 05、08、09、13 |
| 10～12 | 新App/telemetry.c/h::Telemetry_PushRecord/Service/BeginRun/GetStatus；datalog.c::Datalog_Push；app.c::App_Loop/App_TryStart | 09、10、15 |
| 13～15 | 新App/tuning.c/h::Tuning_Enable/Disable/Submit/GetParams/GetStatus；app_config.h::TUNING_*限制；param.c既有访问口 | 08、11 |
| 16～17 | Tuning_SelectControl/CompleteControl；control.c::Tracking_Observe/Execute；ElectromagneticFSM_Update、OpticalFSM_Update及私有方向函数；App_ControlISR | 11、12、16 |
| 18 | Tuning_FinalizeStop/ServiceSave；App_FinalizeStop；Param_SetDirection/SetFan/TryFlushPending及共享PI合并口 | 11、13 |
| 19～21 | datalog.c/h::Datalog_GetMeta/BeginExport/ReadExport/EndExport/CancelExport/ServiceExport/GetExportStatus；App_TryStart；Datalog_Clear | 13、15、17 |
| 22 | scb_link.c::SCB_QueueEvent；Tuning_CompleteControl；App_TryStart/RequestStop/FinalizeStop | 08、10、11、13 |
| 23 | 各新增静态对象；GCC链接脚本、Keil段定位；IRQ回调 | 02、14、15、16 |
| 24 | 新tests/scb_protocol.py、scb_link.py、spi_wifi_state.py、runtime_tuning.py、spi_log_export.py及其C夹具；既有三个tests；联调文档 | 01～18 |

以上是明确的允许影响面；不是要求改完每个文件。`pid.c/h`、`motor.c/h`只可增加/使用本阶段整组
应用必需接口，保留公式、截断、限幅及历史。新增/改名符号在实现前补入本表并先提交规约增量。
生成区仅用于同步.ioc；第三方HAL不改。GCC自动通配新C文件；Keil须显式登记。

## 接口、状态与并发契约

### BSP接口

- `BSP_WiFi_Init`：前台初始化静态状态、CS高、补获READY；不动EN。
- `BSP_WiFi_TryExchange`：输入完整1024B静态TX和RX目标，均满足DMA地址/对齐；只在空闲且有
  新许可时启动，返回启动/忙/错误；忙不消费许可，进行中缓冲不允许复用。
- `BSP_WiFi_Service`：前台检查完成/错误/10ms超时；完成后RX才可读，超时中止后清旧代际回调。
- `BSP_WiFi_GetStatus`：复制事务状态、许可/错误计数及完成标志，不交出可写内部状态。

状态为IDLE→IN_FLIGHT→COMPLETE/ERROR→IDLE；超时走ABORT→重新同步。IRQ只写BSP标志/
许可，App不直接操纵HAL。许可发布与前台消费使用有界同步，避免读改写竞争。

### 协议、结果与事件

SCB解码输出经校验的头和有界负载；解析错误与业务错误区分，不从非法帧发业务副作用。
解析与编解码只在前台。SCB_Link持有会话、双向seq/ack、单个可靠在途副本、32项请求缓存、
最终结果预留和32条事件队列。Tuning消费完整请求，不持有收包临时指针。
请求身份比较保存原命令负载字节，不只比较CRC。回调与ISR只发布固定大小描述/结果快照，
前台构造JSON。读取当前tick、control_sequence、revision和values时须属于同一快照。

### 上传

Telemetry槽=`96B记录+u32 index/run_id/revision`，256条逻辑容量；用单调head/tail差或等价
全容量SPSC实现，不能声明256条实际只可放255条。TIM15发布前作内存屏障，前台读取后推进tail。
组包批次不跨运行/版本/索引断点。Tuning发布的revision由同级TIM6/TIM15一致读取，不能在
前台把旧记录标成新版本。新会话清旧上传传输缓存，保持车辆有效值。

### 控制拍分解

实施必须建立关闭调试的旧行为对照路径，再修改开启路径：

1. 保留现有tick推进、采集、双保护、DL1B与IMU顺序；保护触发立即停车，不再应用请求。
2. 对同一输入推进两套状态选择与原配置合并，保留所有顺序分支和清零请求。分离方向计算
   及其history写入，使本拍转入/退出特殊元素已知；不能先算方向再二次重算纠正。
3. Tuning按本拍普通条件复核待应用请求，选择基线/最后成功调试快照；整组生效时固定结果。
4. 以该快照作一次方向计算、速度基准/PI/风扇应用，再执行选中模式唯一控制输出。
   特殊拍用原控制策略，调试前PI/风扇；光电的电磁影子状态不成为实际电磁执行口。
5. 保留编码器读取→运动累计→电机输出→CompleteControl→日志快照及完整900μs保护。
   APPLIED的control_sequence须指向真实使用该值的控制拍，避免在CompleteControl前取到上拍编号。

关闭调试路径需保留原普通分支“方向计算在状态判定前”的输出语义；拆分可以保存进入本拍
时的配置用于旧路径，不能用重构便利为由把无调试切换拍改成新的状态配置。开启路径遵守
FR-17切换拍暂停要求。两条路径共用一次状态推进和一次history提交，不回滚/多推history。

调参状态：DISABLED、ARMED（停车预开启）、ACTIVE、SUSPENDED；另有唯一PENDING完整请求
和保存状态。TUNE_DISABLE/停车/会话变化均要终结未应用请求，不遗留跨状态延迟执行。
基线、last_success、effective和待保存目标分开持有；起跑复位不能误删停车预开启状态。

### 导出

Datalog持有缓存头、冻结记录数/总长、transfer_id、next_offset、滚动CRC、最近块副本及最后
新读取时刻；SCB只传请求与结果。BEGIN检查App停止且尾包已刷完；READ整块成功才更新offset/
CRC，重发用缓存；END检查完整交付后解锁。GET_LOG_META不调用Flash读口。
取消、读错、新会话及10秒到期共用收尾入口，所有起跑/清日志入口检查同一导出锁。
控制断开且无内部通知时按另两端任务书允许的10秒超时释放，详见protocol.md。

## 内存与栈交付

以下为容量核算，不是本轮测量：上传槽27648B、TX/RX2048B；请求原负载32×992=31744B，
32份最大结果另31744B；32份最大事件负载上界31744B。再加帧暂存、编解码工作区、LOG块、
结构体元数据、旧日志队列98304B及既有对象，必须在实现map逐项核对，禁止仅凭估计宣布够用。
可采用固定类型快照降低实际事件开销，但不能减少32项/32条契约或丢掉去重原内容。
所有992/1024B及960B导出工作区静态，不放调用栈；JSON解析有明确深度/令牌/字符串上限且
不得拒绝协议要求的合法整组参数。检查TIM15/TIM6与低优先级DMA/前台叠加的最大栈深；
当前1KB声明不能视为充足证据。预算不足时先补规约并确认，不擅扩Flash或改变调度。

## 分组与提交

2026-10-18执行回填：用户正式启动实现；G1自动门禁已通过，接续G2；本节“另行启动”是G0
历史安排。G1双LCD、宿主、CS清单及内存证据见validation，全部硬件门禁仍待验。

| 组 | 范围和交付 | 放行标准 |
|---|---|---|
| G0（本轮） | 需求、协议、计划、验证；roadmap/CHANGELOG和关联规约导航；执行提示词在对话中交付 | 文档检查，独立纯文档提交后停止 |
| G1（另行启动） | SCB编解码/有界JSON、固定向量、SPI2 DMA/BSP/工程 | V-02～07与双LCD构建；硬件项保持待验 |
| G2 | 上传SPSC、App两态服务、run_id/revision/事件接线 | V-08～10，旧三项回归与双LCD构建 |
| G3 | 运行调参、FSM计算拆分、原子快照与停车保存 | V-11～13，旧输出等价及双模式回归、双LCD构建 |
| G4 | 缓存元信息、流式导出、全入口互斥/超时/错误 | V-13、17，所有新增/既有宿主检查、双LCD构建 |
| G5 | 中文联调、实测与最终状态回填 | V-14～18；未上机不勾选阶段 |

每组实现前核对本规约是否覆盖全部实际符号；新设计先规约增量独立提交。每组检查CS-01～15
和明确路径差异，展示清单/摘要/中文提交信息后commit，不push。所有实现组由用户后续启动，
本轮不创建实现线程、不写测试脚本或工程占位文件。

## 风险、停止与回退

- 波形、时钟、DMA地址或资源冲突：停止对应上机步骤，保留证据；不改共享PLL、D-Cache或ADC。
- 无调试等价失败：停止调参集成，定位切换拍/部分覆写/历史推进；不放宽对照断言。
- Flash丢样、超时或额外停车：关闭新增通信路径定位，保留原安全链；不得删保护压过失败。
- 结果超长、队列无界或跨端字段冲突：停止相关编码，先同步契约，不截断执行。
- 回退按本阶段独立实现提交恢复，不覆盖其他目录工作；Flash数据/参数格式始终保持原版本。

## 2026-10-18 实现启动与 G1 内部接口增量

用户已正式启动 G1～G5，上述“仅文档/另行启动”保留为 G0 历史范围。开工提交432c183，
全仓库干净，继续现有分支。复核仅ADC占DMA1 Stream0，Stream1/2和SPI2回调没有其他所有者。

G1增加下列内部符号，不改变公共协议：

- scb_protocol.h/c：scb_frame_t、scb_json_t、scb_token_t、scb_command_t、scb_values_t、
  scb_status_t；SCB_ReadU16/ReadU32/WriteU16/WriteU32、SCB_ParseJson/FindMember、
  SCB_GetNumber/GetString、SCB_StatusName；私有 SCB_ParseValue/ParseString/ParseNumber/
  SkipSpace/ReadHex/AppendUtf8/ValidateUtf8。ParseCommand解析完整op/args并区分语法与业务错误。
  JSON深度8、令牌128、解码字符串池992B；数字最长63B，严格JSON文法，转double后要求有限。
  文档/令牌工作区静态，转义解码后比较同层对象键，含Unicode代理对，拒绝嵌入NUL。
- scb_link.h/c：scb_request_entry_t、scb_link_status_t、scb_command_handler_t、
  scb_session_handler_t；SCB_SetHandlers/ReceiveFrame/PrepareFrame/GetStatus；私有
  SCB_FindRequest/AllocateRequest/AcceptRequest/BuildFrame/HandleAck/StartReliable。
  handler只在前台、完成请求缓存预留后调用，未注册handler返回INVALID_ARG；
  后续组注册真实命令分发。QueueResult仅更新已预留槽，未完成项不淘汰。
  最近32项原文与结果各992B，可靠在途另持完整1024B，精确ACK后释放。
  轮询时间由BSP毫秒读取，可靠超时200ms；序号达到0xFFFFFF00重握手。
- bsp_wifi.h/c：bsp_wifi_status_t；BSP_WiFi_GetRx/GetTx、BSP_WiFi_ReleaseRx，
  BSP_WiFi_SetFastClock（仅停车联调显式调用，默认低速）；私有BSP_WiFi_SetCs。
  TX固定0x2407F000，RX固定0x2407F400，各1024B且32B对齐；GCC将普通AXI区域末端
  保留4KB，栈顶0x2407F000，专段放缓冲；AC5用at绝对定位并以map另行验收。
  AXI剩余2KB保留不用。IRQ累计单调许可、前台持消费位置；HAL_BUSY不消费，
  启动错误消费本次许可；abort期间屏蔽本链低优先IRQ、清DMA/SPI pending后再开放，
  不屏蔽控制IRQ。完成/错误只在IN_FLIGHT时接受，前台收取RX后才允许复用缓冲。
- Core固定名：hdma_spi2_tx/rx、DMA1_Stream1/2_IRQHandler、SPI2_IRQHandler，
  HAL_SPI_TxRxCpltCallback/ErrorCallback、HAL_GPIO_EXTI_Callback；工程显式登记新C。
- tests：共享host_support.py及C夹具，scb_protocol.py/scb_link.py/spi_wifi_state.py；
  固定输入从既有scb_vectors.py逐字复制入SmartCar夹具，以zlib独立比对真实C输出。

G1自动验收覆盖上述边界，双LCD和三个旧回归每组执行。CS记录以实际函数和模块清单为准。

### 空闲失联决定（2026-10-18）

证据：公共重发规则只覆盖可靠帧在途；纯IDLE时ESP重启会丢弃旧会话，原文未定义STM32
重新发HELLO的触发时刻。用户明确选择连续800ms没有当前会话合法帧就重握手。
SCB_Service记录最后合法接收毫秒，差值达到800ms调用ResetSession；坏帧/旧会话不更新。
握手态每200ms重发HELLO；新会话IDLE确认成功启动后才开放COMMAND。
仅清传输缓存、未应用请求及导出，不停车、不回滚last_success/effective。
公共帧与跨端字段不变；未选择“仅可靠重试耗尽触发”，以避免纯空闲无法恢复。

### G1 构建诊断与无堆解析增量

实际GCC链接输出libnosys的_close/_fstat/_getpid/_isatty/_kill/_lseek/_read/_write八条warning，
build.sh只统计CC日志，末尾“0警告”与链接输出不一致。最终ELF仅保留_sbrk，其他八个占位
均被gc-sections移除。G1修改MDK-GCC/build.sh：链接日志独立捕获并纳入警告门禁，不屏蔽warning。
使用同一锁定工具链的arm-none-eabi-ar，从GCC自身解析的同架构libnosys.a提取sbrk.o，
只显式链接此必要对象，移除整包nosys.specs依赖；_sbrk指令/语义不改，不新增运行库代码。
AC5不受影响。SCB数字解析不调用strtod/malloc，采用最多63B尾数及有界十进制指数缩放，
仍拒绝非有限值；此项避免newlib strtod的Bigint隐式堆分配，不整改既有运行库堆策略。

### 栈预算决定（2026-10-18）

GCC -O2 -fstack-usage取证：App_Loop16、SCB_Service40、ReceiveFrame160、ParseCommand72、
ParseJson56、ParseValue104、ReadHex16，共464B。解析使用静态显式容器栈，不随嵌套递归。
按现有优先级4/3/2/1/0叠加的保守硬件上下文上界520B，尚未含IRQ/HAL/控制局部栈，
因此1KB声明不能满足预算保证。用户选择8KB（未选择4KB或保持1KB暂停集成）。
修改MDK-GCC/STM32H743VITx.ld::_Min_Stack_Size和MDK-ARM/startup_stm32h743xx.s::Stack_Size
为0x2000；GCC栈顶0x2407F000，低界0x2407D000，不与DMA区域相交。其他RAM布局不变。
最终G5列前台/ISR各路径预算及map占用，AC5未执行不得以GCC帧大小冒充其结果。

## G2 内部接口增量（先于实现）

- 新App/telemetry.c/h：telemetry_slot_t（96B记录+index/run_id/revision）、telemetry_status_t；
  Telemetry_Init/BeginRun/PushRecord/Service/GetStatus/SetRevision/ResetSession。
  生产者只写head/next_index/生产丢弃，消费者只写tail/消费丢弃；256槽全容量，32位单调索引。
  Init仅上电，BeginRun仅定时器未启动时分配run_id并重置采样索引；清旧队列只推进tail并记丢弃。
  会话重建不改已运行run_id或revision，只清旧上传；run_id将回绕前重建链路再从1分配。
  Service前台10ms封包，最多10条，同run/revision/index连续；只跳过旧run槽，不引入新的
  可调新鲜度阈值。无READY不消费，可靠结果/事件优先；包内ESP丢弃计数初始0。
- scb_link.h/c：scb_event_kind_t/scb_event_t固定描述、SCB_QueueEvent；私有SCB_TakeEvent/
  FormatEvent。32条队列支持ISR/前台短临界区发布，不在IRQ格式化JSON。满后保留首个gap
  描述并累计丢弃，先排空旧事件再发送EVENT_GAP，gap未发出期间新事件继续计入遗漏。
  事件与请求最终结果存储互相独立。ResetSession清传输事件/上传缓存，保留有效控制值。
- scb_protocol.h/c：scb_writer_t；SCB_JsonBegin/Append/U32/I32/Float/Values。
  writer有固定容量及failed标志，溢出整条拒绝不截断；Float用有界9位有效十进制数字输出
  binary32，可往返核验，不调用浮点printf/strtod或堆。私有SCB_FormatFloat辅助可合入Float。
- app.c：Init在既有总装后接SCB_Init；两态Loop均服务SCB_Service，运行态严格在
  Datalog_Service之后return之前。App_TryStart在全部门禁通过且复位后BeginRun；
  App_RequestStop仅首次停车时发布RUN_STOPPED，tick和最近完整control_sequence在发生处
  固定复制。事件状态枚举OK，停车原因额外字段stop_reason，不改变首因/累计保护。
- datalog.c：96B记录完整构造后Telemetry_PushRecord，不改Flash head/tail/安全路径；
  补全datalog_record_t每字段offset断言。Keil显式登记telemetry.c。
- tests/scb_link.py扩展真实C上传/事件验收；旧dual_mode_tracking.py/datalog_v2.py只增加
  新通信依赖替身和调用观察，不删改既有断言。前者检查两态Loop服务及实际起跑/停车发布；
  新测试独立编译真实telemetry.c，覆盖256槽/满丢/回绕/断点/版本/运行/10ms与事件gap。

FR-10～12/22的字节、状态、容量和时序不变；其余业务命令由G3/G4注册处理器完成。

### G2 前台5ms时序确认（2026-10-18）

证据：bsp_w25q64.c页等待上界10ms、扇区500ms，Flash_LogWrite在前台跨扇区时阻塞擦除；
既有LCD/串口也有阻塞调用。仅前台启动SCB与“所有工况5ms硬上限”不能同时保证。
用户选择保留阻塞路径，5ms是前台可运行期间的到期尝试周期；阻塞结束后立即服务，
保持Flash优先、IRQ只发布状态。未选择扩展异步Flash/LCD重构，不新增停车条件。
SCB_Service/PrepareFrame用无符号差检查5ms；V-10分别验正常前台周期与模拟阻塞后立即服务。

G2回填：独立队列、run_id/revision、32事件/gap、无堆有界JSON输出和两态接线已完成，
双LCD GCC及六个宿主入口通过。G3方向恢复已获用户决定，见下列增量。

## G3 控制实现增量（先于代码）

2026-10-18用户明确选择：方向/基础速度恢复原FSM策略，PI/风扇恢复开启前基线。
电磁普通2/.008/15/245与停车bank可能不同；禁用后不保留方向覆盖，Phase10继续暂缓。
本增量落实FR-09、13～18、22；公共协议、参数88B v3和日志96B v2不变。

- electromagnetic_fsm.c/h新增ElectromagneticFSM_Select/Calculate：Select测量、合并原策略、
  推进一次状态及原位置的里程/角度复位；静态本拍标志记方向计算、十字限幅、预环输出清零。
  Calculate使用传入增益或原配置，只推进一次历史。Update保留原策略包装。原阈值、分支
  顺序、清零后重读距离、steering/output差异、TURN_LEFT不推进方向历史均保留。
  configuration始终只保存原FSM策略，调试值不进入部分覆写的历史配置。
- optical_fsm.c/h新增OpticalFSM_Select/Calculate；前者原WAIT/LOST/STOPPED判断及计算许可，
  后者一次方向历史；Update保留包装。1000/1001拍边界及保护不变。
- control.c::Tracking_Observe先Select两套FSM，再按新电磁状态决定是否普通赛道，调用
  Tuning_SelectControl，再Calculate。只有选中模式收到调试方向输入；Execute用独立执行
  剖面覆盖基础速度/方向，不污染电磁configuration。无调试全部逐拍输出必须等价。
- 新tuning.c/h：Tuning_Init/HandleCommand/ResetSession/SelectControl/GetControl/
  CompleteControl/Stop/FinalizeStop/Service/GetStatus。静态分开baseline、pending、
  last_success、effective、revision；DISABLED/ARMED/ACTIVE/SUSPENDED。开启幂等，捕获当前
  目标及共享PI/风扇；last_success匹配目标才可恢复。运行禁用在下个完整拍恢复共享值，
  停车立即恢复；每次停车关闭。SET等待/收尾/已有pending或最终邮箱未排空返回BUSY；
  目标不匹配/fan_idle变化INVALID_ARG，特殊赛道SPECIAL_ELEMENT。不推迟到离开元素再执行。
  合法运行SET短临界区发布完整pending并RECEIVED；Select复核，Complete执行后固定
  APPLIED和事件，使用完成sequence及实际tick。保护先停车取消未应用项并BUSY。
  切换拍暂停/恢复在实际有效快照改变时增加revision并固定事件；成功SET即使数值相同仍
  分配提交revision。会话变化取消pending/结果邮箱，保留生效值和last_success。
  Complete不JSON/Flash；Service前台输出单槽最终结果。大JSON缓冲静态，无ISR等待。
- app.c在SCB初始化后接Tuning_Init；两态Loop服务邮箱；风扇PWM移到Observe之后、轮速PI
  之前，以使用本拍参数。Complete接在Tracking_CompleteControl后、Datalog发布前。
  Stop恢复共享基线并取消pending；FinalizeStop在Datalog_Stop后合并last_success到目标
  bank/共享PI/风扇；Service只停车保存一次并发PARAM_SAVED/FAILED事件。失败沿用Flash
  不可用起跑门禁；无成功SET不标脏/不额外写，不从元素effective反推保存值。
- param.c/h新增Param_GetSpeedGains/SetSpeedGains读取镜像共享PI、设置镜像及活动PI并标脏。
  原本地保存入口/布局不变。GET_PARAMS使用显式bank/镜像PI/档位风扇，另附effective；
  baseline捕获活动PI以包含既有本地改动，不能把运行临时值作为存储值。
- app_config.h集中TUNING_GAIN_MIN/MAX、SPEED_MIN/MAX、FAN_MIN/MAX，scb_protocol.c新增
  SCB_ValidateValues供解析与调参二次校验；GET_PARAMS逐项limits同源。
  GET_STATUS复制一致快照后编码，save_state为idle/pending/saved/failed；日志缓存有效位由
  G4元信息接口补齐，在此之前不声称对应FR-19已通过，不读Flash。
- Keil登记tuning.c。新增tests/runtime_tuning.py编译真实控制/FSM/PID/调参/参数模块及模拟
  BSP，验方向当拍、历史一次、切换拍、禁用、保护/会话、停车保存/失败；旧回归只补新
  依赖替身，既有断言保持。全部既有测试与双LCD零警告门禁每组重跑。

### 缓存结果有界适配决定（2026-10-18）

用户选择有界查询适配：普通JSON缓存放result对象；查询GET_REQUEST_RESULT的结果时取
直接业务结果，避免递归包装；LOG_READ缓存查询返回OK及offset/length，原始块用同编号
同内容LOG_READ重取。GET_LOG_META完整结果约600B仍可包装，GET_PARAMS含八项limits及
两份values须核验992B；所有结果超过界限须停止分支报错，不能截断。此为STM32适配，
公共协议原文不改。tuning.c静态查询缓冲及SCB_GetRequestResult复制后有界解析实现。

G3回填：e380a54后实现及自动门禁完成，继续G4；详见validation实际记录。

## G4 缓存元信息与导出内部增量（先于代码）

实现FR-09、19～21，接续G3。用户确认：日志写失败原收尾停止重试且RAM仍可能pending，
停车收尾返回后允许导出written_records确认的前缀，失败批次不算入总长；原时间轴逐字节
保留，未写记录不补造。原fault/accepted/written/rejected/pending统计不被导出改写。

- datalog.h/c新增datalog_meta_t（valid、written_records、256B头副本），
  Datalog_GetMeta；仅从s_header/s_header_valid/s_status.written短临界区固定复制；无效头
  输出清零，查询绝不访问Flash。GET_LOG_META有效时输出512位小写header_hex，无效时空串。
- 新datalog_export_status_t及Datalog_BeginExport/ReadExport/EndExport/CancelExport/
  ServiceExport/IsExporting/GetExportStatus。全部前台独占；单会话active、递增非零ID、
  frozen record_count/total_bytes、next_offset、流式CRC及last_progress_ms。固定968B块
  缓冲含8B LOG_DATA前缀，不持整份日志。Begin检查App_IsStopped且日志非active，优先
  BUSY；无可确认会话NOT_FOUND；已有导出BUSY；冻结256+96*written并从offset0开始。
- Read验证transfer_id（失配/失效NOT_FOUND）、length1～960和减法防溢出总长边界
  （OUT_OF_RANGE）、严格next_offset（乱序INVALID_ARG）。仅最近一块允许新request_id
  同offset/length重取缓存；原request_id重复由SCB32项缓存处理。重复不读Flash、不推进
  CRC、不延长租期。新块先喂狗、读原Flash字节、喂狗；成功后更新CRC/next_offset及租期。
  错误取消导出、保留原首fault并锁存READ（仅无旧fault时），返回IO_ERROR、不停车。
- End失效NOT_FOUND，未交付全长BUSY（锁保留），全部交付返回total_bytes/crc32后释放。
  ServiceExport无新成功读取满10000ms释放，采用无符号差覆盖tick回绕；普通通信、META、
  查询和重复块不续租。Cancel仅释放，不清日志或调试状态。ID回绕跳0，缓存会话变化清除。
- App_TryStart在原停车检查后、任何起跑副作用前检查Datalog_IsExporting；Datalog_Start
  在Init前、Datalog_Clear在改变active/valid前同样检查；拒绝返回原失败1，不增加fault。
  已核对App唯一Flash_LogErase调用均经这两个入口；串口清日志原调用自动受锁保护。
  原CSV Dump仍只读，并行前台串行执行，不改变冻结前缀。
- tuning.c新增Tuning_LogCommand/QueryLogMeta，复用静态编码缓冲。LOG_BEGIN/END返回
  公共字段，READ成功直接SCB_QueueResult(LOG_DATA,final=1)，错误为RESULT，所有请求在
  SCB已有预留槽中完成；GetStatus补log_valid/export_active。Tuning_Service先处理租期；
  ResetSession清未应用请求同时CancelExport，不改有效调参及成功值。无内部TCP断开通知
  时仍按既定10秒释放，不自行新增操作或重置SPI。
- tests/spi_log_export.py用真实App/Datalog/Tuning/协议C配模拟Flash，独立Python zlib比对
  原字节CRC和总长；0/3/最大87336条、1/960边界、乱序/溢出/重复、提前END、读错、
  9999/10000ms与回绕、会话取消、真实起跑/清日志全入口锁和故障前缀时间轴逐项检查。
  旧datalog_v2补App_IsStopped替身；dual_mode_tracking补真实SCB_CalculateCrc依赖，原断言
  保持。全部八个宿主入口、双LCD零警告与CS-01～15本组重跑，硬件门禁仍待验。

G4回填：缓存头、流式CRC、成功前缀与全入口锁通过宿主和双LCD门禁，接续G5。

## G5 自动覆盖补强及证据交付增量（先于代码）

不执行不可用硬件步骤，不重新创建交接文档；中文联调和最终门禁表直接回填validation。
- tests/runtime_tuning.py扩展真实C两模式×两档，并覆盖预环/右环各活动分支及退出普通拍，
  检查切换拍原增益、成功值恢复、原方向历史/部分配置未受污染。补独立有限性/八项边界和
  最大合法GET_PARAMS/GET_REQUEST_RESULT长度检查，不删除或放宽原断言。
- tests/phase16_check.py为可重复入口：顺序执行8个真实C宿主入口、可选双LCD构建及
  -fstack-usage预算取证；结束始终恢复原app_config。核验实际编译/链接诊断、DMA符号
  地址/尺寸、ARM大小、.ioc/IRQ优先级/时钟、Keil源清单及8KB栈，不宣称AC5可由GCC替代。
  原始输出留MDK-GCC/build，结构化实际结果写tests/results/phase16.json，可跟踪测试和
  内存事实；此为机器验收证据，不是独立交接文件。
- 同一锁定GCC按实际-O2/硬浮点编译自维护App/BSP/Core及相关HAL的栈报告，静态列前台和
  各IRQ路径，叠加现有优先级硬件上下文；库函数/间接回调的推导限制明确记录。实际上机
  高水位、700/900微秒、逻辑分析仪及60秒吞吐仍待验，不能以静态预算填成实测。
- 只在已修改模块核查C完整/H摘要、中文阶段及类型字段；未改变行为的格式修正也在此
  明确覆盖。不扩展到旧休眠代码、保护、真实标定或其他端。

G5停车预开启后切换目标的再次ENABLE行为已单独询问；该分支未确认前不改实现。

G5结果错误路径修正：当前Tuning_Send在writer失败时把op固定为GET_REQUEST_RESULT，
与RESULT须沿用原操作名不一致。改为显式接收原op并在IO_ERROR中保持，所有调用同步；
最大合法结果不能触发失败。测试可在宿主旧bank注入非有限旧值触发编码失败，但固件原
镜像加载/本地调参行为不扩改，不用错误对象伪装成功或截断JSON。

2026-10-18 G5目标确认：Tuning_Switch仅在停车目标改变时重新捕获owner/baseline并ARMED；同目标重复ENABLE不捕获调试值。目标相同且处于待禁用恢复的控制拍前重新开启，也沿用原基线；不增加运行切换入口。tests/runtime_tuning.py补停车预开启→切换模式/档位→再次开启→本目标SET成功及同目标基线不漂移。

2026-10-19 G5帧身份增量：scb_protocol.c::SCB_BuildFrame在原类型/标志/长度检查后验证HELLO的session/seq/request均0；HELLO_ACK的非零session、seq/request为0；正常类型session/seq非零，COMMAND/RESULT/LOG_DATA的request非零，IDLE/EVENT/TELEMETRY的request为0。错误编码失败、解码丢弃，不产生ACK或业务副作用。tests/scb_protocol.py用Python独立改头和重算CRC验证身份错，固定13帧保持；scb_link.py注入CRC正确的坏IDLE身份，确认不释放可靠在途、不续租，不修改公共原文。
