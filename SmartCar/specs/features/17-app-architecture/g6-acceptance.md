# G6 全量架构、迁移追溯及资源验收契约 — 阶段入口

本页保留Phase17当时的设计/验收契约，不再独立维护现行功能和代码映射。

## 当前docs入口

- [guides/validation.md](../../../docs/guides/validation.md)
- [architecture.md](../../../docs/architecture.md)

旧迁移清单/JSON/评审及真实结果仍按原基线解释，不用历史章节替代现行docs或硬件验证。

## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# G6 全量架构、迁移追溯及资源验收契约

日期：2026-10-23。对应 FR-01～13、V-01～08。落实用户已批准的最终验收；不改变算法、协议、
持久化、调度、保护或休眠接入。此文件必须纯文档独立提交后才实施以下工具/工程调整。

## G6-01 目录、依赖及状态检查

新增 tests/app_architecture_check.py；修订 app_layout_check.py 的最终清单规则，保留
--g1-equivalence 历史用途。无参数检查最终架构而非强制保留已明确拆空的旧模块。

- App 根文件限 app.c/app.h/app_config.h/app_types.h，直接子目录集合必须恰好为
  system/sensing/control/services/hmi；五类下面不允许任何目录，空目录也拒绝。
- App 全部 C/H 的带引号 include 必须可唯一解析；跨模块使用 App 根可辨识路径。扫描直接及
  传递的 App include，记录完整边；App 不包含 main.h、CubeMX 外设头、stm32 HAL/CMSIS头，
  不使用 HAL_、LL_、寄存器地址或 CubeMX 句柄。标准 C 头和公开 BSP 头为边界，BSP 内部
  的 HAL 依赖不计为 App 传递违规。BSP 固定回调向 App 入口总装的依赖登记为明确适配关系。
- sensing 不依赖 HMI、通信、记录或系统；filter 只接受状态与输入。
  direction/angle/speed_controller、pid、legacy_pid、tuning_values 的传递 App 依赖不能到
  sensing/motor/system/services/hmi。legacy_control 保留原算法到电机适配，不套纯算法限制。
- parameter_bank/store/image 不依赖 control/HMI/system/协议；tuning 业务不依赖 SCB、Flash、
  日志或界面；system 不直接或传递包含协议头。run_record/logger/log_store/log_export 的
  传递依赖不允许 SCB 类型或编码函数；telemetry 属通信适配，其协议依赖为显式允许。
- 产品 App 不新增可写 extern 或文件级非 static 可写定义；既有 owner 的 const 只读地址接口
  逐项登记，内部 legacy IMU 指针只由所属 imu_proc 传入，不复制历史。宏展开/函数名扫描
  只作定位证据，不宣称仅凭正则证明并发一致性；完整拍、事务和队列边界由真实 C 回归验收。
- GCC 源扫描及 uvprojx 每 C 一次且覆盖全 App；uvoptx 的 App 组按最终 uvprojx 顺序同步
  名称/路径/GroupNumber/FileNumber，保留 Target/debug/非App选项。新增源无旧 File 选项时
  使用原普通 C 文件节点的相同显示默认值，不修改调试器、Flash、工具链或编译开关。

违反以上规则时检查失败并列出具体路径/边/对象；不得以排除当前违规模块绕过。

## G6-02 最终迁移清单及文档

保留 migration-map.md 的 G1 历史表；追加完整最终导航并交付 final-symbol-map.json。
范围为原基线 4745aa2339209d5df9a739449685fc7f1cb66429 的所有 App C/H，及最终产品 App C/H
和新增 BSP 适配。清单以 path/symbol/kind/line/conditional 标识，覆盖公共/私有函数、函数
声明、文件级与静态局部对象、typedef/成员/枚举/宏、各预处理分支及头保护宏。每个旧项指向
一个或多个真实最终项；拆入 owner 字段的旧全局明确字段位置与读写上下文；仅删除旧聚合头
和移除重复声明也须登记其实际声明 owner，不写“随文件迁移”替代定位。

