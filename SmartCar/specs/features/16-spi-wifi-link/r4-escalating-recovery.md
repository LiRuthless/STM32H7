# R4：错误恢复升级为有界外设重初始化兜底

2026-10-23规约增量，关联FR-03/04/23及R2/R3。先独立提交本文件及三件套入口，再修改实现。
本批不改变公共协议、引脚、Mode0、分频、1024B事务、参数/日志布局、车辆起停或EN。

## 证据与既有决策的证伪

证据根：Reports/link-validation/20261022-remediation/validation/r4-probe/。

2026-10-23实机确认链路处于持久OVR错误死循环：errors约5笔/秒持续增长，completed
永久冻结，SPI2 SR=0x18007、TXNDTR冻结0x3d7、RXNDTR=0、LISR=FEIF2、hspi2.ErrorCode
在0↔0x44间循环；R2/R3定向修补机械上正常工作（state 3→0循环、ErrorCode被清、
句柄回READY），但链路永不治愈，复位任一板无效（observe.log）。DMAMUX请求路由已核对
无误：DMA_REQUEST_SPI2_RX=39与实机读到的0x27一致，排除RX被TX请求驱动。

决定性实验（spi2rst-experiment.log、cfg2-fix.log）：halt后脉冲RCC APB1LRSTR
SPI2RST（bit14）并按固件配置恢复SPI2寄存器，resume后死循环立即停止——completed以约
80笔/秒恢复增长、errors冻结，持续约35～40秒完全健康；随后以完全相同的寄存器签名
自发复发。结论：(a)病灶在SPI2外设/请求逻辑状态，不在HAL句柄，定向修补对此无效；
(b)外设级复位可治愈；(c)存在持续性诱发源，恢复必须可重复执行。

用户决定（2026-10-23，弹窗确认）：推翻2026-10-22"不做外设重初始化兜底"的决定，
采用"定向修补+连续失败升级外设重初始化"；新增外设重初始化次数调试计数器
（静态变量，不改s_status遥测结构与协议）。诱发源根因调查不属本批范围，本批只
保证每次复发都能自动治愈。

## 可判定需求

- **R4-01 连续失败计数**：新增私有`static uint32_t s_consecutive_errors`，统计自上次
  成功完成以来连续失败的事务代数。递增点三处，与失败计数同点位：TryExchange启动
  失败（errors++处）、HAL_SPI_ErrorCallback（errors++处）、Service超时恢复
  （timeouts++处）。唯一清零点：HAL_SPI_TxRxCpltCallback成功路径（completed++处）。
  不在恢复成功（state→0）时清零——定向修补的机械成功不代表链路治愈。
- **R4-02 升级重初始化**：Service恢复路径中，HAL_SPI_Abort与双向DMA回收完成、CS已
  拉高之后，发布state=0之前：若s_consecutive_errors≥3（私有宏
  BSP_WIFI_REINIT_THRESHOLD），执行外设重初始化——`__HAL_RCC_SPI2_FORCE_RESET()`后
  立即`__HAL_RCC_SPI2_RELEASE_RESET()`脉冲复位SPI2，再`HAL_SPI_Init(&hspi2)`
  （重跑MspInit：重写相同内核时钟选择、相同GPIO AF配置、重新HAL_DMA_Init并重新
  链接hdmatx/hdmarx、重新使能SPI2 IRQ；hspi2.Init未变，含当前分频档）。随后
  s_consecutive_errors=0、++s_reinit_count，继续原共同收尾（清pending、重同步、
  发布state=0）。HAL_SPI_Init返回非HAL_OK时保持state=3不发布空闲、
  s_consecutive_errors保持≥阈值，下一前台周期按同规则重试。不调用
  HAL_SPI_MspDeInit/HAL_SPI_DeInit，避免GPIO翻转毛刺、SPI2时钟关闭与SPI2 IRQ被关。
  执行时机保证DMA流已停（AbortDma判据READY且EN=0）、SPI已中止、三IRQ已屏蔽、CS
  已拉高；RCC脉冲只复位SPI2寄存器域，不改RCC时钟配置、DMAMUX、DMA与其他外设。
