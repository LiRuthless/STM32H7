# Phase 16 — 验证与执行记录

> 当前状态（2026-10-19）：G1～G5可执行实现和自动验证已完成，真实执行证据见各组及G5记录。
> D项及“仅文档”段落保留G0历史；V-14～16、AC5及三端硬件联调仍待验，Phase未勾选。

## 本轮文档检查

| ID | 检查 | 实际结果 |
|---|---|---|
| D-01 | 全仓库状态、当前分支、基线核对 | 开始干净；feature/16-spi-wifi-link；84adaf0 |
| D-02 | 任务书公共契约在三份文件及protocol.md一致 | 通过；统一换行后公共节逐字一致，SHA256见下方 |
| D-03 | 全部FR在计划/验证双向映射；本地链接有效 | 通过；FR-01～24、V-01～18覆盖；新目录16个本地链接有效 |
| D-04 | 现状符号/引脚/DMA/记录布局/时钟/栈只读核对 | 已核对；证据见plan.md，不代表硬件通过 |
| D-05 | 固定向量以独立方法核对CRC、头长和负载长度 | 通过；10组13帧，独立逐位CRC与zlib交叉核对；坏帧按预期失败 |
| D-06 | 仅Markdown、无实现/工程/测试代码；差异空白检查 | 通过；明确路径暂存后复核，见提交记录；git diff --check无错误 |

## 后续验收

2026-10-18实现启动：V-03补验HAL_BUSY保留许可、abort清pending及RX取走前不复用；V-05补验
深度8/令牌128/字符串池992B边界、转义等价重复键、代理对及数字63B界限；V-02核对TX/RX
0x2407F000/0x2407F400、普通RAM/栈不相交。新增host_support.py和C夹具仅作为真实实现测试。

测试入口以真实C实现配宿主替身为主体，禁止只用Python重写相同算法证明自己。
新增入口均已实现；G0当时未创建测试，历史记录不代替下方真实执行结果。
所有异常注入检查执行次数、结果、有效参数和Flash副作用，不只检查返回字符串。

| V | 对应FR | 方法与刺激 | 唯一通过标准 | 状态 |
|---|---|---|---|---|
| V-01 | 01、24 | 每实现组串行构建LCD_TARGET_EXTERNAL=0/1；结束恢复原值；检查新对象/Keil登记/布局 | 两配置均0警告0错误；新对象链接；无工具链升级；AC5另记实际结果 | GCC两配置通过；AC5待验 |
| V-02 | 01～04、23 | 比对.ioc/生成文件/IRQ/链接map；核对DMA、NVIC、EN及SPI45补丁 | TX1/RX2优先级3，READY4，ADC0优先级2及控制0/0/1不变；两缓冲32B对齐且在AXI，无DTCM | GCC/源码核对通过；AC5/波形待验 |
| V-03 | 04 | spi_wifi_state真实BSP替身：上电低、持续低、在途新下降沿、快速高低、HAL忙/错、迟到完成、9/10/11ms超时 | 每新许可只发一笔；不复用旧许可；只允许单在途；完成才释放CS，10ms走前台中止且后续可握手；ISR无等待 | 宿主自动通过 |
| V-04 | 05、24 | scb_protocol：protocol.md全部固定向量；独立逐位CRC与字段期望交叉核对；1/10条、LOG_DATA与边界0/992/993 | 合法字节与公共向量一致，头32B，SPI1024B零填充；无结构体强转；所有记录偏移断言成立 | 宿主自动通过 |
| V-05 | 05、09、13 | 坏magic/version/flags/type/CRC/长度/count/UTF-8、重复键、NaN/Inf、深嵌套/截断/超长、半包命令 | 全部拒绝且不执行部分请求；非法SPI帧不ACK；合法最大消息不被解析上限误拒绝 | 宿主自动通过 |
| V-06 | 06、07 | scb_link可注入时钟：重复HELLO、重启、旧会话、seq回绕前、ack错误/精确、200ms×三重发、业务等待时IDLE确认 | 重发保持原seq/内容，最多三次，失败重握手；高seq非可靠帧不屏蔽可靠重发；旧会话无副作用 | 宿主自动通过 |
| V-07 | 07、08 | 丢ACK/结果、相同request原文重发、相同编号异内容、新TCP保持同SPI会话、换SPI会话 | 接受一次执行一次；精确ACK释放；缓存32项；查询可得最终结果，缺失NOT_FOUND；换会话取消未应用请求、不回滚有效值 | 宿主自动通过 |
| V-08 | 08、09、22 | 填满结果/事件/请求缓存，接收SET_PARAMS后暂不消费，事件持续产生 | 无最终结果槽则BUSY且不应用；已接管请求最终结果不被挤掉；EVENT_GAP可观察；状态快照完整 | 宿主自动通过 |
| V-09 | 09～12 | telemetry真实C：256次生产、满后新采样、前台跳旧、head/tail回绕、元数据切换、CRC延迟计算 | 256条容量可用；满丢本次上传且计数；Flash指针/记录不变；10ms/最多10条封包、不跨缺口/版本/运行 | 宿主自动通过 |
| V-10 | 10～12、22 | 两次运行、会话重建、旧队列迟到、revision切换、无数据及事件/可靠拥堵 | run_id非零且同会话递增；记录索引含丢失缺口；上传携带原槽版本；无数据≤5ms调度尝试并等许可；结果/事件优先 | 宿主自动通过 |
| V-11 | 13～18、22 | runtime_tuning真实C：八项边界/类型/目标、fan_idle变化、一次pending、1000/1001等待边界、元素期间开关、断网/禁用/停车 | 整组拒绝或整组应用；RECEIVED不代表生效；下一完整允许拍APPLIED含真实values/revision/tick/控制拍；历史保持，last_success独立 | 宿主自动通过 |
| V-12 | 01、16、17 | 旧control_equivalence、dual_mode_tracking及新增逐拍对照；普通→十字/预环岛/右环岛→普通；两模式两档 | 无调试全部输出/部分覆写/历史等价；开启时切换拍暂停、待请求终结、恢复最后值；每拍每套FSM/history仅推进一次 | 宿主自动通过 |
| V-13 | 09、18～22 | 停车配置/自动保存/失败；无成功调参；有效/无效日志元信息；导出冲突起跑/清日志及错误路径 | 日志先收尾后保存；无成功调参无额外写；v3逐字节兼容；运行META零Flash访问；导出所有入口互斥；失败非SAVED且锁可释放 | 宿主自动通过 |
| V-14 | 02～04、23 | 逻辑分析仪CS/SCK/MOSI/MISO/READY，低/高速；断线/CRC错/DMA超时/ESP重启 | Mode0、实际SCK符合记录；CS内恰1024B、无旧许可复用；故障恢复可握手，控制周期与停车条件不新增 | 待实机 |
| V-15 | 10～12、19～21、23 | 有效标定环境60秒1kHz，与关闭上传对照；人为堵塞READY/UDP，统计Flash/上传/导出CRC | 约60000条对应实际采样区间，无新增Flash丢样/误停车；拥堵只增上传丢弃；导出总长与CRC一致，完整计数可解释 | 待实机 |
| V-16 | 01、16、17、23 | 实机测灰度扫描和完整TIM6，含普通、切换、应用请求和通信拥堵；记录最大值与工况 | 灰度≤700μs且不越原达到预算时超时边界，完整TIM6<900μs；现有达到900μs保护仍有效；无测量不得通过 | 待实机 |
| V-17 | 20、21 | spi_log_export真实C：0条/尾部/最大条数、1/960边界、越界/整数溢出、重复块/请求、乱序、提前END、Flash读错、10000ms无新读取、会话改变 | 原256B头+成功记录逐字节一致；总CRC独立一致；重复不推进CRC/续租，提前END拒绝；失败与超时解锁并喂狗；无整份RAM副本 | 宿主自动通过 |
| V-18 | 01、24 | CS逐项检查、中文联调说明、三端分阶段联调与所有证据复核 | CS-01～15均有结果/例外；先向量→握手→实时/导出→调参；AC5/实车未执行明确保留，阶段仅全部门禁完成才勾选 | CS/说明/自动证据通过；三端实机待验 |

