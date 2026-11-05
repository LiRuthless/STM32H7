# 源码格式与注释整改：本轮预检

日期：2026-11-05。状态：清点与规约草案已准备，工程基线导入范围待用户确认。未改任何C/H、脚本或工程配置，未暂存、提交、构建或运行行为测试。

## Git事实及门禁

`git ls-files -- ESP32Bridge`为空；`git status --short --untracked-files=all -- ESP32Bridge`列出整个工程未跟踪。当前分支chore/constitution-documentation-alignment，暂存区为空。无法用本仓提交还原当前源码，因此先保存获准的工程基线，再独立提交规约，才有可审查的整改差异。不得将工程源码混入纯规约提交。

仓库已有其他在途项：Hardware/STM32H7_Car下edge.db、kicad_pcb/prl/pro修改；.vscode/settings.json及Reports/firmware-log-review_20261105未跟踪。本轮不处理这些路径。

用户本次明确要求：整体未跟踪时“先完成清点和规约草案……向我确认导入范围”。ESP32Bridge/AGENTS.md同样要求独立纯规约提交后实施；其旧“本次只整理文档”对应前次工作，不是本次授权。此次暂停仅为工程导入确认，不是要求逐文件批准。

## 导入建议及独立提交顺序

建议导入清点时Git列出的全部非忽略工程文本，逐路径及原始字节SHA256见[基线清单](../../specs/research/2026-11-05-source-style-baseline-manifest.md)。包括原有治理/Phase/Docs、生产源码、工程入口、脚本、测试/替身、固定向量和无密码配置说明；历史Markdown/JSON报告及来源Markdown附件按原字节保留，维持现有链接和历史身份，不将其作为本轮验证证据。

不导入忽略目录.local、artifacts、build/build-*、managed_components、生成sdkconfig、缓存、真实密码、BIN/ELF/map、数据库、外部SDK及其他目录。清单不含这些项；不使用强制暂存。清单生成之后新建的本轮草案、清单与本预检属于下一次纯规约提交准备，不混入原工程基线。若原文件在确认前发生变化，重新核对清单并展示差异，不能静默接受新的范围。

确认后的顺序：①单独导入清单中的原工程基线；②落实[规约草案](../../specs/research/2026-11-05-source-style-remediation-draft.md)的Phase/Docs增量并纯规约提交；③按G1～G4整改、审查及适用构建；④回填结果并按组本地提交。每次提交前展示文件清单、摘要及中文提交信息；不push、不合并。

## 已完成的只读核对与限制

- 已读取根/ESP32Bridge规则、mission/techstack/roadmap/code-style、Docs入口/架构/追溯/问题页及相关功能契约，参考SmartCar现行2026-10-31验证分类，未移植其工具链或R08。
- 清点main与tests/host每个C/H的行数、文件说明、同排大括号候选及100字符候选；下表只是差距定位，不是格式全项或逐定义注释验收。超长候选含允许保留的协议/日志字符串，不能一概计为违规。头文件无同排函数括号候选不证明结构体格式合格。
- 初步核对startup→pool/NET/SPI启动、pool DMA状态接口、SPI事务循环及协议reserve/commit/cancel、可靠窗口、NET拆包/短写/连接清理及宿主绑定调用。生产/夹具的逐函数准确性审查仍待整改后完成。

具体上下文事实：SPI在pool submit后queue，超时分支仅poll逻辑及快照，原描述返回成功后reclaim，之后先报告TX完成再处理RX；错误或陌生描述不能证明DMA回收。READY回调只做GPIO电平，保持ISR边界。可靠层只有pending且sent时的精确ACK释放TX，完成发送才记录last_send_ms并计算重发次数。NET部分发送只有cursor到length才调用scb_link_sent；close_control清订阅/待发，session_check只关闭NET连接而不重置SPI，UDP消费检查generation。以上是后续注释必须保留和说明的当前事实，不是本轮完整逻辑审查或硬件通过结论。

