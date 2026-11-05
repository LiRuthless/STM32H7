# Phase 01 Group 1 实施与验证证据

> 历史证据说明（2026-11-05）：本文按原日期/镜像/环境保存；旧“本轮/当前”不作为现行进度。现行状态见[roadmap](../../specs/roadmap.md)，方法见[验证指南](../guides/validation.md)。本次未重跑本文检查；本副本缺原artifacts/.local时仅保留历史报告与JSON，不声称重新核验产物。

日期：2026-10-17（Asia/Shanghai）。实施依据为已提交规约 `5c15c49`，实施分支 `codex/01-project-foundation`；开始时全仓工作区干净。本组复用既有 FR-1、FR-3、TC-1、TC-3，无新增需求或共享接口，未作新提交。G-01～G-03未解除。

## 实际交付与边界

- 工程：根/main CMake，目标锁定 esp32s3；显式 SDK 组件依赖，最小组件闭包，不依赖参考工程、SmartCar或PSRAM，禁止单核配置。
- 配置：Kconfig与本地说明；`.local/sdkconfig.local`为唯一密码来源，构建前检查原始输入及已有生成配置，Kconfig生成后复核一致性。密码无效/缺失、旧缓存密码不一致、其他配置项混入均拒绝，固定诊断不回显输入。
- 环境：核对指定SDK路径和6.0.0版本声明、实际IDF Python，以及该SDK `tools.json`推荐的编译器/CMake/Ninja；运行SDK Python依赖检查，无自动安装或切换。
- 固件入口：`void app_main(void)`对编译后的密码长度/ASCII/NUL再次检查，只报告配置和后续组未实现状态。没有服务假成功、模拟车辆数据或桥接功能。
- 尚未实现：阶段化启动、NVS/AP/socket/SPI初始化、固定池/队列（Group 2），复制/产物回收构建脚本与完整中文构建说明（Group 3），阶段02～06。测试用手工复制不计作FR-5交付。

## 环境与目标构建

| 项目 | 本次实际结果 |
|---|---|
| SDK | `D:\esp\v6.0\esp-idf`，`version.cmake`为6.0.0，Git tag显示`v6.0`，提交`662a3be354759d9487bf4b1a629fadb766cb1800` |
| SDK改动 | 普通本机环境执行`git -C D:\esp\v6.0\esp-idf status --short`为空，含递归子模块检查；沙箱内早先的子模块属主检查失败未算作干净证据，未修改safe.directory全局配置 |
| Python | `C:\Espressif\tools\python\v6.0\venv\Scripts\python.exe`，3.11.15；SDK `check-python-dependencies`通过 |
| 构建工具 | CMake 4.0.3、Ninja 1.12.1、Xtensa GCC 15.2.0 / `esp-15.2.0_20251204`；均匹配该SDK推荐版本 |
| 其他实际工具 | esptool 5.5.0；Git 2.56.0.windows.2（构建日志） |
| 官方导出限制 | `export.ps1`失败；在已选择EIM路径后，全量`idf_tools.py check`仍报缺`riscv32-esp-elf-gdb`、无法识别`esp-rom-elfs`安装。没有安装/修复SDK，未将全量导出记为通过 |
| 本次环境选择 | 读取已有`C:\Espressif\tools\Microsoft.v6.0.PowerShell_profile.ps1 -e`提供的工具路径，显式指定上述SDK/Python，再由本工程检查真实版本；未运行profile的`eim select`分支。官方全量导出失败未阻止本次esp32s3构建 |
| 沙箱限制 | 沙箱内PowerShell设置的PATH未传入Python子进程；SDK核验、完整主机测试及构建在获准的普通本机执行环境运行 |
| 构建目录 | `D:\STM32H7\ESP32Bridge\.local\g1-project`，无空格；手工白名单复制根CMake、sdkconfig.defaults、main、scripts及测试配置，无.git或其他目录源码 |
| 测试配置 | 随机生成的一次性测试密码，只在被忽略的本地文件/生成配置中；不记录内容。源工程部署路径`.local/sdkconfig.local`仍缺失，测试原料保存在`.local/g1-test-sdkconfig.local`；测试镜像不得当作部署配置 |
| 目标结果 | `idf.py -B build build`退出0，生成BIN/ELF/map、bootloader及partition_table；项目描述为esp32s3，组件源码均来自SDK或复制后的本工程main |
| 硬件事实 | 未连接/烧录/启动测量；SDK默认生成的Flash参数（2MB）不是实板容量证据 |