## 实施时的执行入口

G2 V-08～10补验：256槽后第257条丢弃且next_index仍增加；head/tail跨UINT32_MAX；
同run版本断点提前封包；9/10ms时刻、32事件后显式gap、会话清缓存不改revision；
writer溢出整条失败，float用Python独立binary32往返比对。旧回归新替身仅隔离通信副作用。

V-10前台周期按2026-10-18用户确认：前台可运行时4ms不发/5ms到期尝试，等待READY；
模拟Flash/LCD阻塞超过5ms后首个服务立即尝试，不报告阻塞区间硬实时5ms通过。

V-06增量：当前会话合法帧后799ms保持；800/801ms重握手；连续坏CRC、旧session及空DMA
不能续期；握手每200ms发HELLO。比较有效参数与运行标志，确认超时不停车/不回滚。

V-01增量：同时检查CC与LD原始日志；核对_sbrk仍来自相同工具链的sbrk.o，不以脚本末尾
文字替代零警告证据。V-05确认协议数字转换不引用strtod/malloc，不借构建整改改变运行库。

V-02/FR-23栈增量：确认两个工程的栈均0x2000；GCC静态对象/堆预留低于0x2407D000，
栈顶0x2407F000正好位于TX区域下方，TX/RX不与栈重叠。AC5另验，不预填通过。

从SmartCar目录执行，记录运行日期、提交、完整命令、工具版本、摘要及原始日志路径：

```text
MDK-GCC/build.sh                 # Git Bash；LCD两配置串行执行并恢复原值
python tests/control_equivalence.py
python tests/dual_mode_tracking.py
python tests/datalog_v2.py
python tests/scb_protocol.py
python tests/scb_link.py
python tests/spi_wifi_state.py
python tests/runtime_tuning.py
python tests/spi_log_export.py
```

后五项为计划入口。测试必须编译/运行对应真实C，包含资源耗尽、时间边界与错误副作用。
PC既有测试可补充互操作，不能代替上述STM32检查。本轮不运行固件构建或功能测试。

## CS-01～15逐组记录模板

每实现组单独填写：

| 范围 | 要求 | 实际结果 |
|---|---|---|
| CS-01/02 | 本组修改路径、排除的生成区/第三方、先行规约提交号 | 待实现 |
| CS-03～05 | 4空格/独立括号/行宽/换行及来源文本例外 | 待实现 |
| CS-06～09 | 公私有命名、类型/对象/宏、接口与文件组织 | 待实现 |
| CS-10～13 | 逐函数C完整/H摘要、并发/阶段/失败解释；覆盖、格式、人工内容分别结论 | 待实现 |
| CS-14/15 | HAL/IRQ固定名、调用方、行为不变量与对照结果 | 待实现 |

## 本轮实际记录

### G1 实施记录（2026-10-18）

先行提交：7067039、432c183、240321e（内部接口）、0c30587（用户确认800ms）、
8a8fcf3（链接门禁）、3ecbf34（用户确认8KB栈）。本组尚不接入App主循环，G2接线。

- V-03：`tests/spi_wifi_state.py`编译并运行真实bsp_wifi.c，启动低/持续低、在途两个新沿、
  HAL_BUSY保留许可、启动错、错误回调、迟到完成、9/10/11ms中止全部通过。IRQ无等待，
  HAL中止只在前台；超时不修改车辆状态。
- V-04/05：`tests/scb_protocol.py`最终75项通过；13帧固定输入逐字节编码比对，C解码字段
  与独立Python struct期望逐项比较，CRC独立用zlib核验。补验8/9层、128/129令牌、
  63/64字节数字、992/993负载、转义等价重复键、非法UTF-8/NUL、NaN/Inf和缺项类型错误。
  JSON使用静态显式容器栈，数值转换不调用strtod/malloc；没有自我往返替代独立期望。
- V-06/07及V-08缓存部分：`tests/scb_link.py`通过；重复HELLO_ACK、精确/错误ACK、
  200ms三次逐字节重发及失败重握手、高序号IDLE不屏蔽可靠重发、32槽预留耗尽BUSY、
  重复编号同/异内容、旧会话/坏JSON不ACK、799/800ms、毫秒回绕及seq边界均通过。
  事件容量及业务副作用将在G2/G3继续验证，不把本组子集记为整条V-08完成。
- 三个既有入口`control_equivalence.py`、`dual_mode_tracking.py`、`datalog_v2.py`通过。
  Windows默认GBK导致首次控制测试读取Git源码失败，使用PYTHONUTF8=1重跑通过；
  一次临时文件清理遇Windows文件占用，原命令重跑通过，没有修改或放宽既有断言。
- GCC工具链实际为既有EIDE `arm-none-eabi-gcc 15.2.1`，未升级；宿主为MinGW gcc。
  双LCD日志路径`MDK-GCC/build/g1-lcd0.log`、`g1-lcd1.log`，编译与链接分别核对。
  初次出现8条libnosys链接warning，已按8a8fcf3修正依赖选择和统计，不屏蔽warning。
