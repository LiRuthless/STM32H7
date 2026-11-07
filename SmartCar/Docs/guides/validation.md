# 验证证据与维护

现行功能说明，核对代码基线 `8d5ab2f`；2026-11-07复核，保留2026-10-31迁移来源。需求经[功能索引](../traceability.md)进入本页，再由实现表定位源码。
当前行为不等于全部已批准/实机验收，冲突见[已知问题](../known-issues.md)。

<a id="f31"></a>
## F31 验证证据与复现
### 证据分类与当前状态

源码审查回答当前流程，结构检查回答依赖/登记，真实C宿主回答受控输入下行为，ARM/GCC/AC5构建回答对应工具链，板上测试回答电气/时序/实车。五类不能互相代替。
P17已有15入口pass、完整dual受R07缺桌面LQ_Track.c阻塞；101文件LF归一摘要及156资源源码摘要在本次迁移前与实际代码一致。本轮只核对文档，不宣称重跑这些测试或更新历史JSON。原始样本要求SHA256=6AB7B2B68AFE7C00C6CF2096E56953D398C1E73C028D792431B90786F792DCEF，缺失不能用当前公式或同名文件替代。

### 方法选择

已有control_equivalence、dual_mode_tracking、datalog_v2、runtime_tuning、scb_protocol/link、spi_wifi_state/log_export及G2～G5边界可复用，见本页工具表。软件替身不是硬件模块测试。保留真实硬件能力和所有AC5/实测栈/700与900us/波形/吞吐/实车待验。
完整dual依次gray→FSM→参数→App，样本缺失导致后续不执行；独立子项历史通过必须标独立来源，不能写成完整入口通过。旧phase16主入口异常退出可能保留旧成功JSON，使用前看日期、摘要和实际日志。

### 内容审查边界

等待期确有电感/灰度/采集异常刺激，右环历史阶段名含LEFT；不沿用原审计误判。探针只比较某状态时，不扩写成全部硬件调用无副作用；CSV历史字节相同也不独立证明57列契约正确。
栈4176/8192是含库/未解析预留规划，不是实测峰值。软件覆盖不能用脚本数或608计数当分支覆盖。

**需求来源：** [P16 FR-24](../../specs/features/16-spi-wifi-link/requirements.md)。实现入口：[tests/dual_mode_tracking.py](../../tests/dual_mode_tracking.py)、[tests/results/phase17-validation.json](../../tests/results/phase17-validation.json)、[tests/results/phase17-resources.json](../../tests/results/phase17-resources.json)。

<a id="f32"></a>
## F32 标准R08与维护
### 持续规则

现行执行政策以AGENTS“2026-10-31验证要求”为准：格式/命名、C完整H摘要注释、上下文逻辑、固件改动当前配置零警告构建、R08人工记录及提交前检查强制。双LCD/架构/迁移/宿主/资源按需，减少新脚手架；纯文档不运行固件编译/R08实现检查。
R08命令在仓库根：`python SmartCar/tests/comment_review_check.py --base <本批完整基线ID>`，先明确暂存任务文件并更新人工记录/真实依赖摘要；工具不能证明中文语义。code-style是格式与固定边界唯一详细标准，本页不复制CS条款。

### 三级变更闭环

前层治理约束完整 Phase；目标草案、详细设计和本次映射保存在 Phase 并先确认、独立提交。开工组先更新 Docs 并标“本组实现同步中”、Phase/任务组及旧代码基线，紧接着实现；双向核对后取消标记，匹配 Docs 与代码同批交付，Phase 记录实际结果。未开工组目标不预写 Docs。阶段关闭后的独立修改新增 Phase，进行中可以增组。未决冲突禁止相关编码，无关工作可继续。
每功能唯一主条目，每模块可反查；辅助函数归所属功能，不机械每C一篇。旧迁移JSON/注释记录/历史结果冻结，不回写新符号伪造当时状态。增量内容、路径、锚点与范围需核对，不例行增加工具依赖。

**需求来源：** [P11 FR-07](../../specs/features/11-hardening-cleanup/requirements.md)；[P14 FR-22](../../specs/features/14-dual-mode-tracking/requirements.md)；[P15 FR-01, FR-02, FR-03, FR-04, FR-05, FR-10, FR-11, FR-12, FR-13, FR-14, FR-15, FR-16, FR-17, FR-18, FR-19, FR-20, FR-21, FR-22, FR-23, FR-24, FR-25](../../specs/features/15-code-style-consistency/requirements.md)；[P17 FR-01, FR-02, FR-13](../../specs/features/17-app-architecture/requirements.md)。实现入口：[AGENTS.md](../../AGENTS.md)、[specs/code-style.md](../../specs/code-style.md)、[tests/comment_review_check.py](../../tests/comment_review_check.py)。

## 本页实现与模块反查

下表是当前源码定位；公共/私有辅助函数归所属功能，不把声明视为另一实现。Core生成部分只作边界，用户区按原标记维护。

