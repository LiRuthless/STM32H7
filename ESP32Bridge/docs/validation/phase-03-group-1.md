# Phase 03 Group 1：真实 SPI 初始化与生产启动接入

后续授权更新：用户明确改为“完成一个Phase审查一次”，阶段内不再逐组等待人工确认。按已交付证据核对后提交本组，连续完成Phase 03其余组，阶段末统一交审；下文“待组审”为历史交付状态。

2026-10-18，分支`codex/03-spi-slave`。用户已确认Phase 02本端软件及推进Phase 03，并明确确认6144B静态任务/启动资源；先纯规约提交`9ea2351`，后实施。本组待审查提交。

## 代码差异

- [spi_bridge.c/.h](../../main/spi_bridge.c)：静态Core1/优先级12任务，在Core1调用真实`spi_slave_initialize`；SPI2_HOST、SPI_DMA_CH_AUTO、Mode0、CS10/SCK12/MOSI11/MISO13、所有未用扩展脚-1、queue_size=1。使用已有固定DMA池，静态8192bit描述，无自动重分配标记。任务/信号量创建失败、错误核号、DMA不可得、驱动初始化失败均返回真实错误；初始化失败归还DMA。
- [startup_idf.c](../../main/startup_idf.c)：SPI回调由NULL换成`bridge_spi_start`，真实路径为app_main→startup_idf→SPI静态任务→驱动初始化；网络依赖失败不阻止SPI。
- [app_main.c](../../main/app_main.c) / startup.h：日志明确“驱动初始化”与“事务服务”区别。**本组保持READY高，不排队任何事务，成功任务阻塞等待；收发/READY回调留Group 2，协议接入留Group 3。当前不是可用桥接固件。**
- sdkconfig.defaults显式启用SPI_SLAVE_ISR_IN_IRAM/GPIO_CTRL_FUNC_IN_IRAM，CMake真实编译模块。SDK驱动启动分配不算数据路径重复分配，但真实堆仍需实机测量。
- [.gitattributes](../../.gitattributes)：仅锁定固定候选JSON为LF，修复core.autocrlf=true下重新检出破坏文件SHA的风险；新增Git真实checkout过滤字节回归。帧及文件既有SHA256未变。

## 本次实际验证

```powershell
# 仓库根目录
. .\ESP32Bridge\scripts\activate.ps1
python -m unittest discover -s ESP32Bridge/tests -p test_spi_driver.py -v
python -m unittest discover -s ESP32Bridge/tests -v
& .\ESP32Bridge\.local\p3-g1-target\scripts\build.ps1 `
  -WorkRoot 'D:\STM32H7\ESP32Bridge\.local\p3w' -KeepWork
```

| 项目 | 本次结果 | 限制/证据 |
|---|---|---|
| 新SPI适配 | 8项通过 | 直接编译生产spi_bridge.c和buffer_pool.c；SDK任务/驱动边界替身检查全部引脚/核/栈/描述/失败回收；不是RTOS或电气测量 |
| 启动及池 | 43项通过 | 既有startup真实接线用例更新为已接入SPI回调；该夹具只模拟SPI回调，实际适配另由上述8项验证 |
| 全量回归 | **154项通过，0失败/0跳过** | `.local/p3-g1-regression.log`；145+8+1个Git换行回归 |
| 真实目标 | **v6.0.0/esp32s3 build与size退出0** | run_id=`f5a82cb7324346e6948352818002a428`；当前源码白名单隔离副本，一次性私有测试配置，部署密码未提供 |
| ELF接入 | bridge_spi_start、spi_task、spi_slave_initialize均在最终ELF | 通过现有Xtensa nm实际核对，不只是归档未调用代码 |
| 静态资源 | s_stack=6144B、StaticTask=100B、StaticSemaphore=84B、描述=24B | 是目标ELF符号大小；实际任务栈高水位/内部堆未测 |
| size | image293989B，DIRAM62367B、.data12828B、.bss12016B | 链接静态占用，不等于运行时可用堆或最低余量 |
| 实机 | **未执行** | 未烧录，无DMA可达/核亲和性/READY波形/SCK/栈高水位/运行堆/网络/吞吐实测 |

环境仍为SDK commit `662a3be354759d9487bf4b1a629fadb766cb1800`、dirty=false；Python3.11.15/CMake4.0.3/Ninja1.12.1/Xtensa esp-15.2.0_20251204。本次实际生成配置确认目标esp32s3及两个IRAM选项均=y；未换SDK、未跳过检查。[证据与产物哈希](phase-03-group-1-evidence.json)给出文件清单、源码核对及本地索引；产物本体/密码不提交。

## 只读取证与未完成项

本轮用SmartCar原头文件主机静态断言核对全部24项96B记录字段及29项256B头字段偏移/两项尺寸，编译通过；不修改SmartCar，不是STM32目标构建或SCB能力证明。源码查找未见SCB1/HELLO_ACK/SUBSCRIBE/LOG_BEGIN等处理，现有SPI2仅预留初始化；G-05真实能力/联调继续待验。

Group 2事务所有权/READY/截短/回收重排、Group 3握手及可靠协议真实接入，以及Phase 04～06均未完成。最终客户端、夹具、60秒负载和完整中文实机手册仍须按最终实现交付，不能用本组初始化代替。Phase 03总验收不勾选完成。

拟提交：`feat(esp32bridge): 接入 Phase 03 Group 1 真实 SPI 初始化`。核心文件清单：main/spi_bridge.c/.h、main/startup_idf.c、main/startup.h、main/app_main.c、main/CMakeLists.txt、sdkconfig.defaults、tests/host/spi_driver_harness.c及4个SDK边界头、tests/test_spi_driver.py、现有foundation夹具/测试、.gitattributes和tests/test_vectors.py；配套报告/规约状态见证据完整清单。本组实现未暂存、未提交，确认后提交并推进Group 2。
