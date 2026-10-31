# Phase 17 验证

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P17功能索引](../../../docs/traceability.md#p17)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 当前验收方法入口

阶段完成标准：本阶段适用需求的docs契约可定位实现且其验证判据有实际证据；仍未实施/待决定/待上机的项目继续保持该状态。本轮只检查文档迁移，不新增固件通过结论。原V编号、通过标准、结果、提交号、日志和未执行项完整保留在下方。

| 需求 | 当前方法/判据/实现 | 结果读取方式 |
|---|---|---|
| FR-01 | [F32 标准R08与维护](../../../docs/guides/validation.md#f32) | 历史实际结果见下方；限制/当前方法见docs |
| FR-02 | [F32 标准R08与维护](../../../docs/guides/validation.md#f32) | 历史实际结果见下方；限制/当前方法见docs |
| FR-03 | [F30 构建烧录与再生](../../../docs/guides/build-and-flash.md#f30) | 历史实际结果见下方；限制/当前方法见docs |
| FR-04 | [F01 初始化与三线调度](../../../docs/system/runtime-and-safety.md#f01)；[F02 起停、模式与首因](../../../docs/system/runtime-and-safety.md#f02)；[F29 时钟DMA采样与驱动](../../../docs/platform/board-and-bsp.md#f29) | 历史实际结果见下方；限制/当前方法见docs |
| FR-05 | [F05 电感偏差与对称度](../../../docs/sensing/sensors-and-calibration.md#f05)；[F06 灰度采集标定与质心](../../../docs/sensing/sensors-and-calibration.md#f06)；[F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07)；[F03 低压持续判定](../../../docs/system/runtime-and-safety.md#f03) | 历史实际结果见下方；限制/当前方法见docs |
| FR-06 | [F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)；[F09 活动电磁元素](../../../docs/control/elements-and-dormant-code.md#f09)；[F10 光电状态机](../../../docs/control/tracking-and-controllers.md#f10)；[F11 双FSM与唯一执行](../../../docs/control/tracking-and-controllers.md#f11)；[F12 部分配置与全局调参规划](../../../docs/control/tracking-and-controllers.md#f12) | 历史实际结果见下方；限制/当前方法见docs |
| FR-07 | [F18 完整拍临时调参](../../../docs/services/parameters-and-tuning.md#f18)；[F15 多入口参数应用](../../../docs/services/parameters-and-tuning.md#f15)；[F16 参数v3格式](../../../docs/services/parameters-and-tuning.md#f16)；[F17 迁移保存与失败](../../../docs/services/parameters-and-tuning.md#f17) | 历史实际结果见下方；限制/当前方法见docs |
| FR-08 | [F20 队列前台写入与故障](../../../docs/services/recording-and-export.md#f20)；[F19 日志格式容量](../../../docs/services/recording-and-export.md#f19)；[F21 CSV与会话边界](../../../docs/services/recording-and-export.md#f21)；[F22 二进制流式导出](../../../docs/services/recording-and-export.md#f22-export)；[F25 独立上传](../../../docs/services/communication.md#f25) | 历史实际结果见下方；限制/当前方法见docs |
| FR-09 | [F23 SCB帧JSON与命令](../../../docs/services/communication.md#f23)；[F14 菜单按键与显示](../../../docs/hmi/operation.md#f14)；[F24 会话可靠传输与回绕](../../../docs/services/communication.md#f24) | 历史实际结果见下方；限制/当前方法见docs |
| FR-10 | [F13 legacy与未接入元素](../../../docs/control/elements-and-dormant-code.md#f13) | 历史实际结果见下方；限制/当前方法见docs |
| FR-11 | [F11 双FSM与唯一执行](../../../docs/control/tracking-and-controllers.md#f11)；[F01 初始化与三线调度](../../../docs/system/runtime-and-safety.md#f01)；[F20 队列前台写入与故障](../../../docs/services/recording-and-export.md#f20) | 历史实际结果见下方；限制/当前方法见docs |
| FR-12 | [F11 双FSM与唯一执行](../../../docs/control/tracking-and-controllers.md#f11)；[F01 初始化与三线调度](../../../docs/system/runtime-and-safety.md#f01)；[F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)；[F19 日志格式容量](../../../docs/services/recording-and-export.md#f19)；[F23 SCB帧JSON与命令](../../../docs/services/communication.md#f23) | 历史实际结果见下方；限制/当前方法见docs |
| FR-13 | [F32 标准R08与维护](../../../docs/guides/validation.md#f32) | 历史实际结果见下方；限制/当前方法见docs |

**当前复现限制**：完整dual入口仍缺原始LQ_Track.c；独立子项的旧结果不代表完整入口通过。AC5/实测栈/700与900us/波形/吞吐/实车保持待验。














## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 17 验证

状态：G0～G6软件实现与可执行验收均完成，最终暂存区R08已通过；AC5、R07原始样本及实机项目继续待验。下表是验收标准，实际结果见后文。

| 项 | 需求 | 验证方法与唯一通过标准 |
|---|---|---|
| V-01 | FR-01/02/10 | 全量文件/函数/对象清单可定位；G1映射前后除include/保护宏外token完全一致 |
| V-02 | FR-03/12 | LCD=0和1各执行MDK-GCC/build.sh，均0警告0错误；Keil源清单存在、无重复、覆盖全部App C |
| V-03 | FR-05/06/12 | control_equivalence与dual_mode_tracking原断言不删减；逐拍状态输出一致 |
| V-04 | FR-07/12 | runtime_tuning、参数版本迁移原断言通过 |
| V-05 | FR-08/09/12 | datalog_v2、scb_protocol、scb_link、spi_wifi_state、spi_log_export通过 |
| V-06 | FR-04/11 | App无HAL/CubeMX头/寄存器访问；状态写入有唯一归属；无新增反向依赖 |
| V-07 | FR-13 | CS-01～15和R08按组检查；源码/依赖摘要一致；历史记录保留 |
| V-08 | FR-11/12 | GCC RAM/栈与基线比较；AC5构建及硬件700/900us/波形/吞吐/实车另验 |

R07外部文件缺失导致的完整双模式阻塞保持记录；不能删除断言或使用当前实现自证替代历史算法。
未执行项目写“未执行”，不能用GCC代替AC5、宿主代替实机。

## G1 实际结果（2026-10-23）

- V-01：app_layout_check.py --base 4745aa2339209d5df9a739449685fc7f1cb66429 通过；44个C/H包含前缀规范化后字节保持，21个App C在Keil各一次，五类分组齐备。
- V-02：LCD=0/1 GCC均零警告零错误；0配置text/data/bss=90556/296/231744，1配置=93220/300/231760；配置恢复0。
- V-03～05：control_equivalence、datalog_v2、runtime_tuning、scb_protocol（608项）、scb_link、spi_wifi_state、spi_log_export通过。dual_mode_tracking完整入口因原C:/Users/Li/Desktop/LQ_Track.c缺失阻塞；gray已运行，fsm/param/app独立子项通过。
- V-07：CS-01～15按G1适用范围检查：只移动完整文件和include，保留名称、函数体、注释、布局和预处理；未借本批修复继承内容缺陷。R08证据按新旧路径分别登记；旧记录保留。
- Phase16静态契约通过；AC5和实机未执行。G2～G6尚未实施，本阶段未完成。
- 日志：MDK-GCC/build/phase17-g1-*，不是自动纳入提交的产物。

## G2 实际结果（2026-10-23）

- G2-V1：最终LCD=0/1均零警告零错误；text/data/bss分别90764/296/231736和93428/300/231752，较G1静态BSS减少8B、text增加208B；配置与受跟踪ELF原字节恢复。
- G2-V2/3/6：control_equivalence、datalog_v2、runtime_tuning、scb_protocol、scb_link、spi_wifi_state、spi_log_export通过。独立fsm/param/app子项全部通过；App测试保持顶部检查后中断注入、全部停车与再次起跑断言。完整dual入口仍被R07原始文件缺失阻塞。
- G2-V4/5：g2_system真实BSP包装、ADC窄化/滤波、低压严格边界/201次、短路读次数和NONE无副作用测试通过。
- G2-V7：53个C/H结构、25个Keil源各一次检查通过；App无可执行HAL/CubeMX/main依赖及g_app残留；Phase16静态契约通过。人工逐项复核见g2-review.md，暂存区R08记录绑定本组源码与依赖。
- 日志：MDK-GCC/build/phase17-g2-final-*。AC5、栈峰值与700/900us/波形/吞吐/实车未执行，不用text/BSS结果替代。
- G0/G1/G2已完成软件可执行部分，G3～G6未实施；全阶段继续未完成。

## G3 实际结果（2026-10-23）

- 逐模块/注释/状态复核见g3-review.md；控制器与profile/legacy隔离已实现，完整拍与监视字段没有合并。
- LCD=0/1 GCC均零警告零错误；text/data/bss=90740/308/231736和93388/312/231752；配置及受跟踪ELF恢复。
- 67个C/H、32个App源的布局/Keil唯一编译检查通过。
- control_equivalence、runtime_tuning、g2_system、datalog_v2、scb_protocol(608项)、scb_link、spi_wifi_state、spi_log_export及独立FSM/参数/App子项通过。
- g3_control新增真实G2/当前对照通过：角度执行Motor调用/参数/当时状态，增量左右历史跨reset，IMU校准/更新/reset及纯角度无Motor动作。
- 日志phase17-g3-*；R07完整入口原始文件、AC5、栈峰值和实机预算继续待验。G4～G6仍未完成。

## G4 实际结果（2026-10-24）

- G4-V01/02：bank、镜像及system参数应用已分离；原v1/v2/v3布局/CRC/四bank/失败门禁通过。
  g4_hmi使用6125495和当前真实C逐条比较LCD调用与保存镜像；vp/vi活动PI与镜像在
  2999/3000ms边界的差异保持。人工内容/逐函数清单见g4-review.md。
- G4-V03/04/05：runtime_tuning、control_equivalence、scb_protocol608、scb_link、
  datalog_v2、spi_wifi_state、spi_log_export、g2_system、g3_control原入口全部通过。
  独立codec仅增编真实tuning_values.c；原断言未删改放宽。
  g4_transactions新增final占用、事件时点、独立成功值、会话/保护取消及两种revision回绕通过。
- G4-V06：原参数/key子项通过；新增g4_hmi的严格阈值/第二路覆盖、BACK旧位置保存、
  RST当拍早退/下拍清屏、松键保存、两模式两档及风扇钳位历史调用/字节对照通过。
- G4-V07：双LCD GCC均0警告0错误；text/data/bss为91656/308/231688、94220/312/231704。
  app_layout_check确认82个C/H、39个App C在Keil各一次；新拆分103个C定义的标签/EOF/行宽
  无缺项，人工内容、getter/转发类别和例外逐项见g4-review。R08暂存区绑定/提交前检查由提交方执行。
  对G3本批ARM编译帧对比72/118个函数，最大均232B；具体变化见review，完整调用链预算留G6。
- G4-V08/09：g4_boundaries将起停事件全部语句逐token对历史比较一致，Link仅单调用转发，
  mode协调门禁/写入/reset顺序与唯一setter通过；原跨mode、运行BUSY、保存失败及新增非法/
  同mode不早退真实入口断言全部通过。system自身/新header无直接SCB；datalog.h传递闭包留G5/G6。
- 独立FSM/参数/App子项全部通过；完整dual入口仍因C:/Users/Li/Desktop/LQ_Track.c缺失阻塞，
  R07原SHA256断言保留。AC5、700/900us、波形、Flash吞吐及实车继续待验。
- 日志phase17-g4-*。最后构建finally回写ELF遭Windows OSError22后，已用HEAD二进制blob
  恢复并逐字节验证；app_config与受跟踪ELF均恢复且Git状态干净。忽略产物不纳入提交。
- G4-09闭包：2ee3401先行规定保留休眠包装原版本发布副作用；两次自动审批拒绝后停止，
  用户于2026-10-24明确回答“允许，保持原有副作用”，正常受审补丁随后落盘，未启用入口。
  真实g4_legacy_boundary补丁前`TGPORE != TGPOSRE`失败证据保留；补丁后成功/故障精确
  同步顺序及保护优先级通过。runtime_tuning/control_equivalence/g3_control、事务/边界和
  独立FSM/参数/App均复验通过，日志phase17-g4-post-approval-*。
  最终同版双LCD0/1均零警告，体积与前述G4相同；finally逐字节恢复原配置及ELF/HEX/BIN。
  ARM补验原/当前休眠包装栈帧分别16B/16B，角度包装8B/8B；无新增产品活动调用。

- G4暂存区R08实际通过：以2ee34017e5bc42d7f1bb824e7704c176a74456ab为基线，35个变更C/H、144份当前/历史复核记录及依赖摘要一致；人工内容见g4-review，不以门禁代替语义复核。

## G5 实际结果（2026-10-24）

- G5-V01：run_record/logger/log_store/log_export分离，完整快照/头/队列/会话/导出各唯一owner。
  仅log_store调用日志BSP Flash；独立datalog_result_t无协议数值绑定，日志核心无SCB依赖。
  dispatch显式映射六结果，raw缓存+8在同一前台调用内经真实SCB_EncodeLogData复制到原s_json。
- G5-V02/03：原datalog_v2、spi_log_export全部断言通过；新增g5_boundaries通过真实写头
  W/WR/比较短路、raw失败data保持/len先0、最近块不续租/不推进CRC、部分读取失败取消及旧首
  fault保留。codec960字节极值恰写968B且上下guard不变，原wire/CRC/最大前缀/真实dispatch
  结果缓存路径继续通过。真实G4与当前的256B头、12条记录和完整CSV全部逐字节相同，含写时ISR。
- G5-V04：协议608项保持；27个codec原函数体及共享CRC32公式对7eefb95逐token一致。
  独立协议仅链接实际frame/json/codec/checksum和纯tuning_values，不用事务stub或忽略未定义符号。
- G5-V05：scb_link、spi_wifi_state、control_equivalence、runtime_tuning、g2_system、g3_control、
  g4_boundaries/transactions/hmi/legacy_boundary全部通过，17个link原体逐token一致。
  独立FSM/参数/key/App子项通过；完整dual入口仍缺原LQ_Track.c，原SHA256断言保留且不伪报通过。
- G5-V06：LCD0/1均0警告0错误，text/data/bss为91432/308/231688、93996/312/231704；
  相对G4 text各减224B，data/bss不变。95个C/H、45个App C在Keil各一次；所有固定缓冲符号
  仅一份。108个C定义和20个现存C/H已分别复核覆盖、格式、内容、行为，详见g5-review。
  ARM局部编译帧65→80项最大均200B；Push120→112+Capture16，Start32→24+Build16/WriteHeader8，
  ParseCommand72→80B。单帧不等于峰值，最终全链路/中断预算留G6。
- 构建finally逐字节恢复原app_config/ELF/HEX/BIN，before/restored四SHA256均一致；历史结果文件
  未覆盖。日志phase17-g5-*。R08最终暂存区绑定由提交方执行；AC5、实机吞吐、700/900us与实车未验。

- G5最终暂存区R08实际通过：基线7eefb952d96c2bb1e6368afd7029c003abb8455b，23个变更C/H、160份当前/历史复核记录，源码/真实依赖摘要一致。

## G6 最终软件验收（2026-10-24）

- 目录/工程：恰好五个平铺业务目录；App根仅四文件；45个C、50个H、365个App函数定义。
  GCC五类扫描、uvprojx/uvoptx App路径/分组/编号及每C恰编译一次均通过；非App/Target/debug选项除匹配编号外保持。
- 依赖/状态：全部实际App include闭包通过；无直接HAL/CubeMX/寄存器/堆调用，BSP为明确边界。
  参数存储无使用者依赖、算法不反读传感器/电机、system及日志核心无SCB类型传递依赖；所有文件级可写对象私有。
  regex/AST是结构证据，状态时序/指针借用/并发由各组人工审查与真实C门禁验证，不作形式证明。
- 迁移：原44个C/H、1395项（261函数）全部绑定至真实目标；最终App及6个BSP适配C/H共1772项。
  final-symbol-map.json含完整签名/条件/调用方/owner及验证导航；app_migration_check已通过，缺失0。
- 兼容与行为：最终15个可执行入口通过（包括布局），完整dual入口仍退出1且仅因原LQ_Track.c缺失。
  独立gray/FSM/参数/key/App、原协议608项、日志/导出/链路/SPI、控制与完整拍调参、G2～G5新增边界均通过。
  真实G4/当前的日志头/12条记录/完整CSV字节相同；G5原codec/CRC/link体一致；G6二十文件非注释token与fe274b3一致。
- 工具：原R08工具22项测试通过，未放宽判定；资源helper显式旧/新ELF及真实dispatch回调测试通过。
  未运行旧Phase16主入口，未覆盖phase16.json/g5旧日志/stack-final历史产物。
- 文档：README、techstack、code-style、roadmap、CHANGELOG、Phase01～08及12～16当前实现导航、
  Phase15符号/注释清单和休眠清单均同步；旧路径/提交/验证结果保留，G1历史表未改写为最终结构。
- 详细人工内容、CS项及例外见g6-review.md；实际宿主摘要与源码清单见tests/results/phase17-validation.json。
  资源原始ELF/map/日志在MDK-GCC/build/phase17-resources，携带日志/ELF/map及源文件SHA256。

### 独立资源对照

原提交4745aa2339209d5df9a739449685fc7f1cb66429通过git archive独立构建，与冻结最终源码使用同一ARM GCC及O2 hard-float。
以下采用本次新鲜基线，不用旧日志中因链接顺序/对齐不同的data数值混算。

| 配置 | 原text/data/BSS | 最终text/data/BSS | 差额text/data/BSS |
|---|---|---|---|
| LCD0 | 90556 / 300 / 231736 | 91432 / 308 / 231688 | +876 / +8 / -48 B |
| LCD1 | 93220 / 308 / 231752 | 93996 / 312 / 231704 | +776 / +4 / -48 B |

四次独立构建均0警告0错误。before/after分别编译74/99个实际C的1089/1199条frame，均static、无调用环。
完整规划从4328B变为4176B：最终source按优先级组合1568B + 五层硬件上下文560B + 库/未解析预留2048B，低于8192B预留。
最终main/TIM15/TIM6/TIM7源路径分别840/264/296/296B，不把单函数帧或该预算称为实测峰值/形式上界。
HAL_SPI_IRQHandler、_printf_common、_printf_i间接目标仍未完全解析，前后均列出；库/未解析预留不能代替实际高水位。
SysTick保留既有最大分组比较口径，低优先级嵌套额外影响列在2048B预留限制内。

全部24个≥256B可写对象按去编译器局部后缀后的名称、数量、大小一致，包含两份pdata各640B，未被字典覆盖。
关键snapshot96/header256、Flash队列98304/batch768、导出968/check256/CSV640、上传27648、请求64000、
事件1920、可靠帧1024及各JSON工作区保持唯一原份数；ADC与SPI DMA固定地址检查通过。
资源报告tests/results/phase17-resources.json保存全部nm地址/大小、多重名称、回调补边、frame和限制。
执行时current_head是7eefb95且G5/G6工作树尚未全部提交，不能当作干净7eefb95构建；after.source_sha256覆盖全部实际156个产品C/H，
完成后逐字节复核这些源码未变。LCD宏按两配置切换，原app_config与ELF/HEX/BIN/map已finally恢复并核对。

### 需求—实现—验证闭环

| 需求 | 最终实现定位 | 实际证据与限制 |
|---|---|---|
| FR-01 | App五类、根app/app_config/app_types | layout及architecture通过95文件，禁止嵌套/多余目录 |
| FR-02 | G1 path-map及最终migration-map/JSON | G1纯迁移44文件等价、1395原项完整；历史基线路径保持 |
| FR-03 | GCC build.sh、uvprojx/uvoptx、app_fixture | 45源各一次、两LCD及Keil编号/选项保持 |
| FR-04 | system/scheduler/lifecycle/safety、BSP runtime/time/pwm | G2及真实App ISR注入/起停/二次收尾、无App直接HAL |
| FR-05 | sensing六C及标定头 | 原传感器/灰度/IMU数值、低压边界，标定仍32个-1 |
| FR-06 | tracking/profile、两FSM、三控制器、pid/motor | control_equivalence、G3真实历史及双模式独立FSM；完整R07样本仍缺 |
| FR-07 | parameter_bank/store/image、两application、tuning/values | 参数v1/v2/v3、G4 HMI镜像字节与完整拍事务/回绕/保存失败 |
| FR-08 | run_record/logger/log_store/log_export | 日志v2、队列/失败、原CSV/CRC与真实历史字节、独立上传 |
| FR-09 | frame/json/codec/link/dispatch、menu/keys/render/dashboard | 608协议、链路/SPI、G4逐绘制调用及G5边界 |
| FR-10 | 全legacy模块及休眠现行位置表 | G3算法/历史、G4旧包装负/正对照；用户具体批准保留同步，无新增活动入口 |
| FR-11 | 私有owner、原临界区/借用契约、static缓冲 | architecture对象清单、G2～G5真实并发边界、24大对象及资源 |
| FR-12 | 原周期/优先级/阈值/格式及安全/完整拍 | 可执行真实C回归通过，R07/AC5/700与900us/波形/吞吐/实车未验，不伪报 |
| FR-13 | 本目录及README/跨Phase/标准/休眠/CHANGELOG | 全量当前导航、历史记录保留，最终R08收口见下方 |

### 保留的未执行项

完整R07仍缺原C:/Users/Li/Desktop/LQ_Track.c，不删除或替代其SHA256/历史算法断言。
AC5、灰度700us与控制900us实测、SPI波形、持续吞吐、真实栈高水位和实车回归均未执行；旧Phase08～16待验状态保持。
G6最终暂存区R08实际通过：基线fe274b399080c22bd32923543726d362154d94ae，20个变更C/H、160份当前/历史复核记录，源码/真实依赖摘要一致。G0～G6软件可执行部分完成；未执行项仍不标完成。

</details>