| 文件 | 主功能 | 定义/配置定位 |
|---|---|---|
| [AGENTS.md](../../AGENTS.md) | 治理 | 当前执行规则与Git边界 |
| [specs/code-style.md](../../specs/code-style.md) | 标准 | 格式、命名、注释与行为保持 |
| [tests/comment_review_check.py](../../tests/comment_review_check.py) | 本页跨模块协作 | 开发检查入口；非固件C定义 |
| [tests/dual_mode_tracking.py](../../tests/dual_mode_tracking.py) | 本页跨模块协作 | 开发检查入口；非固件C定义 |
| [tests/results/phase17-resources.json](../../tests/results/phase17-resources.json) | 本页跨模块协作 | 历史结果/证据；不表示本轮重跑 |
| [tests/results/phase17-validation.json](../../tests/results/phase17-validation.json) | 本页跨模块协作 | 历史结果/证据；不表示本轮重跑 |

<a id="r08-schema"></a>
## R08执行与记录接口

当前工具接口由本节维护；[Phase15 R08阶段契约](../../specs/features/15-code-style-consistency/r08-review-contract.md)完整保留当时设计与批准依据；格式/内容标准仍见[code-style](../../specs/code-style.md)，何时执行以[AGENTS](../../AGENTS.md#验证要求2026-10-31-起)为准。

`comment_review_check.py`仅依赖Python标准库和Git：--base基线、--records仓库相对JSON路径、--repo仓库；读取暂存区而非工作区，成功0，缺项/过期/未决/非法输入1并列失败对象。

### JSON v1与失效规则

记录保存于[既有证据文件](../../specs/features/15-code-style-consistency/comment-evidence.json)，顶层 version=1、files 为路径到记录的映射。
路径均使用仓库相对 POSIX 路径，禁止绝对路径、空段和 ..。
每条记录包含 sha256（暂存文件原始字节摘要；删除为 null）、reviewed_base（完整提交 ID）、
reviewer、reviewed_at（ISO 日期）、unresolved（必须为空数组）、dependencies（路径到摘要）。
items 是非空数组，每项包含 object、fact、evidence（非空数组，含 path、anchor、revision）。
revision 为 index 或 base；base 依据取该条 reviewed_base。依据路径必须有对应摘要：
index 使用本文件或 dependencies；base 使用 base_dependencies。
checks 必须包含 coverage、format、content、behavior 四个对象，各含 status=pass 和具体 reason。
新增/删除/重命名（重命名以删除＋新增检查，不依赖 Git 相似度猜测）均须含具体 cleanup 说明。
Core 例外另含 exception 的 scope=generated-only、reason、evidence；仍需上述人工记录。

检查基线与暂存区的差异。受影响范围文件必须有记录且 reviewed_base 等于 --base 完整 ID。
另外扫描全部已登记文件：自身或依赖摘要失配即失败，不因本次未直接修改该文件而跳过。
依赖发生变更时，即使同步了摘要，也必须提供本基线的新复核记录。
依据 anchor 必须在指定版本实际存在；检查存在性不证明推理正确。
空白、TODO、TBD、待确认、待处理、同上、保持原逻辑、按原逻辑等占位文本不接受。
不扫描任意 Git 文件作为程序，不自动生成事实或 pass 结论，不改索引/工作区。

本轮没有移动JSON或修改其摘要。历史22项工具测试不要求随纯文档任务重跑。

## 可选工具表与运行环境

下列命令在仓库根执行，先确认当前任务确实需要该项。Python使用实际已安装环境；原生gcc须可由脚本找到，不能把ARM交叉编译器当宿主gcc。纯文档本轮没有执行下表入口。

| 目标 | 入口 | 前提/输出边界 |
|---|---|---|
| 布局/架构/迁移 | `python SmartCar/tests/app_layout_check.py`、`app_architecture_check.py`、`app_migration_check.py`（后二者同tests目录） | 后二者需要pycparser与Git基线；默认结构检查不证明行为 |
| 数值/控制/双模式 | `python SmartCar/tests/control_equivalence.py`、`dual_mode_tracking.py`（同tests目录） | 原生gcc；dual还需R07原始文件；生成临时夹具，不当硬件验收 |
| 参数/日志/通信 | 同目录`runtime_tuning.py`、`datalog_v2.py`、`scb_protocol.py`、`scb_link.py`、`spi_wifi_state.py`、`spi_log_export.py` | Python驱动真实C及BSP/HAL替身；协议固定向量不能自行改成当前输出 |
| 结构拆分边界 | 同目录`g2_system.py`、`g3_control.py`、`g4_boundaries.py`、`g4_transactions.py`、`g4_hmi.py`、`g4_legacy_boundary.py`、`g5_boundaries.py` | 按变更边界选取，历史提交与原生gcc需可用，不默认全量 |
| 新资源对照 | `python SmartCar/tests/phase17_resources.py --output <新的目录>` | ARM GCC/Git Bash/历史提交；会生成产物及结果，不例行覆盖历史 |
| 实现提交R08 | `python SmartCar/tests/comment_review_check.py --base <完整ID>` | 暂存明确路径、更新人工证据/依赖；不以工具替代内容审查 |

`test_comment_review_check.py`仅在检查器本身改变或专门工具验收需要时使用；本轮不改工具、不重跑其22项历史回归。真正硬件模块测试、AC5及板上预算仍按所属阶段实体清单保留。