- V-02：map确认TX=0x2407F000、RX=0x2407F400，各0x400且32B对齐；ADC仍0x30000000。
  RAM为508KB，DMA另4KB，栈顶0x2407F000、预留0x2000，两工程配置一致。
  LCD1 text/data/bss=72892/308/119944B；G1尚未接线的协议缓存可被gc回收，此数不是G5
  最终最大RAM结论。SPI1共享PLL、SPI45补丁、NVIC 0/0/1及ADC优先级2未改。
- AC5无执行条件，绝对at声明及Keil源清单已核对，AC5 map/编译仍待验；SCK仅按源码
  HSE25/M5×N192/Q2=480MHz核算，分频256/64对应1.875/7.5MHz，波形与寄存器实测待验。

| G1 CS项 | 实际检查结果 |
|---|---|
| CS-01 | 新增App/scb_protocol、scb_link与BSP/bsp_wifi六个C/H及bsp.h；Core生成区仅DMA/IRQ同步，第三方未改 |
| CS-02 | 功能入口及以下逐函数清单分别检查，不以构建代替注释验收 |
| CS-03 | 六个新C/H零Tab，4空格、独立大括号、一行一条执行语句通过 |
| CS-04 | 运算符、指针、调用、sizeof空格人工复查通过 |
| CS-05 | 六个新C/H普通行均≤100字符，无行尾空白，UTF-8无BOM；修改既有文件保留原换行 |
| CS-06 | 47个新定义采用SCB_/BSP_WiFi_，其中三个HAL回调按CS-14保留固定名 |
| CS-07 | 自有类型scb_*/bsp_wifi_*及_t；成员小写下划线，静态对象s_，无新增外部可写全局 |
| CS-08 | SCB_/BSP_WIFI_常量与SMARTCAR_*保护宏通过；线协议字段偏移未借改名调整 |
| CS-09 | 文件说明→包含→私有类型/对象→声明→定义；App仅经bsp.h访问硬件 |
| CS-10 | 六个文件各有中文职责/边界说明，C说明数据流与并发所有权 |
| CS-11 | 下列47个定义逐个检查C完整/H摘要；纯getter/字节访问器/注册setter省略不适用details已按类别复核 |
| CS-12 | 新类型逐字段中文说明；静态状态及静态局部表均说明上下文；CRC、许可、缓存预留、显式栈有连续阶段解释 |
| CS-13 | 本组未修改休眠代码、来源或标定；覆盖/格式/人工内容分别通过，功能行为由独立宿主另验 |
| CS-14 | HAL三个回调及三个IRQ入口为外部固定名；Core生成区、工具链段名、ARM属性保留 |
| CS-15 | 新行为限FR-02～08/23；三个既有回归通过，参数v3/日志v2与保护阈值不变 |

逐函数覆盖/格式/人工内容清单（三项均逐个通过）：

- scb_protocol.c（21）：ReadU16、ReadU32、WriteU16、WriteU32、CalculateCrc、BuildFrame、
  EncodeFrame、DecodeFrame、ValidateUtf8、SkipSpace、ReadHex、AppendUtf8、ParseString、
  ParseNumber、ParseValue、ParseJson、FindMember、GetString、GetNumber、StatusName、ParseCommand，
  均为SCB_前缀。四个字节访问器及GetString/GetNumber/StatusName按纯访问器审查；其余
  均检查完整处理阶段、容量/错误保留和输入输出，不以标签数量判内容。
- scb_link.c（14）：SCB_FindRequest、AllocateRequest、SetHandlers、ResetSession、Init、
  QueueResult、GetRequestResult、AcceptRequest、HandleAck、ReceiveFrame、StartReliable、
  PrepareFrame、Service、GetStatus。SetHandlers为注册setter，GetStatus为纯getter；其余
  分别检查会话取消、未完成项不可淘汰、精确ACK和在途缓冲说明。
- bsp_wifi.c（12）：BSP_WiFi_SetCs、Init、GetTx、GetRx、ReleaseRx、TryExchange、Service、
  GetStatus、SetFastClock及HAL_GPIO_EXTI_Callback、HAL_SPI_TxRxCpltCallback、HAL_SPI_ErrorCallback。
  SetCs为单调用转发，GetTx/GetRx为getter；其余检查许可所有权、超时恢复及IRQ边界。

双LCD最终均0警告0错误，LCD0 text/data/bss=70088/300/119928B，配置恢复0；两次链接日志均空。
G1代码完成不等于Phase完成；G2～G5、完整业务验收及全部硬件门禁继续未完成。

### G2 实施记录（2026-10-18）

先行G2规约39bd004；前台5ms阻塞边界经用户确认并先提交1c40d62。

- V-08～10：`scb_link.py`运行真实scb_link.c/telemetry.c/scb_protocol.c；验证256槽全容量、
  第257条上传丢弃但索引递增、32位head/tail回绕、旧run消费丢弃、revision断点21ms提前
  封包及正常9/10ms边界、下一运行索引清零、会话清队列保留revision。
  10条984B负载与公共固定输入逐字节比对（ESP计数由STM32置0），通过。
- 同一真实链路测试验证32条事件满后两次遗漏以EVENT_GAP/dropped_events=2报告，
  结果优先于事件、事件优先于上传；事件可靠ACK后再释放传输。4ms不空轮询、5ms到期、
  无READY不发，模拟阻塞结束首个前台服务立即尝试，均通过；不声称阻塞期间5ms硬上限。
- `scb_protocol.py`594项通过（原75项加519个binary32样本），含正负极值/次正规数、
  随机位型、NaN/Inf拒绝和writer溢出不写前缀；Python独立解析输出后重打包binary32比对。
- 三个旧回归通过。`dual_mode_tracking.py`仅补通信依赖替身及新增观察，验证实际成功
  起跑才分配run_id，两态Loop调用通信、重复保护不重复发停车事件；旧断言全部保留。
  `datalog_v2.py`最终接入真实telemetry.c和scb_protocol.c，不再替身上传队列；上传满256
  时Flash仍成功写入1050条、无新fault；87336容量/故障/CSV断言仍全部通过。
- 双LCD全量GCC编译/链接均0警告0错误，LCD0 text/data/bss=82000/300/224136B，LCD1
  =84808/308/224152B；恢复LCD_TARGET_EXTERNAL=0。原始日志`MDK-GCC/build/g2-native.log`、
  `g2-lcd0.log`、`g2-lcd1.log`；日志集成真实上传的最后一次独立重跑也通过。