另有需在注释审查时准确区分的输出语义：pool_dma_acquire/claim在失败时不统一清空输出；scb_link_receive先清空可用out，再检查link状态；startup_run无效参数返回前不初始化report。不能将这些不同路径统一写成“失败清零”。此处不修改行为。

## 当前构建环境预检

现有activate.ps1固定D:\esp\v6.0\esp-idf，默认工具C:\Espressif\tools和PythonEnv C:\Espressif\tools\python\v6.0\venv；Test-Path确认SDK tools/tools.json、工具根、Python解释器均不存在。IDF_PATH和IDF_PYTHON_ENV_PATH在本进程未设置。.local/sdkconfig.local不存在（只检查存在性，不读取密码）。

通用python命令解析至WindowsApps别名，进程无法启动；清点改用PowerShell完成，没有安装解释器、激活其他SDK或改脚本。以上不足以断言机器其他位置没有SDK，但说明现有锁定入口当前不可直接使用。固件构建未执行、零错误零警告目标未验证；不借历史结果填通过。基线确认不自动解决该环境问题。

## 剩余状态

EB-03保持打开；47个C/H尚未整改，未作覆盖/格式/内容/行为全量验收。Phase01～04阶段验收、Phase05/06门禁、R-05冲突、导出计时歧义、缺失桥接模块及所有实机待验项保持原状。按需行为测试暂未执行：目前只做准备，没有实现差异；不触发全量软件回归。后续选取原则与强制审查见规约草案S1～S7。

准备文档检查：本次三个新增Markdown的相对文件链接均能解析到实际文件，未发现行尾空白；本次引用的F01～F05/F09及EB-03锚点按已读功能页核对。重新计算清单157个原文件的SHA256，均与清点值一致。暂存区仍为空，全仓状态中的其他在途项保持原状。此结果仅限本轮准备文档，不代表原工程所有历史链接或源码合规通过。

## 逐文件初筛清单

下表由实际文件读取生成。缺@file表示尚无标准要求的文件块，不据此判断所有函数均无注释。实际整改时仍须逐定义结合调用方检查。

