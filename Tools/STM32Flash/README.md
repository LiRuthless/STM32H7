# 工作区 STM32 烧录入口

2026-10-20 用户指定：以后使用本工具，默认烧录 **Keil** 产物。

1. 在 Keil 编译 `SmartCar/MDK-ARM/SmartCar.uvprojx`，生成 `SmartCar/MDK-ARM/SmartCar/SmartCar.hex`。
2. ST-Link 接好 SWD 并给目标板供电。
3. VS Code：`Ctrl+Shift+P` → `Tasks: Run Task` → **flash**。

烧录执行写入、校验、复位运行，不自动编译；每次烧录前须在 Keil 重新编译所需版本。`Ctrl+Shift+B` 仍是原 GCC 构建任务，与 Keil 烧录独立。

也可在 PowerShell 从仓库根目录调用：

```powershell
& ./Tools/STM32Flash/flash.ps1
# 只检查路径和依赖，不连接开发板
& ./Tools/STM32Flash/flash.ps1 -CheckOnly
# 显式指定其他 HEX（相对当前调用目录或绝对路径）
& ./Tools/STM32Flash/flash.ps1 -Firmware 'D:\固件目录\SmartCar.hex'
```

脚本兼容 Windows PowerShell 5.1 / PowerShell 7。若执行策略阻止脚本，使用 VS Code 的 flash 任务（仅该进程使用 Bypass）。旧 `SmartCar/MDK-GCC/flash.sh` 转发到同一入口，默认同样为 Keil HEX。

## 工具来源和恢复

- 用户提供：`STM32-VSCode烧录工具.zip`。
- 原包 SHA256：`38AB9384F491640229D8A32C641948712180ECAD091AD517662230B5937E2A14`。
- OpenOCD：`0.12.0-rc2-g7a1adfbec (2022-11-06)`，与原包 945 个文件逐个 SHA256 对照一致。
- `source-flash.ps1`、`source-使用说明.md` 为原包参考材料，不作为本工作区操作指令。
- `openocd/` 已完整安装在本机，保留原有依赖、文档和许可证；第三方目录在本地忽略 Git，不随源码提交。
- 新克隆或缺失工具时，校验原 ZIP 的上述 SHA256，将包内 **整个 openocd 文件夹**解压至本目录；不要用包内 `.vscode/tasks.json` 覆盖工作区配置。
- ST-Link USB 驱动仍需在本机可用；本次未安装驱动，未实际烧录。

版本及只读检查通过代表入口可运行，实际连接和烧录结果以设备上的执行输出为准。