实际命令（环境选择过程的本地复现辅助文件为被忽略的`.local/activate-verified.ps1`，不是Group 3交付脚本）：

```powershell
# 仓库根目录；辅助文件仅选择上表中已安装的工具，不安装或修改SDK。
. .\ESP32Bridge\.local\activate-verified.ps1
python D:\esp\v6.0\esp-idf\tools\idf.py --version
python D:\esp\v6.0\esp-idf\tools\idf_tools.py check
python D:\esp\v6.0\esp-idf\tools\idf_tools.py check-python-dependencies
python -m unittest discover -s ESP32Bridge/tests -v
# 手工白名单复制至上述g1-project，并放入一次性本地测试配置后：
Set-Location D:\STM32H7\ESP32Bridge\.local\g1-project
python D:\esp\v6.0\esp-idf\tools\idf.py -B build build
python D:\esp\v6.0\esp-idf\tools\idf.py -B build size
```

原始本机日志：`ESP32Bridge/.local/g1-host-tests.log`、`.local/g1-target-build.log`、`.local/g1-target-size.log`；构建及工具本身的诊断保存在`.local/g1-project/build/log/`。均未纳入Git。脱敏清单及SHA256见 [证据清单](phase-01-group-1-evidence.json)。

首轮目标配置发现错误组件名`esp_log`，修正为本机SDK实际组件`log`后完整构建通过；失败日志保留于`.local/g1-target-build-attempt-1.log`。一次主机断言因CMake自动折行失败，修正为按空白归一化比对诊断后通过；不是放宽实现的拒绝行为。

## 分类验收

| 类别 | 实际执行及结果 | 不能推导的结论 |
|---|---|---|
| 文档/静态 | Git范围、忽略规则、文件清单、文档相对链接、规约映射及原协议/来源副本未改；结果见证据清单 | 不代表启动或实机通过 |
| 主机配置模拟 | 16个unittest全部通过，无跳过；其中7个调用真实CMake入口，覆盖无环境、其他IDF路径、v5.5.5路径、错误target、缺密码、64字节密码不回显、缓存密码不一致；其余覆盖0/7/8/63/64边界、非ASCII、NUL、转义、重复/畸形/越权项、生成target/PSRAM/单核、版本声明、Python及真实PATH缺工具 | 不代表SCB模拟器、网络服务或实机密码路径通过；版本声明/部分环境错误采用隔离替身 |
| ESP32目标 | 完整构建、更新缓存预检查后的增量构建、`size`均退出0；配置为esp32s3、双核、SPIRAM禁用 | 不代表服务实现、实板Flash容量或目标启动通过 |
| 实机测量 | 未执行 | 无堆、最低余量、任务栈、DMA、时序或吞吐实测 |

`size`本次报告：BIN文件161568B，image size 161453B；DIRAM静态占用47411B（其中`.data`12428B、`.bss`2448B），IRAM16384B。仅为Group 1链接布局，不是完整桥接内存预算，也不是运行时可用堆。实测项保持未执行。

SDK组件闭包中包含`esp_psram`目录是SDK自身依赖关系；最终`CONFIG_SPIRAM`禁用，未启用或要求外部RAM。不能仅用组件目录名判断已使用PSRAM。

## 审查与下一步

本组文件清单以 [证据清单](phase-01-group-1-evidence.json) 的`changed_files`为准：工程/配置/检查/测试文件与关联SDD状态、验证报告一起审查；未暂存、未提交。建议中文提交信息：`feat(esp32bridge): 完成 Phase 01 Group 1 工程与配置校验`。

等待用户审查确认后，才按明确路径暂存，展示暂存清单/差异再提交。不push、不合并。Group 2在本组审查后核对规约：固定池容量/所有权及启动失败行为若需必要增量，先单独交用户确认、纯规约提交，再写代码；Phase 01未完成，暂不出阶段完成交接。
