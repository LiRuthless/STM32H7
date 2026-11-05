# STM32 VS Code 烧录工具（Windows / ST-Link）

## 使用
1. 完整解压本包到普通文件夹，不要在压缩包里直接运行。
2. 在 VS Code 中选择“文件 → 打开文件夹”，打开含 flash.ps1 的这一层。
3. 把自己工程生成的 Intel HEX 文件复制到 firmware 文件夹，命名为 firmware.hex。
4. 通过 ST-Link 连接目标板的 SWDIO、SWCLK、GND 和参考电压，确保板子正确供电。
5. 按 Ctrl+Shift+P，执行 Tasks: Run Task（任务：运行任务），选择 flash - STM32 ST-Link。
6. 工具将写入 HEX、校验并复位运行。烧录会覆盖 HEX 地址范围内的现有程序。

## 适用范围
- 默认目标配置为 STM32H7，来自 STM32H743VIT6 项目。
- 需 Windows、VS Code，以及可用的 ST-Link USB 驱动；OpenOCD 已附带，不必另装 EIDE 或 Git Bash。
- 本包只负责烧录 HEX，不包含智能车源码、编译工具链或固件，也不会自动编译对方工程。
- 其他 STM32 系列须核对并修改 flash.ps1 的 Target 默认值；不能直接套用 H7 配置。
- 找不到 ST-Link 时，检查 USB 数据线、驱动、板子供电和 SWD 接线。

## 包内容
- flash.ps1：分享用便携入口，使用包内 OpenOCD，不含个人安装路径。
- .vscode/tasks.json：VS Code 烧录任务。
- openocd/：本机现有 OpenOCD 工具目录原样副本，含依赖及已有文档/许可文件。
- original/：原工程 flash.sh、build.sh 和 tasks.json，只作参考；含原电脑路径，不能直接移植使用。

## 验证范围
已检查任务配置、PowerShell 语法及 OpenOCD 可执行性；未连接开发板执行烧录。
OpenOCD 是第三方工具，转交或再分发时须保留其已有许可材料并遵守相应许可要求。