- map/nm：请求缓存64000B、上传槽27648B、32事件1920B、两个JSON区各4080B、可靠副本
  1024B；DMA地址/32B对齐、ADC D2、8KB栈及其边界不变。此为G2占用，G5还须复核。
- AC5、60秒吞吐、逻辑分析仪与700/900μs实测未执行，标定仍为-1，不允许起跑绕门禁。

| G2 CS项 | 实际检查结果 |
|---|---|
| CS-01/02 | 上传/协议/链路、App与日志接线、Keil、必要宿主夹具；先行39bd004/1c40d62，未改第三方与硬件 |
| CS-03～05 | 新C/H及新增行4空格、独立括号、无Tab/行尾空白、普通行≤100；已有文件换行保持 |
| CS-06～09 | 新增16定义命名、_t类型/字段/对象及保护宏通过；writer类型置于接口之前，App只经bsp.h访问硬件 |
| CS-10 | telemetry C/H说明SPSC与数据流；改动函数说明包含上传分流、事件及前台调度边界 |
| CS-11 | 16个新定义逐个检查C完整/H摘要，8个改动定义复查；覆盖/格式/人工内容分别通过 |
| CS-12 | 槽和事件字段逐项说明，静态队列/索引分属生产消费；DMB、丢弃缺口、浮点缩放与gap阶段解释通过 |
| CS-13 | 未改休眠/来源/标定；G1摘要定位检查漏掉精确函数名，本组已给44个新模块H接口补齐.c::函数定位，纠正该检查漏项 |
| CS-14/15 | 固定HAL/IRQ未新增改名；新增行为限FR-10～12/22与JSON输出，旧三个回归及记录布局断言通过 |

逐定义覆盖/格式/内容检查均通过：SCB_JsonBegin/Append/U32/I32/Float/Values（6），
SCB_QueueEvent/TakeEvent/FormatEvent（3），Telemetry_Init/BeginRun/PushRecord/Service/
GetStatus/SetRevision/ResetSession（7）。其中SetRevision为单赋值发布，其他并发/计算函数
均有阶段和失败语义。改动定义为SCB_Init/ResetSession/PrepareFrame、App_Init/Loop/TryStart/
RequestStop、Datalog_Push（8），分别核对新增调度/快照/分流与原错误路径。

G2自动门禁通过；G3方向恢复语义的必要问题等待用户确认，未开始相关控制改动。Phase仍不完成。

- 已完成规则/既有规约/任务书及实际源代码只读调查；仅修改文档。
- SPI时钟派生记录不一致、AC5默认存储不能证明DMA可达、方向先计算后转态、风扇早于观察等
  风险已明确写入plan；没有通过修改实现掩盖风险。
- 代码编译、宿主功能、AC5、逻辑分析仪、60秒吞吐、700/900μs、三端与实车均未执行。
  32项灰度标定仍不在本轮范围，既有阶段待验状态不变。
- 文档完成不等于G1～G5启动或Phase完成。

### D-02/03/05校核证据

本轮以一次性标准库检查从stdin执行，不写测试代码、不导入既有scb_protocol：

- `Path.read_text`提取三份任务书与protocol.md公共节，统一换行后逐字比较。
  SHA256=`16a3c43446e8846387ba463be89bb802db222e8ded0f95478e49c1d49291caea`。
- 检查连续FR-01～24/V-01～18与本目录全部16个Markdown本地链接；人工逐行复核plan映射
  与validation刺激/标准，任务书A～D和公共节均有对应需求。
- 用`ast.parse/literal_eval`仅读scb_vectors.py固定常量；`struct.calcsize`头长32，逐位
  反射CRC（多项式0xEDB88320、初末异或0xFFFFFFFF）与`zlib.crc32`交叉核对13帧。
  IDLE/GET_STATUS/SET_PARAMS/TELEMETRY1/TELEMETRY10/LOG_DATA分别为32/61/192/152/1016/72B，
  CRC分别8940aca1/94efa91c/98ef9c12/5daa3261/c58d37ba/8105364a；错误CRC与错误长度按预期
  检出，重复可靠帧2条、重复request_id3条保留原字节。
- 系统python命令是不可执行的WindowsApp别名；改用本机会话依赖的Python运行，校核成功。
  这只是文档输入校核，不是V-04的STM32真实C协议功能通过。

### 文档组织调整复核（2026-10-18）

用户要求不保留独立交接文件，直接提供可复制的执行提示词。本次删除该文件，清理需求与
路线图中的入口，并同步G0说明及CHANGELOG；功能需求、公共协议和后续V项不变。
上方D-03的16个链接是7067039提交时的历史结果，保留原记录。本次检查剩余四份规约的全部
本地链接和删除文件的引用：剩余四份规约的10个本地链接全部有效，SmartCar/Reports的Markdown
中无删除文件的名称引用，`git diff --check`通过；不运行固件构建或功能测试。

## G3 待执行门禁

真实C方向输入/单次历史、切换拍暂停恢复、原子结果、会话/保护取消、停车保存及失败；
固定向量/SCB/SPI、既有三项、双LCD构建、CS-01～15。尚未执行，不预填通过。

## G3 实际执行记录（2026-10-18）

先行规约e380a54。真实C新增runtime_tuning.py通过：双模式独立方向方程检查首拍包含KD、
次拍差分历史不重复；进入十字当拍拒绝pending并恢复共享基线，退出同拍恢复last_success；
禁用回原FSM速度、会话清未应用项不回滚、保护先停车不APPLIED、停车只写一次正确bank，
参数失败保持原起跑门禁。初次失败定位为测试夹具将横右索引3误作竖右2，修正夹具后通过，
未改阈值或放宽断言。保存事件values取成功持久化快照，不能混入特殊元素effective。

七个宿主入口scb_protocol/scb_link/spi_wifi_state/control_equivalence/dual_mode_tracking/
datalog_v2/runtime_tuning全部退出0，日志MDK-GCC/build/g3-native.log。旧双模式逐拍对照、
-1标定拒绝两模式起跑、所有原保护/日志87336容量断言保持。
双LCD GCC（含链接）0 warning/0 error：LCD0 text88444/data300/bss230464；LCD1
text91100/data308/bss230488。日志g3-lcd0.log/g3-lcd1.log。AC5/实机仍未执行。

### G3 CS-01～15及逐函数审查

