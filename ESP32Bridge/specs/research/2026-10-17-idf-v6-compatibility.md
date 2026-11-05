# 调研：本机 ESP-IDF v6.0.0 与任务书接口核对

- 日期：2026-10-17
- 状态：已采纳为文档技术基线；目标构建未执行。

## 本机观察

| 核对对象 | 只读结果 | 结论边界 |
|---|---|---|
| `D:\esp\v6.0\esp-idf\tools\cmake\version.cmake` | major=6、minor=0、patch=0 | 已确认源码声明v6.0.0，未激活环境、未运行idf.py |
| 同机 `D:\esp\v5.5.5\esp-idf` | 声明5.5.5 | 不选作本工程SDK |
| v6.0.0 `components/esp_driver_spi/include/driver/spi_slave.h` | 存在initialize / queue_trans / get_trans_result和post_setup_cb / post_trans_cb | 任务书建议的驱动入口在本机头文件可见，尚无编译或时序证据 |
| 同版 `spi_common.h` | 存在SPI_DMA_CH_AUTO、isr_cpu_id及扩展引脚字段 | 实施须绑定SPI核并明确未使用引脚-1 |
| `C:\Espressif\tools` | 发现Xtensa、CMake、Ninja及Python工具文件 | 文件存在不等于组合可用，后续由SDK导出环境选择并验证 |
| v6.0.0 SoftAP / tcp_server示例 | 可供API与部分发送处理对照 | 示例不是完整SCB/1实现，不能直接当作验收结果 |

## 官方依据

- [v6.0 ESP32-S3 SPI从机文档](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/peripherals/spi_slave.html)：核对任务/ISR归属、DMA内存、回收和READY握手要求。
- [v6.0 外设迁移说明](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/migration-guides/release-6.x/6.0/peripherals.html)：后续工程以新组件依赖为准，不沿用旧示例的隐式依赖假设。
- [v6.0 Windows命令行入门](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/get-started/windows-start-project.html)：后续环境、目标、构建和烧录步骤的版本依据。
- 本机同版本示例路径：`examples/wifi/getting_started/softAP/main/softap_example_main.c`、`examples/protocols/sockets/tcp_server/main/tcp_server.c`，均相对于所选SDK。

## 结论与实现检查点

采用用户选定的v6.0.0；后续必须验证目标esp32s3、双核配置、IRAM回调及GPIO调用链、内部DMA内存、网络组件和任务优先级。不要只通过函数名存在判断已兼容。

本机SoftAP示例包含空密码回退开放热点和打印密码的逻辑；本工程禁止继承这两项行为。单TCP会话和SCB/1有界解析也不能直接由回显示例替代。

`trans_len`在主机超长时可能被驱动限制至所设事务长度，因此`trans_len==8192`不是证明主机没有多发时钟的充分条件。严格1024B还需逻辑分析仪核对CS内时钟；不改变共享线格式来规避此限制。

## 未执行项目

环境导出、`idf.py --version`、配套工具检查、目标编译、size/map、启动堆及任务栈测量、GPIO/时序、网络和吞吐测试均未执行。以上均纳入 [Phase 06](../features/06-validation-delivery/validation.md)，不得据此填写“通过”。
