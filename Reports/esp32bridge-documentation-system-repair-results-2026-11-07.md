# ESP32Bridge文档体系纠偏实际交付

日期2026-11-07；调查/保护基线441a77843ba765cf58dbcdda4d0d55ad2e4f5557，开始HEAD与基线相同，无后续有效增量需要另合并。独立分支codex/constitution-esp32-docs-alignment。依据[执行方案](esp32bridge-documentation-system-repair-plan-2026-11-07.md)、[检查报告](esp32bridge-documentation-system-audit-2026-11-07.md)。本次只文档，不构建/宿主回归/烧录/复位/SDK安装切换/推送/合并。

## 实施与分组提交

| 组 | 实际交付 | 提交 |
|---|---|---|
| G0 | HEAD/分支/状态核对；23包装、18三件套及11附属资料清单；保护范围和旧入口迁移关系 | 本记录与[恢复清单](esp32bridge-phase-restoration-inventory-2026-11-07.md) |
| G1 | 前层治理、完整Phase职责、同步顺序、关闭规则及历史时态 | a82ed06 |
| G2 | 23正文恢复、6validation核对、Phase05未来目标归位 | 3b84fc4 |
| G3 | 固定池/DMA、JSON/检查矩阵、诊断完整字段样板；主执行者读样板并沿生产代码复核后展开 | a80915a |
| G4 | 可靠/会话、SPI/网络及F06～08当前缺口；旧入口导航；两次跨领域只读复核 | 048495e |
| G5 | 参数、架构、追溯、现行模板、问题/历史入口及格式收尾 | 2cbe9b4 |
| G6 | 本矩阵、十二场景、来源/范围检查及交付回填 | 本记录所属“记录体系验收与保护范围”提交 |

## 恢复及保持

[逐文件恢复清单](esp32bridge-phase-restoration-inventory-2026-11-07.md)给出23份原正文行数及SHA256。6 requirements、6 plan、11附属设计撤除错误整篇冻结围栏；原正文按非空行对比零缺行，仅修复Docs大小写和两处构建指南路径。保留原FR/TC显式锚点、R6、原用户决定、8段11月5日整改增量。6validation按内容注明准备/实际结果时点，未把整份当冻结解包。修复本次编辑产生的重复CR及多余末尾空行，历史结果文字和数据不被改写。

Phase05的信用、reserve/commit/cancel、槽位、锁序、租借、事件锚点、导出/墓碑及计时完整保存在阶段设计，Docs仅说明当前端口与缺口。原“已确认”声明与待确认记录并列，不替用户选择。3/6/1槽、10240B和10秒均为未来目标，当前池51072B独立。

保护核对：全部90份ESP32Bridge受保护tracked文件（非Markdown、两来源附件、协议快照）以git cat-file --filters同检出规则与基线逐字节比较，差异0；未读密码文件。全仓Git差异只含授权文档及开始已存在的SmartCar/App/system/board_test_config.h；后者未编辑、未暂存。未跟踪Reports/sensor-isolated-board-test-plan-2026-11-06.md未编辑、未暂存。未新增永久测试脚手架。

附件当前SHA256：

- 任务书：3559fd075b6bb4675976f5ba159f3b2e7db7177c9eb293417c23bc93f8071269。
- SDD附件：c71b025496c56b65fe6df11ed420d865c6c4fdaaa4021d03aead23dea036a399。
- 协议提取：784cf6dc08712dfd9bf3ead5a7bbc41533090a2946d183b7a68b549bf7350d10。

原任务书旧哈希声明差异EB-06保留，未修改附件来消除差异。

## V01～V11实际验收

