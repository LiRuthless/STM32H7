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
