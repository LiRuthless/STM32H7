# 架构与模块反查

2026-11-07后续变更：用户要求移除Python宿主测试，tests下14个test_*.py和4个*_host.py已删除；下表对应条目为历史映射，不再是当前文件。C夹具、协议向量及scripts正式工具保留，生产固件未因此改变。

2026-11-05按实际工作区组织，模块归属不依阶段数量决定。此页只登记分层、依赖及主功能，具体符号/接口表在主条目，不建立第二套。

## 生产数据流与状态

app_main → startup_idf/startup → 固定池与网络/SPI启动。SPI任务（Core1）持DMA、SPI会话和可靠实例；NET任务（Core0）持socket、订阅、流游标和独立可靠实例。codec/JSON不持全局传输状态，workspace分别归调用任务。池与快照短临界区只保护其元数据/副本；共享借出帧的生命周期由实际owner决定。

2026-11-07生产接入：NET COMMAND→bridge_state固定信用→SPI；SPI RESULT/EVENT/LOG_DATA→bridge_state→NET可靠TCP；SPI TELEMETRY→固定FIFO→NET订阅UDP。bridge_stats汇总各owner快照并由NET输出串口。接口与锁序见[桥接](bridge/forwarding-and-lifecycle.md)。最终代码未完成目标构建/阶段验收，不声明实机通过。

## 实际main模块

