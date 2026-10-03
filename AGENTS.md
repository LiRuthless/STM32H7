# STM32H7 项目协作入口

本目录是整个 STM32H7 项目的 Git 仓库根目录，不等于 `SmartCar/` 固件项目根目录。先判断任务涉及哪些目录，再阅读对应资料与规则。

## 目录与规则范围

- `SmartCar/`：智能车固件。涉及其代码、工程配置或固件规约时，先读 `SmartCar/AGENTS.md`，再严格按顺序读 `SmartCar/specs/mission.md`、`SmartCar/specs/techstack.md`、`SmartCar/specs/roadmap.md`，然后读本任务的功能规约。只有这个目录采用其中的 SDD 工作流。
- `Hardware/`：原理图、PCB、封装、器件及硬件分析资料；`引脚分配/`：引脚与接口资料。按本任务需求和相关文件处理，不套用 `SmartCar/` 的功能规约或阶段门禁。
- `MiniSTM32H7xx-master/`、`ESP32-S3-DevKitM-1/`：核心板、模块及 SDK 参考资料；`Sirius20260718/`：一次性纳入的 STC 算法码源快照，不按固件功能阶段维护。引用时核对实际文件与版本，不把参考资料的修改当成 `SmartCar/` 实现。

跨目录任务分别核对固件、硬件和资料的影响；只要修改 `SmartCar/` 的实现代码或工程配置，固件部分仍须遵守 `SmartCar/AGENTS.md` 的规约先行要求。硬件或资料单独修改时不要求建立固件规约。

## 全仓库 Git 边界

- 开始和提交前检查仓库状态，识别已有改动。只用本任务的明确路径暂存文件；检查暂存清单与差异后再提交。不要使用 `git add .`、`git add -A` 或 `git commit -a` 混入其他目录的在途工作。
- 跨固件与硬件的改动按各自范围核对并清楚说明提交内容。提交规则以任务要求及对应目录规则为准；默认只 commit，未获用户明确要求不 push。
