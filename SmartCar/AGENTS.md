# AGENTS.md

本项目采用规约驱动开发（SDD）。开始任何工作前，按顺序阅读：

1. `specs/mission.md` — 项目为什么存在
2. `specs/techstack.md` — 技术约束（含硬性约束，不可违反）
3. `specs/roadmap.md` — 当前进度与下一步

## 工作规则

- 实现代码前必须存在对应的功能规约（`specs/features/NN-*/`）；没有规约的功能先写规约再动手。
- 文档修改需同步更新关联文档（plan / requirements / validation / roadmap），防止漂移。
- 每个任务分组完成后运行对应 `validation.md` 中的自动化验证（基线：`MDK-GCC/build.sh` 构建零警告）。
- **接入任何当前未启用的赛道元素或休眠代码前，必须逐个与用户确认设计**（用户明确要求）；确认结论回填到对应规约后再实现。
- 已知缺陷与整改排期见 roadmap Phase 08–11，不要顺手在别的任务里"顺带修"，按阶段走。
- Git 操作遵守下文「Git 约定」。

## Git 约定

- **仓库根在工作区根目录**（`../.git`，本目录是其子目录）：提交时注意路径范围，只 `git add` 与本任务相关的文件——工作区还有 `../Hardware/`、`../引脚分配/` 等其他在途改动，不得混入提交。
- **改动处理完毕后提交一次；默认只 commit 不 push**，push 必须由用户明确要求。
- **宪章修改走独立分支单独提交**：`specs/mission.md` / `techstack.md` / `roadmap.md` 的任何改动在 `chore/constitution-<topic>` 分支上进行并单独提交，不与功能代码混提。
- 功能开发走 `feature/NN-<name>` 分支，提交顺序：规约三件套 → 实现（按任务分组可分多次）→ 合并前更新 CHANGELOG；大规模再规划走 `chore/replan-<topic>`。
- 提交信息由智能体起草（中文、说明动机与关联 Phase）；每次提交前向用户展示提交信息草稿与文件清单。

## 构建与验证速查

- 构建（GCC）：`SmartCar/MDK-GCC/build.sh`（VS Code 任务 `Ctrl+Shift+B`）
- 烧录：`SmartCar/MDK-GCC/flash.sh`（OpenOCD + ST-Link）
- Keil 工程：`SmartCar/MDK-ARM/SmartCar.uvprojx`（AC5，需正式授权）