| 文件 | 唯一主功能 | 边界 |
|---|---|---|
| [main/app_main.c](../main/app_main.c) | [F01 启动、配置与固定池](system/startup-and-resources.md#f01) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/buffer_pool.c](../main/buffer_pool.c) | [F01 启动、配置与固定池](system/startup-and-resources.md#f01) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/buffer_pool.h](../main/buffer_pool.h) | [F01 启动、配置与固定池](system/startup-and-resources.md#f01) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/json_guard.c](../main/json_guard.c) | [F02 显式帧编解码与有界JSON](protocol/framing-and-links.md#f02) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/json_guard.h](../main/json_guard.h) | [F02 显式帧编解码与有界JSON](protocol/framing-and-links.md#f02) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/link_session.c](../main/link_session.c) | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/link_session.h](../main/link_session.h) | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/net_bridge.c](../main/net_bridge.c) | [F05 网络与单控制会话](transport/network.md#f05) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/net_bridge.h](../main/net_bridge.h) | [F05 网络与单控制会话](transport/network.md#f05) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/protocol.c](../main/protocol.c) | [F02 显式帧编解码与有界JSON](protocol/framing-and-links.md#f02) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/protocol.h](../main/protocol.h) | [F02 显式帧编解码与有界JSON](protocol/framing-and-links.md#f02) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/reliable_link.c](../main/reliable_link.c) | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/reliable_link.h](../main/reliable_link.h) | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/spi_bridge.c](../main/spi_bridge.c) | [F04 SPI事务与协议适配](transport/spi.md#f04) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/spi_bridge.h](../main/spi_bridge.h) | [F04 SPI事务与协议适配](transport/spi.md#f04) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/spi_protocol.c](../main/spi_protocol.c) | [F04 SPI事务与协议适配](transport/spi.md#f04) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/spi_protocol.h](../main/spi_protocol.h) | [F04 SPI事务与协议适配](transport/spi.md#f04) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/startup.c](../main/startup.c) | [F01 启动、配置与固定池](system/startup-and-resources.md#f01) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/startup.h](../main/startup.h) | [F01 启动、配置与固定池](system/startup-and-resources.md#f01) | 自维护；辅助符号归此模块功能，H不是另一实现 |
| [main/startup_idf.c](../main/startup_idf.c) | [F01 启动、配置与固定池](system/startup-and-resources.md#f01) | 自维护；辅助符号归此模块功能，H不是另一实现 |

## 工程、工具及测试

| 实际文件/目录 | 主功能及边界 |
|---|---|
| [scripts/activate.ps1](../scripts/activate.ps1) | [F10 工具、验证与交付](guides/validation.md#f10)；构建/环境或固定向量辅助，非目标业务实现 |
| [scripts/build.ps1](../scripts/build.ps1) | [F10 工具、验证与交付](guides/validation.md#f10)；构建/环境或固定向量辅助，非目标业务实现 |
| [scripts/build_support.py](../scripts/build_support.py) | [F10 工具、验证与交付](guides/validation.md#f10)；构建/环境或固定向量辅助，非目标业务实现 |
| [scripts/check_config.py](../scripts/check_config.py) | [F10 工具、验证与交付](guides/validation.md#f10)；构建/环境或固定向量辅助，非目标业务实现 |
| [scripts/check_vectors.py](../scripts/check_vectors.py) | [F10 工具、验证与交付](guides/validation.md#f10)；构建/环境或固定向量辅助，非目标业务实现 |
| [scripts/generate_vectors.py](../scripts/generate_vectors.py) | [F10 工具、验证与交付](guides/validation.md#f10)；构建/环境或固定向量辅助，非目标业务实现 |
| [tests/link_host.py](../tests/link_host.py) | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03)；方法/工具执行政策见F10 |
| [tests/net_host.py](../tests/net_host.py) | [F05 网络与单控制会话](transport/network.md#f05)；方法/工具执行政策见F10 |
| [tests/protocol_host.py](../tests/protocol_host.py) | [F02 显式帧编解码与有界JSON](protocol/framing-and-links.md#f02)；方法/工具执行政策见F10 |
| [tests/spi_host.py](../tests/spi_host.py) | [F04 SPI事务与协议适配](transport/spi.md#f04)；方法/工具执行政策见F10 |
| [tests/test_build_entrypoints.py](../tests/test_build_entrypoints.py) | [F10 工具、验证与交付](guides/validation.md#f10)；方法/工具执行政策见F10 |
| [tests/test_build_support.py](../tests/test_build_support.py) | [F10 工具、验证与交付](guides/validation.md#f10)；方法/工具执行政策见F10 |
| [tests/test_cmake_guards.py](../tests/test_cmake_guards.py) | [F10 工具、验证与交付](guides/validation.md#f10)；方法/工具执行政策见F10 |
| [tests/test_config.py](../tests/test_config.py) | [F10 工具、验证与交付](guides/validation.md#f10)；方法/工具执行政策见F10 |
| [tests/test_foundation.py](../tests/test_foundation.py) | [F01 启动、配置与固定池](system/startup-and-resources.md#f01)；方法/工具执行政策见F10 |
| [tests/test_network_stream.py](../tests/test_network_stream.py) | [F05 网络与单控制会话](transport/network.md#f05)；方法/工具执行政策见F10 |
| [tests/test_protocol.py](../tests/test_protocol.py) | [F02 显式帧编解码与有界JSON](protocol/framing-and-links.md#f02)；方法/工具执行政策见F10 |
| [tests/test_reliable.py](../tests/test_reliable.py) | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03)；方法/工具执行政策见F10 |
| [tests/test_sessions.py](../tests/test_sessions.py) | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03)；方法/工具执行政策见F10 |
| [tests/test_spi_driver.py](../tests/test_spi_driver.py) | [F04 SPI事务与协议适配](transport/spi.md#f04)；方法/工具执行政策见F10 |
| [tests/test_spi_lifecycle.py](../tests/test_spi_lifecycle.py) | [F04 SPI事务与协议适配](transport/spi.md#f04)；方法/工具执行政策见F10 |
| [tests/test_subscribe.py](../tests/test_subscribe.py) | [F05 网络与单控制会话](transport/network.md#f05)；方法/工具执行政策见F10 |
| [tests/test_vectors.py](../tests/test_vectors.py) | [F02 显式帧编解码与有界JSON](protocol/framing-and-links.md#f02)；方法/工具执行政策见F10 |
| [tests/host](../tests/host/) | foundation_harness归F01，spi_driver_harness归F04，net_harness归F05；SDK替身仅模拟边界，保留固定接口名 |
| [tests/vectors](../tests/vectors/) | F02固定候选字节/说明，F10独立核验及证据，不是三端通过 |
| [CMakeLists.txt](../CMakeLists.txt)、[main/CMakeLists.txt](../main/CMakeLists.txt)、[sdkconfig.defaults](../sdkconfig.defaults)、[main/Kconfig.projbuild](../main/Kconfig.projbuild) | F10工程目标/依赖及F01配置；本轮不改配置 |
| [config说明](../config/local-config.example.md)、[.gitignore](../.gitignore)、[.gitattributes](../.gitattributes) | F10本地密码/产物隔离、候选向量LF；不包含实际密码 |
| [specs/references](../specs/references/) | 历史来源、非现行规则；SDK/生成区/第三方不受自维护全量迁移 |
| [Docs/validation](validation/) | 阶段报告/原JSON；原值与采集条件保留，缺产物不补造 |

从上表进入功能后，按[traceability](traceability.md)反查Phase/FR/TC。F06～F08归main/bridge_state.c/.h，F09汇总归main/bridge_stats.c/.h；启动和NET接入分别归startup_idf与net_bridge。

2026-11-05现有main及tests/host格式/注释整改保持上述模块、数据流和owner不变；逐文件范围与行为保持证据见[本轮人工复核](validation/2026-11-05-source-style-review.md)，未新增Phase05模块。


## 当前主题反查

[固定池与DMA](system/resource-pools.md#f01-pools)对应buffer_pool；[帧检查](protocol/frame-codec.md#f02-validation)对应protocol；[JSON](protocol/json-validation.md#f02-json)对应json_guard；[可靠与会话](protocol/reliable-and-sessions.md#f03)对应reliable_link/link_session。启动、SPI、NET模块仍分别归F01/F04/F05；[参数索引](parameters.md)连接各主说明，不按每个C/H另建功能身份。
