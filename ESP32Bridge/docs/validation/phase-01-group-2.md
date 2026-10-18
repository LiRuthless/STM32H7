# Phase 01 Group 2 实施与验证

记录日期：2026-10-18（Asia/Shanghai，承接10月17日授权跨日实施；产物时间戳见清单）。用户已明确确认Group 2增量，纯规约先独立提交为 **73a2a2f**，代码自该提交后开始修改；分支`codex/01-project-foundation`。本组实现及验证完成，等待用户审查确认提交。Group 3与阶段02～06未实施，Phase 01未完成，G-01～G-03未解除。

## 实际改动

| 需求 / 验证 | 文件与符号 | 本组结果 |
|---|---|---|
| FR-2 / V-2 | [startup.c](../../main/startup.c) `bridge_startup_run`、[startup.h](../../main/startup.h) | 按NVS/netif/event/AP/socket/SPI顺序处理，逐步报告成功、原始错误、依赖阻塞或未实现；网络失败不屏蔽独立SPI尝试 |
| FR-2、FR-3 / V-2、V-3 | [startup_idf.c](../../main/startup_idf.c) `bridge_startup_idf_run`、`pool_alloc`；[app_main](../../main/app_main.c) | 实际READY高电平保护、编译密码校验、完整池初始化及真实SDK NVS/netif/event调用；输出分步状态及真实堆诊断入口，不自动擦NVS、不重启、不操作EN |
| FR-3 / V-3 | [startup.c](../../main/startup.c) `bridge_password_validate` | 按显式长度拒绝NULL、长度越界、NUL及非ASCII；非法配置只阻塞AP/socket，SPI独立 |
| FR-4、TC-2 / V-4、V-8 | [buffer_pool.c](../../main/buffer_pool.c)、[接口](../../main/buffer_pool.h) `bridge_pool_*` | 启动四次分配，全部成功后发布；失败回收，固定容量、代际凭据、峰值、DMA所有权及忙池拒绝销毁 |
| 回归 / V-1、V-7、V-9 | [组件依赖](../../main/CMakeLists.txt)、原配置检查、[本组测试](../../tests/test_foundation.py) | 生产C测试加Group 1回归；v6.0.0目标构建及size通过，忽略规则与脱敏检查单列 |

数据区为2×1024B DMA（64B对齐）、16×1016B UDP、32×1024B EVENT，共51072B请求容量。IDF分配器检查两端内部内存/DMA能力，核心层检查对齐、非重叠及部分失败。数据路径的claim/release和DMA转换不分配内存。元数据通过FreeRTOS短临界区保护，分配、释放、完整帧处理不在锁内执行。初始化/销毁要求服务停止且无并发调用。

DMA API执行`IDLE → APPLICATION → DRIVER → APPLICATION → IDLE`；驱动持有时拒绝二次借用、归还及销毁。C裸指针不是硬件访问隔离：调用者仍必须遵守持有期间不读写的契约；阶段03实际驱动接入后才能证明回收时机，不能凭逻辑超时调用reclaim。

AP/socket/SPI回调当前确实缺失，报告`UNIMPLEMENTED`或前置依赖`BLOCKED`，没有固定成功桩。FIFO、UDP丢旧、可靠ACK、查询/最终结果保留槽、协议与SPI驱动仍属后续阶段。本组没有为验证而启动假服务。

## 实际环境与命令

目标SDK：`D:\esp\v6.0\esp-idf`，源码声明6.0.0，Git `662a3be354759d9487bf4b1a629fadb766cb1800`；本次`git status --short`含子模块检查为空。Python3.11.15、CMake4.0.3、Ninja1.12.1、Xtensa GCC15.2.0 / `esp-15.2.0_20251204`，构建输入检查实际通过且匹配SDK推荐版本。没有安装或修改SDK。

主机C：现有MATLAB附带Clang18.1.7，路径`C:\Program Files\MATLAB\R2025a\toolbox\sldrt\clang\win64\clang.exe`；现有LLD20.1.1，路径`C:\Espressif\tools\esp-clang\esp-20.1.1_20250829\esp-clang\bin\ld.lld.exe`。只生成Windows本机测试DLL，不参与ESP32目标构建。测试支持`BRIDGE_HOST_CLANG`、`BRIDGE_HOST_LLD`显式选择既有工具，不自动安装。