| 项 | 实际检查 |
|---|---|
| CS-01/02 | 仅App控制/调参、Keil清单及测试/规约；新设计先文档提交，记录与实现对应 |
| CS-03/04 | 新增4空格、Allman、控制语句有括号；逐字段声明与运算空格检查 |
| CS-05 | 新增代码/注释≤100字符；app原stop串口完整格式串104字符属既有例外；保留原换行符 |
| CS-06～09 | Tuning模块动作名、s_状态、_t类型、独立头保护；未新增旧名兼容宏 |
| CS-10 | tuning C/H职责与并发、两个FSM拆分说明、App/控制调用边界逐模块检查 |
| CS-11 | C完整/H摘要及.c::函数定位；getter省details，其余新函数有阶段/错误/共享约束 |
| CS-12 | 参数/结果类型字段中文说明；pending、final、baseline与last_success归属及发布解释 |
| CS-13 | 私有与公共函数人工内容审查，算法出处保留；休眠元素没有新增接入或删除 |
| CS-14 | HAL固定回调/定时器/工程/Flash格式名称保持，新增文件显式Keil登记 |
| CS-15 | 原历史输出对照及原断言全通过；显式功能变化仅调试覆盖、快照/结果/保存 |

逐定义覆盖/格式/人工内容/行为检查清单（均已检查；功能变化按规约，旧包装按等价门禁）：
- tuning.c全部26定义：IsOrdinary、ReadActive、ApplyShared、Equal、Event、Begin、Send、
  Position、Merge、RejectPending、SelectControl、GetControl、CompleteControl、Stop、
  FinalizeStop、ResetSession、GetStatus、Service、ReadBank、QueryParams、QueryStatus、
  QueryResult、Set、Switch、HandleCommand、Init（均为Tuning_前缀）。IsOrdinary/ReadActive/
  Equal/GetControl/GetStatus/ReadBank为小型读取或字段操作；状态/错误路径其余逐块说明。
- electromagnetic_fsm.c：Select/Calculate/Update；删除的旧UpdateDirection由Calculate取代。
  其余GetState/Reset/Accumulate/GetDistance/ClearDistance/ClearAngle/Measure人工核对不变量。
- optical_fsm.c：Select/Calculate/Update；GetState/Reset保持。
- control.c：Tracking_Observe/Execute；app.c：Init/Loop/ControlISR/RequestStop/FinalizeStop；
  param.c：GetSpeedGains/SetSpeedGains；scb_protocol.c：ParseCommand/ValidateValues。
  对应H摘要定位已逐项核对；镜像类型、ISR优先级、保护及历史复位均保持。

G3覆盖FR-09调参/查询部分、13～18/22。FR-19日志有效头与导出查询由G4接续，不把当前
GET_STATUS尚缺的log_valid当成完整通过；Phase未完成。

## G4 待执行

V-13/17按上述缓存头/顺序流/全入口锁/故障前缀时间轴/超时释放细化执行，
新增spi_log_export.py；未经实际运行不预填通过。

## G4 实际执行记录（2026-10-18）

先行规约e17c8bf。spi_log_export.py编译真实App/Datalog/Tuning/SCB协议C，在模拟Flash
确认0/3/87336条及写失败后8条前缀逐字节输出；Python独立zlib校验256+96*N与整体CRC，
保留tick/control_sequence时间轴，3条样本控制序号确为[1,1,2]，未重排/补样。运行META
及STATUS查询零Flash读，头512位文本与实际原字节一致；未建/清日志后valid=false。
真实起跑、Datalog_Start/Clear均受锁；测试最近重复块不读Flash、不推进CRC/不续租，
乱序/0/961长度/UINT32_MAX越界、提前END、错误transfer_id、读错解锁并喂狗、会话取消、
tick回绕及9999/10000ms边界全部通过。写失败pending=2保留、fault=WRITE未被导出改变。

八个宿主入口全部退出0，原三回归及G3原子调参均通过；日志g4-native.log。增加STATUS和
精确控制序号断言后spi_log_export独立复跑通过。双LCD GCC（含链接）零警告零错误：
LCD0 text90252/data300/bss231736，LCD1 text92916/data308/bss231752；日志g4-lcd0/1.log。
所有记录位于MDK-GCC/build；AC5/硬件仍待验，无阶段勾选。

### G4 CS-01～15与定义清单

| 项 | 实际结果 |
|---|---|
| CS-01/02 | 仅App日志/调参/起跑锁、三测试依赖及规约；e17c8bf先行，结果真实回填 |
| CS-03/04 | 新增4空格/Allman/单声明/有括号控制体，逐行空格及git diff --check通过 |
| CS-05 | 新增≤100字符；既有App stop104和CSV status106完整格式串属内容例外；原换行保持 |
| CS-06～09 | Datalog/Tuning模块动作、_t类型、s_前台状态；新公开类型位于接口前，App经bsp |
| CS-10 | 日志缓存/导出职责及前台独占边界审查，未新增第三方/生成代码修改 |
| CS-11 | 8个Datalog新接口C完整/H摘要，2个Tuning私有分支完整，逐项.c::定位核对 |
| CS-12 | meta/export字段含计数/身份/字节单位；CRC、重复不续租、防溢出、失败释放连续说明 |
| CS-13 | 按每个定义人工审查成功/错误输出、所有权及时间轴；无休眠入口改动 |
| CS-14 | HAL/IRQ/工程名字不改；本组无新增源文件需登记，既有Keil登记保持 |
| CS-15 | 原容量/头/记录/故障/保护及三线不变；新行为仅已授权日志互斥/导出 |

逐定义覆盖、格式、人工内容和行为核对：Datalog_GetMeta、CancelExport、ServiceExport、
IsExporting、GetExportStatus、BeginExport、ReadExport、EndExport；后两getter仍明确服务
租期副作用，Read的输出容量和失败写0长度逐项说明。修改Start/Clear在任何状态改变前拒绝。
Tuning_QueryLogMeta/LogCommand、QueryStatus/ResetSession/Service/HandleCommand与
App_TryStart接线人工检查。Datalog类型/字段和对应H全部摘要核对；其余旧函数不扩改。

FR-09、19～21对应V-13/17自动部分通过，FR-23静态RAM/栈最终预算及中文联调由G5汇总；
三端真实联调、AC5及V-14～16不因宿主通过而通过。

## G5 待执行

补两模式两档/右环调参覆盖，最终八入口/双LCD/静态RAM及栈取证；随后据实际结果
更新总门禁表、中文联调和硬件待验清单，不预勾Phase。

2026-10-18 G5新增已确认用例：停车预开启后切换mode/gear，再次ENABLE绑定新目标；同目标重复幂等及原基线保持，待实际执行。

2026-10-19 G5新增帧身份边界待执行：握手/正常session、0序号及请求ID按公共既定规则拒绝；独立重算CRC，合法固定向量保持。

## G5 最终执行与交付（2026-10-19）

先行规约9876e24、701230f、6620248。已执行：

