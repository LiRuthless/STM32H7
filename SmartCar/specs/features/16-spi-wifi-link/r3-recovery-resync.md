# R3：错误恢复双通道修补与许可积压结构性重同步

2026-10-22规约增量，关联FR-03/04/23及R2。先独立提交本文件及三件套入口，再修改实现。
证据：Reports/link-validation/20261022-remediation/baseline（失败基线全过程）、
d1-localization（测试固件定位，含负结果）及freeze-state.log。
本批不改变公共协议、引脚、Mode0、分频、1024B事务、参数/日志布局、车辆起停或EN。

## 已确认失败机制及范围

通道A（ERROR态DMA句柄）：HAL_DMA_Abort对非BUSY句柄只写ErrorCode=NO_XFER(0x80)
并返回错误，不改变State（stm32h7xx_hal_dma.c:797-805）。BSP_WiFi_AbortDma仅中止
BUSY句柄，ERROR/TIMEOUT/ABORT态句柄被跳过且永远回不到READY，R2-02判据永不满足。
实机证据：baseline/stuck-state.log与d1-localization/freeze-state.log中hdmarx
ErrorCode=0x80、Stream EN=0、state=3永久冻结。

通道B（粘性ABORT位）：HAL_SPI_Abort见到ErrorCode中残留的HAL_SPI_ERROR_ABORT位即
返回HAL_ERROR，且只有成功路径才清ErrorCode（stm32h7xx_hal_spi.c:2661-2670），
一次失败后每次重试都失败。实机证据：freeze-state.log中hspi2.ErrorCode=0x44
（OVR|ABORT），前台tick仍前进，恢复永远失败。

D1（许可积压无重同步）：permits/consumed为自由运行计数，全生命周期无重同步。
ESP32侧SPI从机队列深度为1，任意时刻最多一笔已装载事务，合法积压（permits−
consumed）不超过1；实测基线中积压12～16恒定存在且使STM32不等READY自由发，
ESP32装载窗口内被截短。泄漏种子依赖完整App的IRQ/时序环境：最小测试固件在更强
负载下hal_busy计数恒0、积压不增长（负结果），不能在无证据下定点修改。

用户决定（2026-10-22）：D2定向修补两条通道，不做外设重初始化兜底；D1采用结构性
兜底重同步，不等待泄漏种子定位。

## 可判定需求

- **R3-01 通道B**：Service中HAL_SPI_Abort返回非HAL_OK时，先将hspi2.ErrorCode
  写回HAL_SPI_ERROR_NONE，再有界重试一次；每次Service调用最多两次Abort尝试。
  重试仍失败保持state=3，下一前台周期按同规则再试；不在单次调用内无界循环。
  错误计数仍在ErrorCallback累计，清ErrorCode不影响errors统计语义。
- **R3-02 通道A**：BSP_WiFi_AbortDma对非BUSY且非READY句柄（ERROR/TIMEOUT/ABORT）：
  不调用HAL_DMA_Abort（避免NO_XFER污染）；若Stream CR.EN=1先软件清EN并有界等待
  （不超过2ms）其归零，超时返回0；随后将句柄ErrorCode写回HAL_DMA_ERROR_NONE、
  State写回HAL_DMA_STATE_READY。BUSY路径保持R2的HAL_DMA_Abort；READY路径不动。
  回收成功判据不变：最终READY且EN=0。流错误标志由下一次HAL_DMA_Start_IT起始
  清标志序列处理，本函数不直接写IFCR。
- **R3-03 许可重同步**：新增私有BSP_WiFi_ResyncPermits，复用G5-R1启动去重模式：
  短临界区内先清READY pending、采样READY电平、按电平设积压（低=1：一笔已装载
  等待；高=0），电平低时二次清pending合并窗口边沿；consumed向permits对齐
  （consumed=permits−积压）。正常路径两计数只增不减；采样窗口内刚装载而尚未计数
  的边沿允许consumed回退1，使积压如实反映当前装载状态。调用点两处：(a)Service恢复成功
  发布state=0前；(b)Service常规路径发现state==0且积压>1时。IRQ沿计数路径不变，
  不在IRQ中clamp或丢弃边沿，FR-04在途快速沿保留规则不变。依据不变量：ESP32
  队列深度1，合法积压≤1，>1必为幻影；电平采样给出真实装载状态。
- **R3-04 不变量**：不新增公开API、状态字段或动态分配；不改bsp_wifi_status_t
  布局、正常完成路径、HAL_BUSY保留许可路径、初始化READY去重。全部等待有界，
  仅前台执行，不屏蔽TIM15/6/7，不新增ISR等待或通信停车，不复位两块板。