- **R4-03 调试计数器**：新增`static volatile uint32_t s_reinit_count`，每次外设重
  初始化递增一次。仅供调试器经map/符号读取；不进s_status，不改bsp_wifi_status_t
  布局、不改协议与遥测，无公开访问器。
- **R4-04 复发语义**：重初始化后若诱发源再现，s_consecutive_errors重新累计，到
  阈值再次重初始化；每次重初始化间隔至少三笔失败事务。固件不尝试定位或消除诱发源，
  不限制重初始化总次数（每次执行本身有界，间隔由失败速率自然节流）。
- **R4-05 不变量**：不新增公开API、状态字段或动态分配；不改公共H、协议、引脚、
  时钟树、分频档语义、车辆起停、EN。全部路径无无界等待，仅前台执行，不屏蔽
  TIM15/6/7，不新增ISR等待，不复位任何一块板。正常无错误路径行为逐字节不变
  （计数器只在错误/成功回调点位读写，成功路径增加一次写零）。

## 影响文件、符号与顺序

1. `BSP/bsp_wifi.c`：新增私有宏BSP_WIFI_REINIT_THRESHOLD、静态对象
   s_consecutive_errors与s_reinit_count；TryExchange/ErrorCallback/Service超时三处
   递增，TxRxCpltCallback清零；Service恢复发布点前按R4-02升级。模块头说明及
   受影响函数的C完整契约按CS-11～13更新；不动其他模块。
2. `tests/spi_wifi_state.py`：夹具补RCC复位建模（复位清除注入的OVR外设锁存）与
   HAL_SPI_Init调用计数；新增断言：连续3次失败第3次恢复时恰好1次重初始化且
   consecutive清零、成功后consecutive归零需重新累计、重初始化失败保持state=3且
   下轮重试、负对照（隔离撤掉升级路径时OVR锁存场景永不恢复、对应断言失败）。
   保留R2/R3及更早全部既有断言。
3. 规约先提交，随后实现/验证提交，最后回填validation、roadmap和CHANGELOG。
   仅暂存本批路径，提交在当前检出feature/02-workspace-flash，不push；
   ESP32Bridge及仓库其他在途改动不纳入。

## 验证与唯一通过标准

| 编号 | 需求/方法 | 通过标准 | 初始状态 |
|---|---|---|---|
| R4-V01 | 01/02；宿主注入连续外设OVR锁存故障 | 前2次恢复仅定向修补，第3次恰好1次RCC脉冲+HAL_SPI_Init，s_reinit_count=1，state回0且下一笔真实完成 | 待执行 |
| R4-V02 | 01；成功完成清零语义 | 成功后consecutive=0；再需3次连续失败才第2次升级；运行期无错误时两计数不变 | 待执行 |
| R4-V03 | 02；重初始化失败路径 | HAL_SPI_Init失败保持state=3、不发布空闲、下轮有界重试；IRQ最终恢复 | 待执行 |
| R4-V04 | 01～05；负对照 | 隔离撤掉升级后OVR锁存场景errors持续增长且不恢复；当前生产代码全部断言通过 | 待执行 |
| R4-V05 | 回归；既有宿主断言（启动去重、R2/R3、超时、迟到回调） | 全部保留通过 | 待执行 |
| R4-V06 | 05/CS-01～15；宿主夹具、Keil AC5实际构建 | 记录真实编译器与结果、ABI/布局不变、无新增警告；Clang/LLD证据标注为替代宿主 | 待执行 |
| R4-V07 | 01～04；烧录后实机：死循环工况 | errors停止增长、completed恢复增长、s_reinit_count≥1且可经调试器读取；10分钟连续运行无人工复位、backlog≤1；记录旧/新HEX哈希 | 待烧录 |

板上更新前展示旧/新HEX哈希、构建/宿主结果和更新原因，按统一flash.ps1执行；
本次烧录已获用户授权。没有烧录与实机记录不关闭R4-V07。
回退仅限本批BSP/夹具/规约增量，不清其他任务文件；不通过提高SCK或自动复位掩盖异常。