```text
python SmartCar/tests/phase16_check.py --build --budget
```

本机解释器为Codex运行时Python 3.12.14，宿主MinGW GCC 8.1.0；ARM为现有构建脚本所指的
Arm GNU Toolchain 15.2.Rel1 / GCC 15.2.1，未升级。原始输出在MDK-GCC/build/g5-*.log及
stack-final/，可复核的[机器证据](../../../tests/results/phase16.json)记录日志与关键源SHA256、
实际nm符号、各帧大小及规划预算。最新ELF/map另存MDK-GCC/build/phase16-artifacts/，
SHA256已与实际构建产物逐字节核对；归档对应最后构建的LCD1，源配置已恢复LCD0。
不把ELF、map、对象或临时夹具纳入源码提交；上机前按实际LCD单独构建再烧录。

八个入口scb_protocol、scb_link、spi_wifi_state、control_equivalence、dual_mode_tracking、
datalog_v2、runtime_tuning、spi_log_export全部退出0。协议608项含不可变13帧、独立zlib、
非法UTF-8/JSON/身份字段、519个binary32输出对照及各资源边界；新增坏IDLE即使CRC正确也
不释放可靠在途、不续租。全部合法向量字节保持。两模式×两档的首拍方向/次拍历史、十字
及预环/右环全过程、原FSM私有配置、最后成功值/停车保存均通过。停车切换目标后再ENABLE
绑定当前新目标，同目标重复及禁用恢复拍前重开不漂移基线；最大合法GET_PARAMS及缓存查询
完整可解析，非有限旧bank的编码错误返回原GET_PARAMS操作名。旧回归断言均保留。

| 最终GCC配置 | text | data | bss（含专段/堆栈预留） | 编译/链接 |
|---|---:|---:|---:|---|
| 板载LCD=0 | 90556 | 300 | 231736 | 0 warning / 0 error |
| 外接LCD=1 | 93220 | 308 | 231752 | 0 warning / 0 error |

结束恢复LCD_TARGET_EXTERNAL=0。公共protocol.md及三份来源任务书与7067039比较无差异，
其他端没有修改。G5期间发现的Hardware/STM32H7_Car/STM32H7_Car.kicad_pcb在途修改保持，
只检查Git边界，不把该文件纳入提交。

### RAM、DMA与时钟核对

| 对象/区域 | GCC实际事实 |
|---|---|
| 原Flash日志队列s_buf | 98304B，1024个96B槽；原有效1023条机制保持 |
| 上传s_slots | 27648B，256×108B，逻辑256条全可用 |
| 请求s_requests | 64000B，32份原文/结果各992B及元数据；pending不可淘汰 |
| 事件s_events | 1920B，32个ARM固定描述，另保留gap描述；不存32份完整JSON |
| 导出s_export_block | 968B，8B头+960B数据；无整份日志副本 |
| JSON工作区 | command/query各4080B，hello另4080B；全部静态，容器栈有界 |
| SPI TX/RX | 0x2407F000 / 0x2407F400，各1024B、32B对齐；AXI，非DTCM |
| ADC DMA | s_adc_buf=0x30000000，20B；D2专段含对齐，不与SPI DMA相交 |
| AXI静态对象末端 | LCD1 _ebss=0x2403605C，堆起点end=0x24036060 |
| 栈 | GCC顶0x2407F000、下界0x2407D000；GCC及Keil声明均8192B |

508KB普通AXI与顶部4KB Wi-Fi保留区分开；扣除原512B最小堆预留后，堆预留末端到栈下界
仍有290208B间隔。size的bss包含最小堆/栈标记及DMA专段，不能把它全当作运行期对象。
ARM map已核对，AC5 map未取得，不能用上述GCC位置冒充AC5结果。

SPI2源码为HSE25/M5×N192/Q2=480MHz，SPI123选择PLL1Q；启动分频256=1.875MHz，显式
联调快速分频64=7.5MHz。仅为源码核算，未测SCK、未自动切高速。SPI45的既有PLL3Q80MHz
初始化和App重写选择补丁保持，未按.ioc旧SPI45派生值改共享PLL。控制/采样/辅助优先级
0/0/1及1/2/5ms不变，ADC DMA2、SPI/DMA3、READY4均核对；D-Cache未启用，EN不自动复位。

### GCC栈规划预算（不是实测高水位）

同一-O2/硬浮点配置编译74个App/BSP/Core/HAL源，所有.su帧均为static，无动态栈报告。
以LCD1实际ELF调用边和明确回调集合合并，尾调用仍累加，HAL公共分发器按所有分支取最大，
所以TIM7/DMA的表值是保守合并值，不声称它们实际调用了另一外设的分支。

| 路径 | 源码帧累计B |
|---|---:|
| main→App_Loop→SCB_Service→ReceiveFrame→Tuning_HandleCommand→ParseJson→ParseValue→ReadHex | 864 |
| TIM15最深保守路径（含日志满/停车分支） | 296 |
| TIM6最深保守路径 | 256 |
| TIM7公共HAL回调合并上界 | 256 |
| DMA1 Stream0公共回调合并 | 64 |
| DMA1 Stream1/2公共回调合并 | 64 |
| SPI2 | 32 |
| USART1 | 64 |
| READY EXTI | 8 |
| SysTick源码帧 | 0 |

按前台864 + 优先级0最大296 + 优先级1最大256 + 优先级2最大64 + 优先级3最大64 +
优先级4最大8，源码帧合计1552B。每层额外按112B覆盖基本/浮点硬件上下文及对齐，共560B；
库函数及未列间接显示/printf调用另预留2048B，规划总计4160B，低于8192B预留，余量4032B。
新JSON解析不递归，不调用strtod/malloc；本预算中2048B是明确规划预留，不能当成经形式
证明的库上界。库/间接调用、AC5差异及真实中断栈高水位仍需实机核验，未伪报测量通过。

### G5 CS-01～15与逐定义审查

| 项 | 实际结果 |
|---|---|
| CS-01/02 | 9876e24/701230f/6620248均先行纯规约提交；只改本任务C/测试/证据/文档 |
| CS-03/04 | 新C四空格、Allman、括号控制体/单声明/运算空格，最终逐行与差异检查通过 |
| CS-05 | 新增C/注释≤100字符，原换行保持；无新增格式串例外 |
| CS-06～09 | SCB/Tuning既定名称与文件边界保持；未引入旧名宏或改变链接属性 |
| CS-10 | 现有模块职责不变；机器检查脚本说明运行范围和硬件不执行边界 |
| CS-11 | SCB_BuildFrame、Tuning_Send/Switch及所有Send调用点C完整标签/含义人工检查；H无新接口 |
| CS-12 | 握手/正常身份、原op错误结果、基线幂等及重武装步骤连续解释，静态所有权保持 |
| CS-13 | 格式、覆盖、人工内容和行为分别检查；库栈预留不写成测量事实，无休眠例外新增 |
| CS-14 | HAL/IRQ固定名、.ioc/链接段及Keil栈符号保持；本组无新增固件C文件需登记 |
| CS-15 | 原三回归全通过；功能修正仅既定非法身份拒绝、用户确认目标绑定及错误op保持 |

