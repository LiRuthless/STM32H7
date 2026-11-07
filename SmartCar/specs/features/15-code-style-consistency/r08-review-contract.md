# R08 注释修复与持续复核契约

日期：2026-10-22。用户已批准五模块整改及检查脚本＋人工证据。
实现前基线：a738ded668bc56d8975707836ab28226b1ecdbc6。

## 需求（FR-21～FR-25）

- FR-21：复核 BSP 的 bsp_dl1b、bsp_adc、bsp_key、bsp_pwm 及 App 的 pid 的全部 C/H。
  修正模块、函数、枚举、阶段说明；C 为完整契约，H 为摘要。固件非注释 token、
  预处理结构、接口、参数、状态、时序均不变；R01～R07 不在修复范围。
- FR-22：每个模块和函数提供职责、数据流、错误路径、时序/并发、算法/单位的具体依据。
  不适用项写原因。跨文件事实登记依赖文件。内容、覆盖、格式、行为分别验收。
- FR-23：新增只读检查器 tests/comment_review_check.py，仅依赖 Python 标准库与 Git。
  --base 指定基线提交，--records 指定仓库相对 JSON 路径；--repo 可指定仓库。
  所有待验内容包括 JSON 均读取暂存区，不读取工作区替代部分暂存内容。
  成功退出 0，缺项/过期/未决/非法输入退出 1；列出具体失败对象。
- FR-24：范围是 App 下 C/H、BSP/bsp*.c/h、BSP/lcd/lcd.c/h 和 Core 下 C/H。
  Core 仅生成区变更可逐文件登记例外，用户区变化不能豁免；第三方不纳入默认范围，
  但作为已登记证据依赖变化仍会使相关复核失效。不安装 hook 或修改全局 Git 配置。
- FR-25：以后每次固件实现提交，暂存明确路径后运行检查器。旧记录与源码绑定，
  依赖变化重新审查；不得把检查器通过说成语义自动证明。未登记模块随变更补齐。

## JSON v1 与失效规则

记录保存于本目录 comment-evidence.json，顶层 version=1、files 为路径到记录的映射。
路径均使用仓库相对 POSIX 路径，禁止绝对路径、空段和 ..。
每条记录包含 sha256（暂存文件原始字节摘要；删除为 null）、reviewed_base（完整提交 ID）、
reviewer、reviewed_at（ISO 日期）、unresolved（必须为空数组）、dependencies（路径到摘要）。
items 是非空数组，每项包含 object、fact、evidence（非空数组，含 path、anchor、revision）。
revision 为 index 或 base；base 依据取该条 reviewed_base。依据路径必须有对应摘要：
index 使用本文件或 dependencies；base 使用 base_dependencies。
checks 必须包含 coverage、format、content、behavior 四个对象，各含 status=pass 和具体 reason。
删除/重命名（以删除＋新增检查，不依赖 Git 相似度猜测）须含具体 cleanup 说明。
Core 例外另含 exception 的 scope=generated-only、reason、evidence；仍需上述人工记录。

检查基线与暂存区的差异。受影响范围文件必须有记录且 reviewed_base 等于 --base 完整 ID。
另外扫描全部已登记文件：自身或依赖摘要失配即失败，不因本次未直接修改该文件而跳过。
依赖发生变更时，即使同步了摘要，也必须提供本基线的新复核记录。
依据 anchor 必须在指定版本实际存在；检查存在性不证明推理正确。
空白、TODO、TBD、待确认、待处理、同上、保持原逻辑、按原逻辑等占位文本不接受。
不扫描任意 Git 文件作为程序，不自动生成事实或 pass 结论，不改索引/工作区。

## 实施顺序与验收

1. 规约、标准、AGENTS 增量独立提交后，才编辑 C/H 或检查器。
2. 对五模块逐函数复核并填证据，纠正报告六处及同模块错误；历史 CF/CM 保留并追加更正。
3. 实现检查器及临时 Git 仓库测试：缺项、过期、自身/依赖变化、仅注释、增删改名、
   部分暂存、合法/非法 Core 例外、占位/未决、完整通过、只读不变。
4. 验证十个 C/H 非注释内容及预处理不变，词法处理须识别字符串、字符、转义及续行。
   隔离副本执行 LCD=0/1 零警告构建和 control_equivalence、dual_mode_tracking、datalog_v2。
   R07 外部文件缺失如实记录阻塞，不改测试绕过并声称全通过；可单列独立子测试结果。
5. 暂存注释、工具和证据，执行新门禁后提交实现；回填 validation、roadmap、CHANGELOG
   及审查报告，再独立提交。不 push、不碰硬件改动或已有构建产物。

## 证据要求与限制

人工须从当前实现及调用方验证公式、转换先后、失败输出、状态归属、硬件映射与调度。
不能把预期设计写成已实现，不能编造单位。PID 窄化和 DL1B 阻塞风险如实保留。
复核与撰写分开进行，不批量用模板替代事实。未知事实暂停相关项询问用户。
摘要采用文件级保守失效；未登记的隐式依赖仍需人工影响分析。
本机制不是不可绕过的 Git hook，也不是完整 C 语义证明。

> 2026-11-07 文档纠偏：本页恢复本阶段完整正文（恢复依据 `68cb508`），原路径、符号、决定和结果按当时基线解释。当前功能见 [Docs](../../../Docs/index.md)，后续演进见 [路线图](../../roadmap.md)。历史验证安排不自动成为后续每次改动的门禁；本次未重新验收固件或实机。


## 现行功能导航

- [docs F32](../../../Docs/guides/validation.md#f32)
- [JSON规则](../../../Docs/guides/validation.md#r08-schema)