| 编号 | 实际核对、结果和限制 |
|---|---|
| V01 通过 | 前层五文件、AGENTS、维护模板逐项核对完整Phase、目标位置、Docs先行/同批交付、关闭规则；保留v6.0.0、桥接职责及ESP32审查政策。 |
| V02 通过 | 29份阶段Markdown逐份核对，23原文恢复零缺行、8整改增量保留；没有借SmartCar提交回退。 |
| V03 通过 | F06～08只保留生产端口/缺口/来源；bridge_state/bridge_stats不存在，Phase06未交付，不以静态池或UDP出口宣称完整桥接。 |
| V04 通过（静态语义） | 逐主题对输入、算法、状态、调用顺序、参数和失败分支核对生产C/H；JSON/矩阵、固定池/DMA及诊断样板先主审，协议与运行时负责人互换只读复核，主执行者交叉阅读并修正文案。不是行为运行证明。 |
| V05 通过（静态归属） | architecture列全部main生产C/H与工程/脚本职责；codec/JSON/可靠/会话、SPI适配和NET路径均有主说明；不存在桥接模块不造链接。 |
| V06 通过（文档参数） | 按方案6.3全部参数组核对宏、字段及调用条件；参数索引到实际明细，容量/实测及未来额度分开。SDK栈水位字节单位补查官方v6.0文档，不推断本机安装或实测水位。 |
| V07 通过 | 187静态整改宿主、222历史网络、258原R6声明分层保留；S5目标构建未验证、旧镜像AP/DHCP与修复后待复测区分，EB01～07未擅自关闭。 |
| V08 通过 | 90受保护文件同检出规则逐字节差异0；原证据JSON/向量/附件/协议快照未改。 |
| V09 通过 | F01～F10身份稳定、五领域及证据层保留，父页到子主题、参数/模块/FR/TC往返可达；旧显式锚点及旧标题导航保留。 |
| V10 通过 | 一次性只读遍历77份活动Markdown本地路径、锚点及大小写；排除原附件/协议来源和围栏示例，阶段恢复正文纳入；修复格式后全范围git diff --check无错误。 |
| V11 通过 | 基线到最终提交清单仅授权Markdown；源码、配置、tests/scripts、硬件、原证据均未改；已知其他在途内容留在工作区。 |

## 十二个读者场景实际走查

| 场景 | 路径、核对结论 |
|---|---|
| 1 | mission/techstack→P02/P05→F03→reliable_link/link_session：ACK是接管而非车辆APPLIED/SAVED，业务及持久化由STM32决定。 |
| 2 | P01→startup-and-resources#f01-startup→startup/startup_idf：READY仅阻SPI，密码阻AP/socket，池阻各服务；网络链失败仍独立尝试SPI。 |
| 3 | resource-pools#f01-pools/f01-dma→buffer_pool：四次分配检查与回滚、类/索引/精确地址容量/代际、跨deinit代际保留、忙时不能销毁；池不代替SDK回收证据。 |
| 4 | json-validation→json_guard：全文消费、UTF-8/转义标量、同对象解码后重复键、非递归497项/496层、binary64阈值及十进制精确u32逐分支可解释。 |
| 5 | frame-codec#f02-validation→protocol：SPI忽略填充、TCP/UDP精确长、CRC覆盖、1～10记录/24+96N/保留零、LOG_DATA1～960及失败码；身份许可不冒充codec检查。 |
| 6 | reliable-and-sessions→reliable_link：短写不sent，首次完整发送起200ms，3次重发后再等待，精确ACK；预留后接管复制，重复身份/同seq改内容区分。补明非可靠STALE仍可处理精确ACK。 |
| 7 | spi#f04-transaction/ready/ownership→run_transactions：submit/queue→READY回调→get原描述→reclaim→sent/receive；无时钟超时不释放DMA，错描述不能证明回收。 |
| 8 | parameters→network#f05-timing：backlog2/控制1/AP关联4/poll10ms/BUSY500ms/停滞严格>2000ms各自控制不同资源与时间，不混作单超时。 |
| 9 | network#f05-subscribe/udp→handle_subscribe/offer/udp_service→F07：本地端口精确u32、对端IP/连接代际、完整sendto；SPI供给链缺失，不宣称完整实时桥接。 |
| 10 | diagnostics#f09-net→dispatch/tx/busy/close：received可能含背压，sent_bytes含BUSY不含UDP，peer保留；断线直接清pending不保证逐包计丢，overflow也可折叠。 |
| 11 | F06～08→Phase05生命周期/R6→known-issues：3/6/1槽目标、R-05批准冲突、导出计时两声明与当前模块缺失分开；不启动实现。 |
| 12 | 旧framing-and-links F02/F03及startup-and-resources F01→具体新子主题；F10/roadmap/历史报告解释187/222/258不同采集与未验边界，未把文档核对当重跑。 |

十二场景是跨层走查，V04/V05另覆盖全部现行模块归属与关键条件路径，没有用链接数替代语义审查。语义走查中发现并修复两项旧总览误导：非可靠STALE的反向ACK及断线pending的非计数清理。

## 仍未决与待验

EB-01 R-05批准冲突、EB-05导出计时歧义、EB-04桥接模块/部分历史产物缺失、EB-06附件哈希差异、EB-07跨任务代际同步风险全部保留。候选向量三端一致、SPI电气/READY/吞吐、修复后网络实机复测、完整桥接/导出及Phase06交付仍待对应任务验证。S5目标构建未验证；本次不新增虚假通过、不关闭阶段。

单位补查依据：[ESP-IDF v6.0 ESP32-S3内存指南](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-guides/performance/ram-usage.html#run-time-methods-to-determine-stack-size)。仅确认API单位，不替代当前目标构建或实际内存验收。