逐定义覆盖：SCB_BuildFrame的header规则/失败不发布；Tuning_Send的新增op形参、溢出整条
失败及九处调用；Tuning_Switch的同目标幂等、停车新目标与运行拒绝分支。读取/格式化/
应用职责未混入IRQ。tuning其余修改仅调用参数同步，26个原定义及两个日志私有定义仍有效。
新测试和机器证据不属于固件CS范围，不用其通过代替C注释人工内容检查。

### 中文联调顺序与未执行项

1. 先运行上述完整验收入口，检查共同向量、三旧回归和两LCD构建；确认配置已恢复，
   保持32项真实标定为-1，绝不为通信测试填示例值或绕过起跑门禁。
2. 停车接SPI2：Mode0、CS内1024B，先1.875MHz；用逻辑分析仪核对CS/SCK/MOSI/MISO/READY。
   启动低电平补获一次，持续低不得重复，在途新下降沿应保留。记录RCC及实际SCK；通过后
   才显式调用快速分频入口测7.5MHz，不改变PLL或ESP EN。
3. 以HELLO(session0/seq0)→非零HELLO_ACK→IDLE(seq1)握手，GET_STATUS/GET_PARAMS核对
   run_id、mode/gear、两套元素、tune_state、revision及limits。同request原文重发只取缓存，
   异原文INVALID_ARG；丢ACK、CRC错、DMA不完成及ESP重启应恢复，不增加停车条件。
4. 由操作者实测并维护32项黑白标定，满足原对比度门禁后再开始真实运动验收。原PC13起跑
   路径及1000/1001控制拍延时保持；停车预开启后按当前目标测试，换目标需再次ENABLE。
   运行SET必须八项完整且fan_idle不变，观察RECEIVED→真实完整拍APPLIED，记录JSONL事件
   和实际tick/control_sequence；TUNE_DISABLE方向/速度回原FSM，PI/风扇回捕获基线。
5. 覆盖十字/预环/右环同拍暂停及恢复，核对每拍只选择/计算一次；断网800ms重握手不回滚。
   停车先日志收尾再保存最后成功值；看到PARAM_SAVED才能声称Flash成功，失败应为
   PARAM_SAVE_FAILED且保留原参数/日志门禁。Flash v3仍88B，日志v2仍256+96*N。
6. 运行GET_LOG_META只查缓存；停车收尾返回后LOG_BEGIN，按offset顺序LOG_READ(1～960)，
   拼接LOG_DATA去掉其8B头后的原字节，全部交付后LOG_END并以zlib核对总长/CRC。
   导出间尝试所有起跑/串口清日志入口应拒绝。重复块/普通查询不续租，10秒无新读取或换
   SPI会话释放。未定义TCP内部取消通知，按已确认10秒释放路径，不假装检测到TCP断开。
7. 在有效标定工况做60秒1kHz，比较实际采样区间、Flash已写条数、上传缺口、run_id和
   revision事件；堵塞READY/UDP只可增加上传丢弃，不能新增Flash丢样或通信停车。故障导出
   仅written前缀，tick及control_sequence原样保留，不把未写样本补入时间轴。
8. 实测灰度扫描及完整TIM6最大耗时，包含调参/切换/拥堵拍；保持原灰度达到700μs超时边界
   和完整控制达到900μs停车边界，不放宽保护。所有失联/导出租期由前台服务到期处理，
   保留既有Flash/LCD/串口阻塞；5ms只是前台可运行期间的到期尝试，不能声称硬中断保证。
9. 本环境PATH未找到armcc，未执行AC5或验证其授权；有条件后用现有Keil工程核对C99、
   零警告、DMA绝对段、8KB栈及实际map/高水位。GCC证据不能代替这些检查。

待验：AC5构建/map与栈；逻辑分析仪及RCC/SCK实际读数；60秒吞吐/对照；700/900μs最大
耗时与栈高水位；三端真实联调。V-14～16及V-18实机部分均未通过，Phase16不勾选。

### 主要实现提交

| 组 | 实现提交 | 自动门禁 |
|---|---|---|
| G1 | f531de9 | 六入口及双LCD |
| G2 | 069854f | 六入口及双LCD |
| G3 | 63fbde6 | 七入口及双LCD |
| G4 | 61e0b5c | 八入口及双LCD |
| G5 | 0925394（随后G5-R1更新最终机器证据） | 八入口/608协议项、双LCD、74源栈取证 |

规约增量240321e/0c30587/8a8fcf3/3ecbf34、39bd004/1c40d62、e380a54、e17c8bf、
9876e24/701230f/6620248均独立先行；基础规约7067039及组织调整432c183保持。
所有提交只本地commit，未push。源码以外的构建原始证据留build，不混入其他目录在途工作。

2026-10-19 G5-R1待执行：启动READY补获与EXTI同时到达竞态，注入各读取/清除边界并检查实际事务数。原G5自动结果保持历史，修复后重新记录最终结果。


## G5-R1 READY启动竞态实际关闭（2026-10-19）

先行规约edfa298。BSP_WiFi_Init在补获低电平时二次清EXTI，把第一次clear与GPIO采样之间
到达的同一下降沿合并，避免许可2导致多发一笔。高电平采样后的边沿不清，二次clear后的
新边沿留IRQ。仅上电短临界区变动，不增加ISR等待/控制停车，不变时钟、DMA地址或EN。

spi_wifi_state.py新增PR锁存/临界区模型，五组真实C注入均通过：旧pending+低电平=1、
旧pending+高电平=0、第一次clear后下降=1、采样高后下降=1、第二次clear后新下降=2。
逐许可验证实际DMA启动数并确认持续低不增许可。去掉修复块的旧代码负对照稳定在许可
断言失败，避免仅用简化替身掩盖竞态；原9/10/11ms、HAL_BUSY/错误/迟到完成等断言保留。

重新执行完整phase16_check.py --build --budget，八入口全通过，协议仍608项。最终LCD0
text90556/data300/bss231736，LCD1 text93220/data308/bss231752，均含链接0警告0错误。
74源均static栈报告，规划预算仍4160/8192B；DMA/map/内存地址及源/ELF/map SHA256已复核，
配置恢复LCD0，机器证据及原始归档均更新为本修复的实际产物。AC5及硬件状态不变。

