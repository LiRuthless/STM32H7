# Phase 03 Group 1：真实驱动与启动资源

2026-11-05职责迁移：现行/目标契约见[F04 SPI事务与协议适配](../../../Docs/transport/spi.md#f04)。本页保留原动机、候选、用户决定、提交及验证声明的冻结原文，不继续维护现行详细接口；原“待确认/已完成”等只对应记录时点，当前冲突见[问题页](../../../Docs/known-issues.md)。

<details>
<summary>原设计、决定及阶段验证（冻结原文，不作为现行规则）</summary>

~~~~~markdown
# Phase 03 Group 1：真实驱动与启动资源

确认记录：用户已明确选择“确认资源增量，纯规约提交并实施 Group 1”；按本文件资源/错误/启动范围实施，完成后仍交审。

2026-10-18实施前设计。用户已确认Phase 02本端软件及推进Phase 03独立实现；本增量新增任务栈资源，先交用户确认并作纯规约提交，再改代码。阶段分组/实机待验不变。

## requirements

- 03/FR-1、FR-2、TC-1：`main/spi_bridge.c/.h`实现实际SPI2_HOST驱动初始化，SPI_DMA_CH_AUTO；Mode0、CS10/SCK12/MOSI11/MISO13，WP/HD及扩展引脚均-1，queue_size=1，不调用EN。固定`spi_slave_transaction_t`长度8192bit，TX/RX取已有内部DMA池，禁用自动重分配标记。
- 新增静态Core1任务，优先级12，栈6144B（ESP-IDF任务参数按字节，使用StackType_t数组并断言字节数），另有静态TCB/启动信号量及小状态快照；不占业务队列，不依赖PSRAM，不在收发路径重复分配。驱动本身的SDK启动分配单独记账。
- `bridge_spi_start`由真实启动SPI回调调用；任务在Core1内检查实际核号后初始化驱动，报告真实返回值，启动调用者等待该初始化结果。驱动初始化失败归还DMA；任务创建失败不报告服务成功。启动信号只表示驱动就绪，不表示事务READY/协议握手。
- 本组初始化后保持READY高、不提交任何事务，任务阻塞等待；事务提交/回收及回调由Group 2实施，不以初始化成功冒充收发服务。app_main日志必须明确SPI驱动已初始化、事务循环未交付。网络仍未实现，网络失败不阻塞SPI初始化。
- `CONFIG_SPI_SLAVE_ISR_IN_IRAM=y`与`CONFIG_GPIO_CTRL_FUNC_IN_IRAM=y`显式锁定供下一组回调；Group 1不发布READY。SPI初始化的中断标记ESP_INTR_FLAG_IRAM，同核调用；SDK源码说明ISR在初始化核，真实亲和性仍需实测。

## plan

新增spi_bridge.c/.h；startup_idf.c将SPI回调由NULL替换为真实bridge_spi_start；app_main.c打印真实驱动状态，删除“全部SPI未实现”的误导文字，但明确事务/网络待后续组。sdkconfig.defaults和CMake接入。现有启动替身按SPI已接入事实更新，单独驱动替身直接编译同一spi_bridge.c与真实buffer_pool.c，验证驱动配置/任务参数/错误回收。

## validation

03/V-1/V-2/V-7软件子项：直接测试生产C适配；检查全部引脚、Mode0、8192bit、DMA固定缓冲/所有权、Core1/优先级12/6144B栈、驱动仅同核初始化一次。注入任务创建失败、错误核号、DMA取得失败、driver初始化失败；无任务/驱动错误被固定成功掩盖。测试不得把主机指针当真实DMA可达证据。

运行全回归及v6.0.0真实目标build/size，记录新增静态栈/目标符号及startup接入。实板核号/ISR亲和性/DMA能力、堆/栈高水位全部待验。Group 1通过后先交审再提交推进Group 2，不把整个Phase 03勾选完成。
~~~~~

</details>