| 文件 | 行数 | 超100字符候选数/首行 | 同排函数或控制大括号首行 | @file |
|---|---:|---|---|---|
| [main/app_main.c](../../main/app_main.c) | 61 | 8 / 23 | 29 | False |
| [main/buffer_pool.c](../../main/buffer_pool.c) | 240 | 9 / 23 | 27 | False |
| [main/buffer_pool.h](../../main/buffer_pool.h) | 55 | 0 /  |  | False |
| [main/json_guard.c](../../main/json_guard.c) | 322 | 4 / 150 | 6 | False |
| [main/json_guard.h](../../main/json_guard.h) | 41 | 0 /  |  | False |
| [main/link_session.c](../../main/link_session.c) | 150 | 8 / 11 | 5 | False |
| [main/link_session.h](../../main/link_session.h) | 52 | 1 / 35 |  | False |
| [main/net_bridge.c](../../main/net_bridge.c) | 637 | 23 / 61 | 60 | False |
| [main/net_bridge.h](../../main/net_bridge.h) | 63 | 2 / 25 |  | False |
| [main/protocol.c](../../main/protocol.c) | 150 | 12 / 7 | 40 | False |
| [main/protocol.h](../../main/protocol.h) | 49 | 0 /  |  | False |
| [main/reliable_link.c](../../main/reliable_link.c) | 91 | 4 / 27 | 6 | False |
| [main/reliable_link.h](../../main/reliable_link.h) | 47 | 0 /  |  | False |
| [main/spi_bridge.c](../../main/spi_bridge.c) | 201 | 12 / 78 | 41 | False |
| [main/spi_bridge.h](../../main/spi_bridge.h) | 23 | 0 /  |  | False |
| [main/spi_protocol.c](../../main/spi_protocol.c) | 139 | 18 / 5 | 11 | False |
| [main/spi_protocol.h](../../main/spi_protocol.h) | 50 | 3 / 46 |  | False |
| [main/startup_idf.c](../../main/startup_idf.c) | 88 | 0 /  | 17 | False |
| [main/startup.c](../../main/startup.c) | 60 | 5 / 23 | 6 | False |
| [main/startup.h](../../main/startup.h) | 34 | 1 / 12 |  | False |
| [tests/host/foundation_harness.c](../../tests/host/foundation_harness.c) | 507 | 25 / 108 | 54 | False |
| [tests/host/net_harness.c](../../tests/host/net_harness.c) | 563 | 28 / 65 | 65 | False |
| [tests/host/spi_driver_harness.c](../../tests/host/spi_driver_harness.c) | 196 | 40 / 9 | 26 | False |
| [tests/host/include/esp_attr.h](../../tests/host/include/esp_attr.h) | 3 | 0 /  |  | False |
| [tests/host/include/esp_err.h](../../tests/host/include/esp_err.h) | 10 | 0 /  |  | False |
| [tests/host/include/esp_event.h](../../tests/host/include/esp_event.h) | 2 | 0 /  |  | False |
| [tests/host/include/esp_heap_caps.h](../../tests/host/include/esp_heap_caps.h) | 2 | 0 /  |  | False |
| [tests/host/include/esp_intr_alloc.h](../../tests/host/include/esp_intr_alloc.h) | 2 | 0 /  |  | False |
| [tests/host/include/esp_log.h](../../tests/host/include/esp_log.h) | 5 | 0 /  |  | False |
| [tests/host/include/esp_mac.h](../../tests/host/include/esp_mac.h) | 4 | 0 /  |  | False |
| [tests/host/include/esp_memory_utils.h](../../tests/host/include/esp_memory_utils.h) | 2 | 0 /  |  | False |
| [tests/host/include/esp_netif.h](../../tests/host/include/esp_netif.h) | 12 | 0 /  |  | False |
| [tests/host/include/esp_random.h](../../tests/host/include/esp_random.h) | 3 | 0 /  |  | False |
| [tests/host/include/esp_timer.h](../../tests/host/include/esp_timer.h) | 3 | 0 /  |  | False |
| [tests/host/include/esp_wifi_default.h](../../tests/host/include/esp_wifi_default.h) | 5 | 0 /  |  | False |
| [tests/host/include/esp_wifi.h](../../tests/host/include/esp_wifi.h) | 29 | 1 / 10 | 7 | False |
| [tests/host/include/nvs_flash.h](../../tests/host/include/nvs_flash.h) | 2 | 0 /  |  | False |
| [tests/host/include/string.h](../../tests/host/include/string.h) | 4 | 0 /  |  | False |
| [tests/host/include/test_sdk.h](../../tests/host/include/test_sdk.h) | 30 | 1 / 15 |  | False |
| [tests/host/include/driver/gpio.h](../../tests/host/include/driver/gpio.h) | 2 | 0 /  |  | False |
| [tests/host/include/driver/spi_slave.h](../../tests/host/include/driver/spi_slave.h) | 27 | 3 / 25 |  | False |
| [tests/host/include/freertos/FreeRTOS.h](../../tests/host/include/freertos/FreeRTOS.h) | 2 | 0 /  |  | False |
| [tests/host/include/freertos/semphr.h](../../tests/host/include/freertos/semphr.h) | 7 | 0 /  |  | False |
| [tests/host/include/freertos/task.h](../../tests/host/include/freertos/task.h) | 17 | 1 / 12 |  | False |
| [tests/host/include/hal/gpio_ll.h](../../tests/host/include/hal/gpio_ll.h) | 5 | 0 /  |  | False |
| [tests/host/include/lwip/sockets.h](../../tests/host/include/lwip/sockets.h) | 54 | 1 / 51 |  | False |
| [tests/host/include/soc/gpio_struct.h](../../tests/host/include/soc/gpio_struct.h) | 3 | 0 /  |  | False |

## 用户确认后的准备进展

用户已明确确认157文件清单导入并连续整改；原工程独立基线35e56277f1842bc5509fa583c37103b4532f930d。上方待确认和未提交状态保留为预检时点事实。本次已补齐Phase01～04三件套及Docs入口增量，待纯规约提交后开始源码整改。
