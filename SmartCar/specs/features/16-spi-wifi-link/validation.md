# Phase 16 — 验证与执行记录

> 2026-10-18：仅文档。D项为本轮文档校核；V项是后续实现验收，全部待执行。
> 历史Phase和电脑端的通过记录不构成本阶段通过证据；不得预填实测数字。

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
新增入口是plan.md的目标路径，本轮未创建，不能把“文件不存在”记为测试失败或通过。
所有异常注入检查执行次数、结果、有效参数和Flash副作用，不只检查返回字符串。

| V | 对应FR | 方法与刺激 | 唯一通过标准 | 状态 |
|---|---|---|---|---|
| V-01 | 01、24 | 每实现组串行构建LCD_TARGET_EXTERNAL=0/1；结束恢复原值；检查新对象/Keil登记/布局 | 两配置均0警告0错误；新对象链接；无工具链升级；AC5另记实际结果 | 待执行 |
| V-02 | 01～04、23 | 比对.ioc/生成文件/IRQ/链接map；核对DMA、NVIC、EN及SPI45补丁 | TX1/RX2优先级3，READY4，ADC0优先级2及控制0/0/1不变；两缓冲32B对齐且在AXI，无DTCM | 待执行 |
| V-03 | 04 | spi_wifi_state真实BSP替身：上电低、持续低、在途新下降沿、快速高低、HAL忙/错、迟到完成、9/10/11ms超时 | 每新许可只发一笔；不复用旧许可；只允许单在途；完成才释放CS，10ms走前台中止且后续可握手；ISR无等待 | 待执行 |
| V-04 | 05、24 | scb_protocol：protocol.md全部固定向量；独立逐位CRC与字段期望交叉核对；1/10条、LOG_DATA与边界0/992/993 | 合法字节与公共向量一致，头32B，SPI1024B零填充；无结构体强转；所有记录偏移断言成立 | 待执行 |
| V-05 | 05、09、13 | 坏magic/version/flags/type/CRC/长度/count/UTF-8、重复键、NaN/Inf、深嵌套/截断/超长、半包命令 | 全部拒绝且不执行部分请求；非法SPI帧不ACK；合法最大消息不被解析上限误拒绝 | 待执行 |
| V-06 | 06、07 | scb_link可注入时钟：重复HELLO、重启、旧会话、seq回绕前、ack错误/精确、200ms×三重发、业务等待时IDLE确认 | 重发保持原seq/内容，最多三次，失败重握手；高seq非可靠帧不屏蔽可靠重发；旧会话无副作用 | 待执行 |
| V-07 | 07、08 | 丢ACK/结果、相同request原文重发、相同编号异内容、新TCP保持同SPI会话、换SPI会话 | 接受一次执行一次；精确ACK释放；缓存32项；查询可得最终结果，缺失NOT_FOUND；换会话取消未应用请求、不回滚有效值 | 待执行 |
| V-08 | 08、09、22 | 填满结果/事件/请求缓存，接收SET_PARAMS后暂不消费，事件持续产生 | 无最终结果槽则BUSY且不应用；已接管请求最终结果不被挤掉；EVENT_GAP可观察；状态快照完整 | 待执行 |
| V-09 | 09～12 | telemetry真实C：256次生产、满后新采样、前台跳旧、head/tail回绕、元数据切换、CRC延迟计算 | 256条容量可用；满丢本次上传且计数；Flash指针/记录不变；10ms/最多10条封包、不跨缺口/版本/运行 | 待执行 |
| V-10 | 10～12、22 | 两次运行、会话重建、旧队列迟到、revision切换、无数据及事件/可靠拥堵 | run_id非零且同会话递增；记录索引含丢失缺口；上传携带原槽版本；无数据≤5ms调度尝试并等许可；结果/事件优先 | 待执行 |
| V-11 | 13～18、22 | runtime_tuning真实C：八项边界/类型/目标、fan_idle变化、一次pending、1000/1001等待边界、元素期间开关、断网/禁用/停车 | 整组拒绝或整组应用；RECEIVED不代表生效；下一完整允许拍APPLIED含真实values/revision/tick/控制拍；历史保持，last_success独立 | 待执行 |
| V-12 | 01、16、17 | 旧control_equivalence、dual_mode_tracking及新增逐拍对照；普通→十字/预环岛/右环岛→普通；两模式两档 | 无调试全部输出/部分覆写/历史等价；开启时切换拍暂停、待请求终结、恢复最后值；每拍每套FSM/history仅推进一次 | 待执行 |
| V-13 | 09、18～22 | 停车配置/自动保存/失败；无成功调参；有效/无效日志元信息；导出冲突起跑/清日志及错误路径 | 日志先收尾后保存；无成功调参无额外写；v3逐字节兼容；运行META零Flash访问；导出所有入口互斥；失败非SAVED且锁可释放 | 待执行 |
| V-14 | 02～04、23 | 逻辑分析仪CS/SCK/MOSI/MISO/READY，低/高速；断线/CRC错/DMA超时/ESP重启 | Mode0、实际SCK符合记录；CS内恰1024B、无旧许可复用；故障恢复可握手，控制周期与停车条件不新增 | 待实机 |
| V-15 | 10～12、19～21、23 | 有效标定环境60秒1kHz，与关闭上传对照；人为堵塞READY/UDP，统计Flash/上传/导出CRC | 约60000条对应实际采样区间，无新增Flash丢样/误停车；拥堵只增上传丢弃；导出总长与CRC一致，完整计数可解释 | 待实机 |
| V-16 | 01、16、17、23 | 实机测灰度扫描和完整TIM6，含普通、切换、应用请求和通信拥堵；记录最大值与工况 | 灰度≤700μs且不越原达到预算时超时边界，完整TIM6<900μs；现有达到900μs保护仍有效；无测量不得通过 | 待实机 |
| V-17 | 20、21 | spi_log_export真实C：0条/尾部/最大条数、1/960边界、越界/整数溢出、重复块/请求、乱序、提前END、Flash读错、10000ms无新读取、会话改变 | 原256B头+成功记录逐字节一致；总CRC独立一致；重复不推进CRC/续租，提前END拒绝；失败与超时解锁并喂狗；无整份RAM副本 | 待执行 |
| V-18 | 01、24 | CS逐项检查、中文联调说明、三端分阶段联调与所有证据复核 | CS-01～15均有结果/例外；先向量→握手→实时/导出→调参；AC5/实车未执行明确保留，阶段仅全部门禁完成才勾选 | 待执行 |

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
