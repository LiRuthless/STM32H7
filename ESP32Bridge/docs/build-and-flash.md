# ESP32Bridge 环境、配置与构建

本说明对应Phase 01 Group 3。目标固定`esp32s3`，SDK精确为`D:\esp\v6.0\esp-idf`的**v6.0.0**，不依赖PSRAM。当前固件只交付工程基础、启动协调和固定池，AP/socket/SPI完整功能尚未实现；编译成功不代表桥接可用。当前未授权烧录、push、合并或修改硬件。

## 1. 从干净终端选择已有工具

在PowerShell中执行：

```powershell
Set-Location -LiteralPath 'D:\STM32H7\ESP32Bridge'
. .\scripts\activate.ps1
```

这个入口显式选择本机已安装的工具，不下载、不安装、不修改SDK。它从指定SDK的`tools/tools.json`读取推荐版本，选择Xtensa编译器、CMake、Ninja及既有IDF Python，再实际检查版本和Python依赖。默认工具根为`C:\Espressif\tools`，Python环境为`C:\Espressif\tools\python\v6.0\venv`。如既有工具位于别处，可显式传入`-ToolsRoot`、`-PythonEnv`；SDK路径和6.0.0约束保持不变。

已经激活其他`IDF_PATH`或IDF Python时会拒绝，请打开干净终端；不会悄悄切换其他项目环境。校验失败时恢复此前变量，包括原本未设置的变量。脚本不承诺完整GDB/ROM ELF调试工具可用：本机官方全量`export.ps1`曾有布局/工具识别限制，记录见各组验证报告；本入口只选择并验证本目标构建所需工具。

本次实用组合为Python3.11.15、CMake4.0.3、Ninja1.12.1、Xtensa`esp-15.2.0_20251204`。`build.ps1`仍会重新校验实际环境，不把一次选择视作永久有效。

## 2. 在本机填写密码

用编辑器创建本工程`.local/sdkconfig.local`。只填写一条Kconfig字符串赋值：键为`CONFIG_BRIDGE_AP_PASSWORD`，等号后为双引号包围的本人本机密码。文件为无BOM的ASCII；密码解码后8～63字节、无NUL，允许ASCII注释与空行。转义与限制见 [无密码配置示例](../config/local-config.example.md)。

不要把密码写进命令行、聊天、README或代码。缺失、格式非法、非ASCII、重复项或混入其他配置项均失败，不生成开放热点配置。当前仓库源工程未填写部署密码；验证使用的是`.local/`内一次性夹具，不能当作部署配置。

目标已由工程锁定，无需运行`idf.py set-target`。正式入口每次创建新的构建工作区，只使用`sdkconfig.defaults`与本地密码；不复制源目录中旧的生成sdkconfig或构建缓存。CMake还会检查最终配置为esp32s3、双核、禁用PSRAM及密码一致。

## 3. 正式构建入口

```powershell
& .\scripts\build.ps1
```

默认在本工程`.local/build-work/run-<唯一编号>/`内构建，该目录必须不含任何空白字符。源工程位于含空格路径时，显式指定独立的无空格工作根，例如本仓库下的忽略目录：

```powershell
& 'D:\STM32H7\ESP32Bridge\.local\source with spaces\scripts\build.ps1' `
    -WorkRoot 'D:\STM32H7\ESP32Bridge\.local\build-work'
```

上例中的含空格源目录仅为路径用法示意，不声称该示例目录已经存在。本组真实含空格路径记录见 [验证报告](validation/phase-01-group-3.md)。保留成功工作区可追加`-KeepWork`，便于本地定位；失败时自动保留已创建的工作区，无需追加。

脚本只复制根CMake、sdkconfig.defaults、main、scripts及必需本地密码配置，不递归复制仓库根，不带.git、其他目录、既有产物或测试目录。原工程不搬移。它先运行真正的`idf.py -B build build`，再核对生成配置和源码依赖，然后运行`idf.py -B build size`，任何阶段失败均返回非零。

## 4. 产物与诊断

成功后打印`artifacts/<唯一编号>/`位置；该目录被忽略，包含：

| 文件 | 用途 |
|---|---|
| `esp32bridge.bin`、`esp32bridge.elf`、`esp32bridge.map` | 当前应用、调试和链接布局 |
| `bootloader/bootloader.bin`、`partition_table/partition-table.bin` | SDK本次生成的配套镜像 |
| `flasher_args.json`、`flash_args` | 与回收目录内相对路径一致的烧录参数，仅留档 |
| `size-report.txt`、`build.log` | 实际大小报告及脱敏构建诊断 |
| `environment.json`、`manifest.json` | SDK提交/是否有改动、实际工具/路径、源文件哈希、阶段/退出码及产物大小/SHA256 |

未回收sdkconfig、sdkconfig.h、完整缓存、compile_commands或整个工作区。BIN/ELF含本地固件配置，仍属本地私有产物，不因日志脱敏而可以公开；不提交产物或密码。manifest只有`status=success`且进程退出0才代表本次构建和回收成功。部分回收失败会留下`status=failed`，不能拿残留BIN冒充成功。

原始脱敏诊断及manifest另保存在`.local/build-diagnostics/<唯一编号>/`。成功默认仅删除本次工作区，保留产物和诊断；失败保留工作区。清理前核对绝对路径、唯一编号、所有权标记、源目录边界和所有重解析点；不会递归删除工作根、源工程或其他运行目录。检测到junction/symlink时拒绝复制/清理并保留现场。

## 5. 失败排查

| 阶段或提示 | 检查方式 |
|---|---|
| `preflight` / 密码缺失或非法 | 用编辑器核对本地文件及ASCII格式，诊断不会回显值；无需发送密码 |
| 环境、版本或工具拒绝 | 从干净终端重新选择本机已有v6.0工具，核对SDK路径与推荐工具；不自动安装或改用v5.5.5 |
| `prepare` / WorkRoot有空白 | 指定独立绝对无空格工作根；源路径可以有空格 |
| `copy` / 权限、文件缺失或重解析点 | 检查本工程白名单文件与路径；不要绕过junction检查搬移整个仓库 |
| `build` / CMake或编译失败 | 查看对应脱敏build.log及保留工作区；进程非零，不宣称生成成功 |
| `configuration` | 检查target、PSRAM/单核及密码一致性；不发布旧缓存产生的镜像 |
| `size` / 大小报告失败 | 保留工作区与诊断，恢复工具后重新正式构建 |
| `artifacts` / 产物或Flash元数据不一致 | 检查本次构建的规定文件，拒绝外部路径和不完整回收 |
| `cleanup` | 不手工扩大删除范围；检查所有权标记/重解析点，保留现场定位 |

## 6. 回归与实机边界

在已核验的环境、工程根目录运行：

```powershell
python -m unittest discover -s tests -v
```

生产C主机检查另需现有本机目标Clang/LLD，默认路径见`tests/test_foundation.py`，可用`BRIDGE_HOST_CLANG`、`BRIDGE_HOST_LLD`显式选择；测试不会安装工具。主机固定池/启动替身、文件系统错误注入、目标构建和实机测量分别记账。

本阶段不执行烧录。SDK默认Flash参数不代表已核实实板容量；串口、供电、接线、Flash容量和烧录授权须在后续交付时核对。尚无实板启动、内部堆、最低余量、任务栈、DMA/READY/SCK或吞吐测量；不能用size/map或主机成功代替这些结果。