CS-01/02：只改已先行规约的BSP初始化、对应测试和证据/回填；CS-03/04：4空格Allman且
分支有括号；CS-05：C全部≤100字符、保留原LF；CS-06～09：名字/类型/作用域/头接口不变；
CS-10/11：BSP_WiFi_Init的C完整说明明确两次清除时机，H原摘要契约不变；CS-12：临界区、
补获/IRQ合并与保留新沿逐步解释；CS-13：覆盖、格式、人工内容及旧版负对照分别检查；
CS-14：固定HAL/IRQ与资源配置不变；CS-15：仅修复既定一许可一事务，八入口及双LCD通过。
逐定义人工核对BSP_WiFi_Init，未改其余BSP接口、正常在途计数或前台恢复路径。

G5-R1对应本次fix提交，可由最终机器证据源SHA和git log定位；G5基线为0925394。
全部可执行实现/自动验证已处理，Phase仍因AC5/真实联调及V-14～16硬件门禁待验而不勾选。

2026-10-22 R2：新增R2-V01～06，方法、唯一通过标准及初始待执行状态见
[R2规约](r2-dma-recovery.md)。源码/宿主/Keil产物/板上更新/实机分别记账，不提前标通过。

2026-10-23 R4：新增R4-V01～07，方法、唯一通过标准及初始待执行状态见
[R4规约](r4-escalating-recovery.md)。外设重初始化兜底与调试计数器分别记账，
诱发源根因调查不在本批；实机复发数据回填R4-V07。

## R2实现前置风格验收（2026-10-22）

规约独立提交d28c4a4、12fe38b先于实现。范围仅BSP/bsp_wifi.c和对应Python内嵌C夹具；
不修改公共H、第三方HAL、Core生成区、协议、EN、时钟或其他端。CS-01/02范围核对完成。
CS-03/04：生产C新增局部对象逐条声明，4空格Allman、分支带括号及续行运算符；
CS-05：生产C无Tab、无普通超100字符行、无尾空白，保留原LF和末尾单换行；
CS-06～09：私有BSP_WiFi_AbortDma符合模块命名，局部lower_snake_case，未新增公开类型/
对象/宏/枚举，原包含顺序和文件边界不变。内嵌测试C不在CS-01产品代码范围。
CS-10：原模块说明仍准确覆盖前台中止、固定AXI缓冲、IRQ职责及不操作EN，无新增模块。
CS-11～13逐定义核对：AbortDma完整说明BUSY中止、READY与EN双重判断、真实返回语义、
句柄非空/已初始化前提、状态写入时机和有界等待；Service说明错误态先建立、三IRQ屏蔽、
两方向独立回收、失败保留与共同恢复IRQ。C为权威说明，H无新接口无需改写；标签格式、
覆盖和人工内容分别通过。阶段注释说明SPI请求位已关时仍需回收DMA及缓冲复用限制。
CS-14：原HAL固定名和寄存器/句柄名保留。CS-15：本批是先行R2批准的错误恢复行为修复，
并非纯风格保持；1024B、正常完成、启动许可去重、HAL_BUSY保留许可、参数/日志布局及
起停均不变。相同生产C夹具通过正常/故障/迟到回调测试，两种隔离负对照按预期失败。
已有Windows Clang/LLD替代宿主证据见Reports/link-validation/20261022-162357/r2-build；
不将其记为GCC或实机通过。原GCC入口因编译器不存在失败；Keil及板上状态另行回填。

## R2实际结果（2026-10-22，实施提交f857145）

| 验证项 | 实际结果 | 判定/限制 |
|---|---|---|
| R2-V01 | 同一生产BSP的TX、RX、双向BUSY残留均经显式中止，READY且EN=0后可凭新许可发1024B | 替代Clang宿主通过 |
| R2-V02 | TX/RX分别中止失败、EN未清、SPI中止失败、DMA非READY保持state3；双向均尝试且IRQ恢复 | 替代Clang宿主通过 |
| R2-V03 | 迟到回调不发布RX；失败后重试保留新READY；一次10ms超时仅计数一次，故障解除后继续 | 替代Clang宿主通过 |
| R2-V04 | 原正常/启动边沿断言通过；撤掉启动去重在permits断言失败，撤掉显式DMA回收在tx_dma.State断言失败 | 两个负对照符合预期 |
| R2-V05宿主 | MATLAB已有Clang 18.1.7/已有ESP Clang包LLD20.1.1编译同一C夹具，-O2 -Wall -Wextra -Werror；3个变体保存源码与构建命令 | 替代宿主通过；非GCC，非实机 |
| R2-V05 GCC | 原native gcc未安装；原MDK-GCC/build.sh因旧电脑/c/Users/Li编译器不存在中止 | 未通过，未安装/改工具链，不沿用旧门禁结果 |
| R2-V05 Keil | AC5 5.06u5 build528完整构建0错误5警告；五条与原构建逐行相同，新BSP无警告 | 构建通过，非零警告门禁通过 |
| R2-V05布局 | map TX/RX仍0x2407f000/0x2407f400各1024B；wifi状态仍0x24034924/24B；Code74926、RO3962、RW708、ZI521552 | 本批未改公共布局；不是栈/实时预算实测 |
| R2-V06 | 人工RESET后新会话1051912663，但GET_STATUS仍5秒无业务返回，随后5秒25笔均截短 | 旧固件复位恢复失败；新修复未烧录，未验收 |

证据根：`Reports/link-validation/20261022-162357/`。宿主摘要与3个build.json、Keil日志、
原/新HEX、map、风格检查、源码及产物SHA256在r2-build内。clang-host-summary.json记录
实际编译器版本，以机器证据为准。第一次受限Keil调用无法启动编译子进程，已保留失败
日志；正常本机环境重试完整构建成功。原GCC失败产生的唯一已跟踪构建日志已恢复为
执行前内容，失败输出另存Reports，不将构建副产物混入提交。

旧板HEX SHA256：db3877863e195a703dcd2d598ce8be9c4096526fdd1c09525ce2f10bf56c6a14。
新Keil HEX SHA256：c208713e70f895fb04cbbcb8588cb2ac6831d16eaed404b914c0004e08273c0d。
统一flash.ps1 -CheckOnly通过，无设备访问。当前板上仍旧版，未烧录；60秒遥测、日志导出、
波形和初始移位成因仍未验，不勾选Phase16，未启动ESP32 Phase06。