当前函数额外记录完整签名、调用方、状态 owner/写入上下文及对应验证入口；新增包装/适配
说明来自哪条拆分契约。自动提取只生成索引，匹配不唯一必须人工绑定；未匹配项不能填入
虚构路径。清单检查验证目标符号/路径存在、原项覆盖及当前函数/对象覆盖。保留历史 git show
路径和原始验证位置，不对仓库全文替换。

README 给出最终五类完整模块、阅读顺序、标定位置和双工程构建说明；architecture 给出实际
模块、状态所有权、起停/逐拍调参/记录/导出时序及前台/ISR模型。techstack/code-style追加
新前缀和边界；roadmap/CHANGELOG/Phase01～07、12～16追加现行位置/接口，Phase15迁移与
注释清单指向新清单并绑定最终源码依赖；休眠清单追加最终入口与实际调用状态，原研究不改写。

G6 可修正本次拆分新增/迁移 C/H 的注释定位、模板句和失实说明，逐项按真实实现说明输入、
状态、顺序、失败保留及并发。不修改可执行 token、签名或数据布局；若发现行为差异停止该
点并按所属组先补规约，不借注释验收修既有行为。R08证据按暂存源码和实际依赖重新复核。

## G6-03 独立资源证据与回归

新增 tests/phase17_resources.py，输出到新的 MDK-GCC/build/phase17-resources/ 及
 tests/results/phase17-resources.json，原 phase16.json/g5-*/stack-final 不覆盖。
修订 phase16_check.py 的资源 helper：可显式传 ELF 路径；SCB 注册回调补边改为实际
CommandDispatch_HandleCommand/CommandDispatch_ResetSession（旧基线兼容仅在旧符号实际
存在时补旧 Tuning 目标），其他 HAL DMA/SPI/UART 回调补边保持并人工核对所有 blx 位置。
默认历史记录不改写；当前硬编码源清单按 G4/G5真实 owner 同步。

同一 ARM GCC、-O2 hard-float 编译原基线与最终源码的两种 LCD，比较 text/data/BSS。原基线
通过 git archive 提取至本次独立输出目录，构建脚本与源保持原提交，只有临时副本 LCD 宏
切换；不借原未跟踪 .su/.elf 证明旧资源。当前构建前备份 app_config.h、受跟踪 ELF 及原map
字节，成功/失败 finally 全部恢复；归档两配置本次 ELF/map 摘要与源码/工具链摘要。

最终配置 LCD1 各实际 C 编译生成 .su，结合本次 ELF 反汇编建立调用图；原基线同样处理。
按前台加0/1/2/3/4各优先级最大路径、五层112B硬件上下文及2048B库/未解析预留计算，
与8192B栈预留比较。不把 source frame 之和称为实测峰值或形式化上界。所有动态frame、
环、间接调用位置和未解析目标列明；不能静默忽略。nm相同私有名称保存为多条地址/大小记录，
不得用字典覆盖同名对象。主要日志/上传/可靠帧/JSON/导出缓冲逐项核对数量与大小，禁止复制
大缓冲；拆分小状态和调用开销的差额记录在 validation。

最终综合回归保留原 control_equivalence/datalog_v2/runtime_tuning/scb_protocol/scb_link/
spi_wifi_state/spi_log_export 断言，运行 g2_system/g3_control 和 G4/G5新增边界验证、布局与
架构检查。dual_mode_tracking完整入口仍运行并记录 R07 缺失；独立 gray/FSM/param/App
子项可执行，不能修改原缺失条件或用当前公式替代历史输出。工具检查若修改R08门禁本身须
执行原22项工具测试，不允许降低原判定。所有失败完整记录，修复后只复验有影响的部分。

## G6-04 完成判定和回退

软件完成：全部目录/状态/依赖/迁移项无未定位项，双LCD零警告零错误，真实C回归通过，
R08暂存区通过，资源差额有证据且规划预算不超过8192B，文档与最终源码一致。
完整R07、AC5、实测栈峰值、SPI波形、持续吞吐、灰度700us/控制900us及实车继续列为待验，
不据软件完成勾掉历史硬件门禁。保留既有Hardware/其他未跟踪产物，不push。
回退本组工具/附属工程/文档及仅注释变更为完整组，不执行参数或日志格式迁移。

</details>
