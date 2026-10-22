# R2：SPI异常后的DMA完整回收

2026-10-22规约增量，关联FR-03/04/23。先独立提交本文件及三件套入口，再修改实现。
证据：Reports/link-validation/20261022-162357及其post-reset目录。人工RESET后仍再次失联。
本批不改变公共协议、引脚、Mode0、分频256、1024B、参数/日志布局、车辆起停或EN。

## 已确认失败机制及范围

100个运行态分时样点：SPI TX/RX DMA请求关闭，但TX Stream1 EN=1、NDTR=493固定，RX
Stream2 EN=0、NDTR=1024；成功完成不增、错误增加12。锁定HAL_SPI_Abort按SPI请求位
决定是否中止DMA；现有Service仅调用它后即发布空闲，不能证明两个流都已回收。
初次接收移位的电气/时序原因尚未证明，本修复不将其宣称为已解决。

## 可判定需求

- **R2-01**：Service仍只在既有state=3或在途满10ms时进入。先置state=3，屏蔽既有
  TX/RX DMA及SPI三个IRQ；HAL_SPI_Abort后分别检查两个DMA句柄，BUSY时显式调用
  HAL_DMA_Abort，不再依赖已关闭的SPI请求位。非BUSY不调用无意义的DMA Abort。
- **R2-02**：两个方向都必须检查，先失败不能短路跳过另一个。每个方向只有中止成功、
  最终非BUSY且实际Stream CR.EN=0才返回回收成功。SPI Abort成功及两个方向均成功后
  才发布state=0；否则保持state=3，GetTx/GetRx不得借出，TryExchange不得启动新事务。
  后续前台Service可有界重试；不以超时或句柄非BUSY单独作为回收证明。
- **R2-03**：所有退出路径清原CPU pending并恢复三个IRQ。迟到完成在state=3不能发布
  成功RX；新READY沿继续由原EXTI累计。原CS释放、permit/consumed所有权、错误计数保持；
  原10ms超时入口仅加一次timeouts，持续错误态重试不重复增加同一次超时。
- **R2-04**：全部中止沿用锁定HAL已有有界超时；只在前台执行，不屏蔽TIM15/6/7，不
  增加ISR等待或通信停车，不复位两块板。不新增公开API、状态字段或动态分配，不改
  bsp_wifi_status_t布局、初始化READY去重、正常完成及HAL_BUSY保留许可路径。

## 影响文件、符号与顺序

1. `BSP/bsp_wifi.c`新增私有`BSP_WiFi_AbortDma(DMA_HandleTypeDef *)`。句柄来自hspi2的
   TX/RX，固定为DMA1 Stream1/2；辅助函数返回1表示已确认停止、0表示失败或仍EN。
   修改Service保存SPI中止结果，独立调用两方向辅助函数，成功才退回空闲；共同收尾
   恢复IRQ。C函数按CS-11～13写完整中文契约和连续阶段解释，其他代码不整理。
2. `tests/spi_wifi_state.py`扩展既有假HAL的两个DMA句柄、Stream CR、BUSY状态及Abort
   结果，直接执行同一生产BSP；保留全部原断言及READY启动竞态负对照。
3. 规约先提交，随后实现/验证提交，最后回填validation、roadmap和CHANGELOG。共享
   检出/索引和ESP32Bridge在途改动保持；用临时索引仅纳入本批路径，提交到独立本地
   `feature/16-spi-dma-recovery`，不切换用户当前检出、不push。

## 验证与唯一通过标准

| 编号 | 需求/方法 | 通过标准 | 初始状态 |
|---|---|---|---|
| R2-V01 | 01/02；SPI Abort不回收残留TX/RX，单向和双向BUSY | 双流均检查并停止、EN=0才空闲，新READY可完成下一笔1024B | 待执行 |
| R2-V02 | 02；分别注入TX/RX Abort失败、EN仍高、SPI Abort失败 | 保持state3，不能借缓冲或启动；两方向均尝试，IRQ恢复 | 待执行 |
| R2-V03 | 02/03；迟到完成、失败后重试、READY新沿、10ms超时 | 不发布旧RX、不丢新许可、不复用旧许可；一次超时只加一次，可在故障解除后恢复 | 待执行 |
| R2-V04 | 01～04；隔离撤回显式DMA回收，保留旧READY负对照 | 旧路径精确失败于残留DMA断言；实际生产代码和原状态测试通过 | 待执行 |
| R2-V05 | 04/CS-01～15；宿主夹具、既有GCC入口及Keil AC5实际构建 | 记录真实编译器和结果、ABI/布局不变、新BSP无新增警告；旧工具缺失不写通过 | 待执行 |
| R2-V06 | 01～04；获准烧录后真实停车查询及无自动复位恢复 | 三项STM32业务返回，无残留DMA导致的持续截短；初始移位仍单独记录 | 未烧录 |

本机原native GCC及旧电脑ARM GCC路径若缺失，记录原入口失败，不安装/切换固件SDK。
可另用已有Windows Clang/LLD编译同一BSP和相同C断言夹具执行，明确这是另一种宿主
编译器证据，不伪称原GCC或实机通过。Keil构建保留旧警告清单，比较本批有无新增。
STM32板上更新前展示旧/新HEX哈希、构建/宿主结果和更新原因，按统一flash.ps1及既有
授权边界处理。没有授权不烧录；新HEX不是板上版本证据。

回退仅限本批BSP/夹具/规约增量，不清其他任务文件；不通过提高SCK或自动复位掩盖异常。
