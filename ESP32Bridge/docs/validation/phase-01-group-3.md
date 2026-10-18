# Phase 01 Group 3 实施与验证

记录日期：2026-10-18，Asia/Shanghai。分支`codex/01-project-foundation`；Group 2已随用户继续指令复核并提交为`a2947a8`，其纯规约提交为`73a2a2f`。本组复用已提交Phase 01 FR-5、FR-6、TC-1、TC-3，不新增共享协议或设计门禁，不修改固件实现、其他目录、SDK或硬件。

当前状态：脚本、说明、77项主机检查及5个真实入口场景全部完成，结果如下。Group 3代码尚未暂存/提交，Phase 01完整实机验收仍未完成。

## 交付与追溯

| 需求 / 检查 | 实际文件 / 符号 | 行为 |
|---|---|---|
| FR-5 / V-5 | [build.ps1](../../scripts/build.ps1)、[build_support.py](../../scripts/build_support.py) `build_project`、`copy_project` | 正式入口、独立无空格目录、白名单复制、实际IDF build/size、配置/源码边界检查、受控产物回收 |
| FR-5、TC-3 / V-5、V-9 | `prepare_work`、`safe_cleanup`、`validate_flash_metadata` | 唯一编号及所有权标记；清理前绝对路径/源边界/重解析点检查；失败非零且保留诊断和工作区；拒绝外部Flash文件 |
| TC-1 / V-7 | [activate.ps1](../../scripts/activate.ps1)、复用`check_environment` | 显式选择已有工具、读取指定SDK推荐版本并实际核验；拒绝其他SDK，失败恢复原环境，不安装或静默换版 |
| FR-3、FR-5 / V-3、V-9 | `Diagnostics`、`redact`、受控manifest | 不将密码写进命令、日志、版本清单、源文件哈希列表；生成配置和产物本地忽略 |
| FR-6 / V-6 | [中文构建说明](../build-and-flash.md)、README与本地配置说明 | 从终端选择环境、本人本地配置、正式构建、产物/诊断、失败排查及烧录边界 |

源根不递归复制，只取CMakeLists.txt、sdkconfig.defaults、main、scripts和必需的`.local/sdkconfig.local`；不搬移仓库，不带.git、其他目录、已有缓存或测试。成功回收BIN/ELF/map、bootloader、partition table、flash_args/flasher_args.json、size-report、脱敏build.log、environment及manifest。成功默认只清理本次工作区，`-KeepWork`可保留；失败保留诊断及已创建的工作区，不用残留镜像冒充成功。

## 本次环境与执行

SDK精确为`D:\esp\v6.0\esp-idf` v6.0.0，提交`662a3be354759d9487bf4b1a629fadb766cb1800`。实际选择Python3.11.15、CMake4.0.3、Ninja1.12.1、Xtensa`esp-15.2.0_20251204`。正式`activate.ps1`及每次build预检均实际核验，不调用安装或全局环境修改；官方全量export的历史限制仍单列，不冒充通过。

本次从PowerShell执行正式环境入口：

```powershell
. .\ESP32Bridge\scripts\activate.ps1
python -m unittest discover -s ESP32Bridge/tests -v
# 下列命令分别在本组隔离工程副本中执行；副本路径及退出码见证据清单。
pwsh -NoProfile -NonInteractive -File '<副本>\scripts\build.ps1' `
    -WorkRoot 'D:\STM32H7\ESP32Bridge\.local\g3-build-work'
```

上面`<副本>`为路径示意，真实绝对路径由本次`.local/g3-integration-cases.json`留档；执行结果、artifact/diagnostic manifest及哈希汇总见 [本组清单](phase-01-group-3-evidence.json)。全部副本均为本工程白名单文件，位于被忽略的`.local/g3-integration/`。成功路径使用一次性本地测试配置，源工程部署密码仍缺失。

## 分类结果

| 类别 / 场景 | 实际结果 | 证明边界 |
|---|---|---|
| 主机回归 | **77项通过，0失败、0跳过** | 59项原检查＋14项构建文件系统/注入＋4项真实PowerShell入口检查；不等同两次目标构建 |
| 普通源路径的正式build.ps1 | 退出0，build/size成功，规定产物已回收，本次工作区已删除 | 真实esp32s3构建；源工程保持原位，不含实机启动 |
| 含空格源路径的正式build.ps1 | 退出0，build/size成功，规定产物已回收，本次工作区已删除 | 目标工作区必须无空格，原副本保留 |
| 实际C编译失败 | 正式入口退出1，stage=build，IDF实际退出2；诊断和工作区保留，无成功产物发布 | 在隔离副本app_main添加明确#error，不伪造编译器返回值 |
| 实际复制失败 | 正式入口退出1，stage=copy；诊断和已创建工作区保留，源未搬移 | 隔离副本缺sdkconfig.defaults，应非零并保留工作区 |
| 源工程缺部署密码 | 正式入口退出1，stage=preflight；留诊断，没有创建目标工作区 | 正式入口应在preflight失败，不构建、不开放回退 |
| 文档与范围 | 静态复核通过，链接/需求/源码及文件清单见证据JSON | 仅本任务明确路径；Reports在途文件保留 |
| 实机测量/烧录 | **未执行** | 无实际堆、任务栈、DMA/READY/SCK/吞吐或联调证据 |

14项构建支持测试调用实际文件系统和生产编排逻辑，SDK子命令替身明确标记为HOST FIXTURE。覆盖复制、编译、size、产物缺失、密码不一致、外部组件/Flash路径、无密码、空白工作根、源/根目录与错误所有者清理拒绝、转义脱敏，以及真实Windows junction的复制和清理拒绝。4项入口检查真实运行PowerShell脚本，覆盖缺解释器/环境、其他SDK拒绝及失败回滚。

首次77项回归中，回滚测试发现原先未设置的IDF_PATH未恢复为未设置。已修复为空值时显式删除环境变量，复测4项入口及最终77项全回归均通过；没有删除此检查或放宽判据。测试日志为`.local/g3-host-tests.log`，入口复测日志`.local/g3-entry-tests.log`。该回滚分支修复晚于集成副本准备；最终环境入口正常调用、真实回滚检查及77项回归验证的是修复后的版本。两次成功构建的build.ps1/build_support.py和全部固件输入与当前源码哈希一致，副本中旧回滚分支未伪装为新版本，区别在清单明确记录。没有重新运行无关实机项目来填数。

BIN/ELF必然包含本地固件配置，因此产物仍为本地私有、被忽略的数据；不承诺可以公开。发布目录不包含sdkconfig/sdkconfig.h或整个缓存。日志/JSON/报告只保留脱敏信息；产物哈希证明本次文件，不代表三端协议共同向量。

## 审查与交接

本组新增脚本、测试、中文说明，以及plan/requirements/validation/roadmap/CHANGELOG/traceability状态回填。完整待审文件在清单`changed_files`；未暂存、未提交Group 3实现。建议中文提交信息：`feat(esp32bridge): 完成 Phase 01 Group 3 构建与产物交付`。

用户审查确认后再按明确文件清单暂存、检查差异并提交。Phase 01的软件组交付与完整实机验收分别看待，见 [阶段交接](../phase-01-handoff.md)；后续阶段不据此自动标为已实现，G-01～G-03仍须对应功能前逐项确认。