## 影响文件、符号与顺序

1. `BSP/bsp_wifi.c`：扩展私有BSP_WiFi_AbortDma覆盖非BUSY非READY句柄；Service在
   HAL_SPI_Abort失败后清ErrorCode有界重试一次；新增私有BSP_WiFi_ResyncPermits并
   在上述两个安全点调用。C函数按CS-11～13更新完整中文契约；不动其他模块。
2. `tests/spi_wifi_state.py`：夹具HAL模型补SPI ErrorCode粘性行为（含ABORT位直接
   失败、成功路径清零）与DMA句柄ErrorCode字段；R2既有"ERROR态句柄保持state=3"
   断言按R3-02反转为恢复成功，旧行为由新增负对照覆盖；新增重同步断言：构造
   积压>1且电平高/低、恢复成功点、窗口注入边沿合并。保留其余全部既有断言。
3. 规约先提交，随后实现/验证提交，最后回填validation、roadmap和CHANGELOG。
   共享检出及ESP32Bridge在途改动保持；仅暂存本批路径，提交在当前检出
   feature/02-workspace-flash（R2分支已并入），不push。

## 验证与唯一通过标准

| 编号 | 需求/方法 | 通过标准 | 初始状态 |
|---|---|---|---|
| R3-V01 | 02；注入TX/RX ERROR态句柄（EN=0与EN=1两种） | 恢复成功：句柄READY、EN=0、state=0，新READY沿可完成下一笔1024B | 待执行 |
| R3-V02 | 01；模拟粘性ABORT位使首次Abort失败 | Service清ErrorCode重试成功恢复，单次调用Abort≤2次；持续失败保持state=3且每轮有界 | 待执行 |
| R3-V03 | 03；构造积压5且电平高/低、恢复成功点重同步、窗口边沿注入 | 电平高积压归0、低归1；窗口边沿不双计不丢失；IRQ计数路径行为不变 | 待执行 |
| R3-V04 | 01～03；负对照：隔离撤回通道A修补、通道B重试、重同步 | 旧路径分别在对应断言精确失败；当前生产代码全部通过 | 待执行 |
| R3-V05 | 回归；既有宿主断言（启动去重、R2回收、迟到回调、超时） | 全部保留通过 | 待执行 |
| R3-V06 | 04/CS-01～15；宿主夹具、Keil AC5实际构建 | 记录真实编译器与结果、ABI/布局不变、无新增警告；GCC路径缺失如实记录，Clang/LLD证据标注为替代宿主 | 待执行 |
| R3-V07 | 01～03；烧录后实机：查询负载与持续运行 | backlog≤1恒定；发生错误后state回0不永久冻结；端到端查询恢复；记录旧/新HEX哈希 | 待烧录 |

板上更新前展示旧/新HEX哈希、构建/宿主结果和更新原因，按统一flash.ps1执行；
本次烧录已获用户授权。没有烧录与实机记录不关闭R3-V07。
回退仅限本批BSP/夹具/规约增量，不清其他任务文件；不通过提高SCK或自动复位掩盖异常。

## 勘误（2026-10-22，烧录后实机复测）

证据：Reports/link-validation/20261022-remediation/validation/backlog-after-fix.log
（3000点/60秒）与同窗esp32-passive-serial.txt。旧版R3固件（HEX 707e753d…）下
errors=0、timeouts=0、ESP32侧crc=0，但permits−consumed以恒定偏移3～4起步，
靠READY沿突发棘轮至20，四次state==0采样瞬间（backlog=4/3/4/9）均未被修剪。

机制：SCB_Service每趟顺序为BSP_WiFi_Service→ReleaseRx→TryExchange；只要幻影
积压≥1，同一趟内TryExchange立即把state从0拉到1，下一趟入口永不以state==0
进入Service，R3-03调用点(b)在持续流式工况下不可达——采样到的state==0瞬间全是
ReleaseRx与TryExchange之间的趟中快照。调用点(a)（恢复成功后）不受影响。

修补：R3-03调用点(b)的触发由"state==0且积压>1"扩展为"(state==0或state==2)
且积压>1"。state==2时DMA已完成、前台独占状态、缓冲静态，与state==0同为无在途
事务安全点；每趟入口通常以state==2到达，重同步在每次完成事务后生效，幻影积压
在一笔事务周期内被修剪，合法积压（ESP32队列深度1）不受影响。G5-R1采样去重顺序、
consumed回退1勘误及其余不变量不变。宿主夹具补"state==2入口幻影积压被修剪"断言，
并以隔离撤回本扩展作负对照；R3-V03/V04覆盖本修补，R3-V07按同标准重新烧录验证。
