# 验证证据与维护

现行功能说明，核对基线 `68cb508`；2026-10-31迁移。需求经[功能索引](../traceability.md)进入本页，再由实现表定位源码。
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

Phase定义FR和目标docs→docs写可判定行为、代码符号和验证→二者纯规约独立提交→实现→docs回填实际结果→Phase登记任务验收。目标未实现须标“待实现”；未决冲突禁止编码，但不阻塞其他无关文档迁移。
每功能唯一主条目，每模块可反查；辅助函数归所属功能，不机械每C一篇。旧迁移JSON/注释记录/历史结果冻结，不回写新符号伪造当时状态。增量内容、路径、锚点与范围需核对，不例行增加工具依赖。

**需求来源：** [P11 FR-07](../../specs/features/11-hardening-cleanup/requirements.md)；[P14 FR-22](../../specs/features/14-dual-mode-tracking/requirements.md)；[P15 FR-01, FR-02, FR-03, FR-04, FR-05, FR-10, FR-11, FR-12, FR-13, FR-14, FR-15, FR-16, FR-17, FR-18, FR-19, FR-20, FR-21, FR-22, FR-23, FR-24, FR-25](../../specs/features/15-code-style-consistency/requirements.md)；[P17 FR-01, FR-02, FR-13](../../specs/features/17-app-architecture/requirements.md)。实现入口：[AGENTS.md](../../AGENTS.md)、[specs/code-style.md](../../specs/code-style.md)、[tests/comment_review_check.py](../../tests/comment_review_check.py)。

## 本页实现与模块反查

下表是当前源码定位；公共/私有辅助函数归所属功能，不把声明视为另一实现。Core生成部分只作边界，用户区按原标记维护。

| 文件 | 主功能 | 定义/配置定位 |
|---|---|---|
| [AGENTS.md](../../AGENTS.md) | 本页跨模块协作 | 类型/布局/配置；关联同模块C定义 |
| [specs/code-style.md](../../specs/code-style.md) | 本页跨模块协作 | 类型/布局/配置；关联同模块C定义 |
| [tests/comment_review_check.py](../../tests/comment_review_check.py) | 本页跨模块协作 | 类型/布局/配置；关联同模块C定义 |
| [tests/dual_mode_tracking.py](../../tests/dual_mode_tracking.py) | 本页跨模块协作 | 类型/布局/配置；关联同模块C定义 |
| [tests/results/phase17-resources.json](../../tests/results/phase17-resources.json) | 本页跨模块协作 | 类型/布局/配置；关联同模块C定义 |
| [tests/results/phase17-validation.json](../../tests/results/phase17-validation.json) | 本页跨模块协作 | 类型/布局/配置；关联同模块C定义 |
