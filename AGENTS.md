# STM32H7 项目协作入口

本目录是整个 STM32H7 项目的 Git 仓库根目录，不等于 `SmartCar/` 固件项目根目录。先判断任务涉及哪些目录，再阅读对应资料与规则。

## 目录与规则范围

- `SmartCar/`：智能车固件。涉及其代码、工程配置或固件规约时，先读 `SmartCar/AGENTS.md`，再严格按顺序读 `SmartCar/specs/mission.md`、`SmartCar/specs/techstack.md`、`SmartCar/specs/roadmap.md`，再读 `SmartCar/specs/code-style.md` 和本任务的功能规约，再沿规约链接阅读对应 `SmartCar/Docs/` 功能条目。本条所述固件专用门禁只适用于该目录；ESP32Bridge按下方独立入口执行自身SDD规则。
- `Hardware/`：原理图、PCB、封装、器件及硬件分析资料；`Material/引脚分配/`：引脚与接口资料。按本任务需求和相关文件处理，不套用 `SmartCar/` 的功能规约或阶段门禁。
- `Material/MiniSTM32H7xx-master/`、`Material/ESP32-S3-DevKitM-1/`：核心板、模块及 SDK 参考资料；`Material/Sirius20260718/`：一次性纳入的 STC 算法码源快照，不按固件功能阶段维护。引用时核对实际文件与版本，不把参考资料的修改当成 `SmartCar/` 实现。

跨目录任务分别核对固件、硬件和资料的影响；只要修改 `SmartCar/` 的实现代码或工程配置，固件部分仍须遵守 `SmartCar/AGENTS.md` 的规约先行要求。硬件或资料单独修改时不要求建立固件规约。

## 开发与验证原则（2026-10-31 用户决定）

- 普通软件改动以格式/命名、注释准确性和上下文逻辑审查为基础；核对调用顺序、状态归属、接口、错误路径和跨模块冲突，不以“看起来通顺”代替具体审查结论。
- 尽可能减少直接测试软件实现的脚本，不为普通改动例行新增宿主测试、模拟对端、临时 C 驱动或其他测试脚手架；确有任务需要时优先复用已有工具。
- 固件修改仍须当前配置编译零错误、零警告；`SmartCar/` 的代码标准和 R08 人工证据/提交前检查继续强制。完整分类与执行范围见 [`SmartCar/AGENTS.md`](SmartCar/AGENTS.md#验证要求2026-10-31-起)。
- 双 LCD 构建、架构/迁移脚本、软件行为测试、资源/历史对照改为按需，不默认全量运行。真实硬件模块测试入口、测试能力及相关板上验证保留；假 BSP/HAL、虚拟 Flash、模拟 SPI 对端仍属于软件测试。
- 本次不清理已有脚本，不改写历史结果，不把硬件待验项改成完成。纯文档变更只做文档交叉核对，不要求固件编译或软件行为回归；规约先行与 Git 范围检查保持。

## 全仓库 Git 边界

- 开始和提交前检查仓库状态，识别已有改动。只用本任务的明确路径暂存文件；检查暂存清单与差异后再提交。不要使用 `git add .`、`git add -A` 或 `git commit -a` 混入其他目录的在途工作。
- 跨固件与硬件的改动按各自范围核对并清楚说明提交内容。提交规则以任务要求及对应目录规则为准；默认只 commit，未获用户明确要求不 push。

## 推送流程与连接排查

1. 推送前执行 `git status --short`、`git branch --show-current` 和 `git remote -v`，核对工作区状态、当前分支、远端地址与待提交范围。按明确路径暂存，检查 `git diff --cached --name-status` 与 `git diff --cached`，展示文件清单、差异摘要和提交信息后提交。默认只 commit，用户明确要求后才 push。
2. 普通推送使用 `git push -u origin HEAD`，将当前分支推送到 origin 的同名分支并设置跟踪关系；执行前须确认当前处于具名分支。
3. 遇到 `Connection was reset` 等连接错误时，检查域名解析及 Windows 代理设置，不反复盲目重试。PowerShell 可用以下命令读取系统代理；本机 2026-10-31 成功使用的代理为 `127.0.0.1:10808`，以后使用前须核实当前设置及代理服务仍运行，不能把该端口视为所有机器的固定配置。

   ```powershell
   Get-ItemProperty 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings' | Select-Object ProxyEnable,ProxyServer,AutoConfigURL
   Test-NetConnection 127.0.0.1 -Port 10808
   ```

4. 确认代理可用后，使用此次已验证的单次命令；`-c` 仅对本次 Git 操作生效，不修改永久 Git 配置：

   ```powershell
   git -c http.proxy=http://127.0.0.1:10808 -c http.version=HTTP/1.1 push -u origin HEAD
   ```

5. 推送后查询远端分支，并与本地完整提交号核对。使用代理推送时，查询也使用同一代理；直接连接成功时可省略代理参数。以下命令在 PowerShell 中执行，只有远端查询成功且目标分支提交号与本地 HEAD 一致，才能报告推送成功；同时如实说明工作区是否仍有改动。

   ```powershell
   $pushBranch = git branch --show-current
   git -c http.proxy=http://127.0.0.1:10808 -c http.version=HTTP/1.1 ls-remote --heads origin "refs/heads/$pushBranch"
   git rev-parse HEAD
   git status --short
   ```

6. 如果 Git 提示 `.git/index.lock` 已存在，先检查相关 Git 进程及正在进行的提交、暂存或其他操作。仅在确认没有相关进程、没有正在进行的操作且该锁确为遗留文件后，才清除锁并重试；不得为继续操作直接删除活动锁。权限或自动审批阻止操作时，按实际提示处理，不绕过拒绝。

**2026-10-31 实例：** 提交 `d710e8fa32bece0bbb091e6edb2fb90833af320a`（短号 `d710e8f`）通过上述单次代理命令成功推送到 `https://github.com/LiRuthless/STM32H7.git` 的 `chore/constitution-documentation-alignment` 分支；远端分支与本地 HEAD 完整提交号一致，工作区干净。本次用户明确授权整工作区推送，范围包括历史构建产物、日志、副本及 `.hq/edge.db` 本地数据库；该授权仅适用于本次，不成为以后默认提交范围。现有忽略规则继续有效，不因“整个工作区”而自动强制纳入被忽略文件。

## 文档体系关系（2026-11-07）

前层治理文档 → Phase → Docs ⇄ 代码。Phase 完整保存背景、需求、详细设计、用户决定、实施计划、文件与符号映射、验证标准和实际结果，按该阶段基线解释；Docs 说明当前固件功能、算法、状态、时序、参数、错误与兼容性。功能语义对应不要求逐行翻译。阶段显式关闭后的独立修改新增 Phase；进行中的阶段可以补充任务组，软件完成但硬件待验不自动视为关闭。

Phase 确认并独立提交 → 先更新本组 Docs → 紧接着实现代码 → 双向核对 → 匹配 Docs 与代码同批交付。目标草案与未开工组设计保存在 Phase；本组 Docs 短暂领先时标明“本组实现同步中”、Phase/任务组及更新前代码基线，完成前取消标记。
入口见[功能文档](SmartCar/Docs/index.md)。本次纠偏不新增 Phase18。

## ESP32Bridge 独立范围（2026-11-07）

[ESP32Bridge协作入口](ESP32Bridge/AGENTS.md)及其前层文档共同治理该工程，采用前层治理 → 完整Phase → Docs ⇄ 代码。技术与验证按ESP32自身规则，不套用STM32 R08、AC5或双LCD要求；[功能入口](ESP32Bridge/Docs/index.md)。