首轮选用ESP-Clang20.1.1时真实失败：仅注册riscv32/riscv64/xtensa，不能生成x86_64。失败日志保留于`.local/g2-foundation-first.log`，随后核对现有Clang18.1.7本机目标支持后成功。未将首轮失败记成通过，也未替换目标工具链。官方全量`export.ps1`的既有工具识别限制见Group 1报告；本轮使用相同、已核验的EIM路径及本工程环境检查，不重新宣称全量export通过。

本次实际命令：

```powershell
# 仓库根目录；此辅助文件只选择已安装的v6.0工具路径，属忽略的本机证据。
. .\ESP32Bridge\.local\activate-verified.ps1
python -m unittest discover -s ESP32Bridge/tests -v
# 白名单复制本工程至.local/g2-project，并放入一次性本地测试配置后：
Set-Location D:\STM32H7\ESP32Bridge\.local\g2-project
python D:\esp\v6.0\esp-idf\tools\idf.py -B build build
python D:\esp\v6.0\esp-idf\tools\idf.py -B build size
```

构建辅助文件`.local/run-group2-target.ps1`仅用于本组手工复制与命令复现，不能当作Group 3的复制/产物回收脚本验收。复制白名单为根CMake、sdkconfig.defaults、main、scripts及一次性配置，无.git、其他目录源码或仓库搬移。

## 分类别结果

| 类别 | 本次执行结果 | 证据与限制 |
|---|---|---|
| 文档静态 | 文件边界、链接、需求覆盖、差异空白、规约在实现前提交、协议原文未改，见清单 | [脱敏清单](phase-01-group-2-evidence.json)；不是运行测试 |
| 主机 | **59项通过，0失败、0跳过**：16项Group 1回归＋43项生产C检查 | `.local/g2-host-tests.log`；没有SCB对端或网络模拟完成的含义 |
| ESP32目标 | `build`与`size`均退出0；实际生成BIN、ELF、map、bootloader、partition table及烧录参数 | `.local/g2-target-build.log`、`.local/g2-target-size.log`、`.local/g2-project/build/log/`；目标esp32s3、双核、禁用PSRAM |
| 实机 | **未执行** | 未烧录、未上电读取、未测内部堆/最低余量/任务栈/DMA/READY/SCK/吞吐 |

43项生产C测试由 [foundation_harness.c](../../tests/host/foundation_harness.c) 调用实际startup/适配/app_main代码，并直接编译原buffer_pool.c。池C包含在测试翻译单元中，仅为将私有代际计数置于UINT64_MAX作边界注入，不给固件增加测试接口。SDK/内存分配/GPIO替身仅存在于tests/host；未用Python重写池或启动算法。

覆盖：四次分配逐点失败、所有块对齐异常、重复地址回收、内部/DMA范围端点错误、16/32槽全满及峰值、重复/伪造/重建后过期句柄、驱动所有权下拒绝销毁、代际耗尽不回绕、2000次收发资源循环无额外分配、逐个启动步骤失败、缺失回调、密码/READY/池前置失败、多个错误保留首错，以及真实app_main诊断不含一次性测试密码。密码C测试覆盖全部256种字节值及长度边界。测试编译参数包含`-Wall -Wextra -Werror`。

本次链接报告：BIN272800B，image size272677B，DIRAM52967B（`.data`12684B、`.bss`5600B），IRAM16384B。`nm -S`中`s_pool`为0x248＝584B；snapshot的metadata计数是C模型容量（含代际计数），不能替代map和平台锁/分配器开销。当前未使用的池API可能被链接器回收；它们已编译并在主机直接执行，后续服务接入后链接占用需重新核对。51072B数据区在启动时申请，不计入静态BSS，不能把链接余量当作启动后可用堆。

SDK依赖闭包可含esp_psram目录，最终CONFIG_SPIRAM禁用；未要求外部RAM。SDK默认2MB Flash参数只是构建配置，不是实板容量证据。

## 配置、文件与提交审查

本轮仅使用被忽略的一次性目标测试配置及随机主机夹具。源工程`.local/sdkconfig.local`仍不存在，部署密码未提供；测试值不写入代码、文档、日志或清单。生成配置、DLL、固件及缓存均本地忽略，编译命令和产物SHA256见 [清单](phase-01-group-2-evidence.json)。全部本组待审文件列在清单`changed_files`，与其他任务的Reports文件分开；未暂存实现、未提交实现，不push、不合并、不烧录。

建议实现提交信息：`feat(esp32bridge): 完成 Phase 01 Group 2 启动与固定资源`。等待用户审查确认后按明确文件清单暂存、检查差异并提交，再推进Group 3；当前不标记Phase 01完成，不启动阶段02～06。
