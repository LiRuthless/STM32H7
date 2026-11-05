# 本地诊断

> 核对：2026-11-05 工作区；需求见 [traceability](../traceability.md)。

<a id="f09"></a>
## F09 快照与串口计数

### 状态、接口与规则

SPI/NET/池的内部快照和本地串口诊断已实现；Phase05统一bridge_stats模块不存在，不能按R6手稿宣称桥接统计已交付。详见 [问题 EB-04](../known-issues.md#eb-04)。

输入为各服务实际处理事件；输出是包数、CRC错、截短、重试、背压、丢弃、队列额度/峰值、session/ready、服务错误和栈余量等内部快照。单位以实际字段为准：事务/包/记录不能混用；容量推导与真实堆栈测量分开。具体快照结构由所属模块头定义，不在此复制第二套完整字段表。

SPI任务发布自己的health快照，NET发布自己的状态，调用者短临界区读取副本，不持锁解析、操作socket或等待驱动。app_main打印启动状态；SPI现有约5秒日志包括invalid、duplicates、backpressure、delivered、rebuilds、telemetry_dropped、invalid_outbound等既有protocol计数。本地日志不承担业务传输，不包含密码或向SCB/1增加命令/统计字段。统计汇总不应改变计数点、重试/队列语义或清零时机。

### 实现与状态归属

| 文件 / 关键符号 | owner |
|---|---|
| [app_main.c](../../main/app_main.c)：app_main | 启动报告打印；具体服务职责见F01 |
| [spi_bridge.c](../../main/spi_bridge.c)：publish_health、bridge_spi_snapshot、run_transactions | SPI任务写，其他任务读取快照 |
| [net_bridge.c](../../main/net_bridge.c)：bridge_net_snapshot | NET任务写，短临界区读取 |
| [buffer_pool.c](../../main/buffer_pool.c)：bridge_pool_snapshot | 池元数据锁内复制；无数据指针转移 |
| 目标 bridge_stats.c/.h | 尚不存在；统一桥接统计待Phase05与R-05冲突核实后实施 |

### 验证及历史边界

向既有SPI/网络/池测试注入已知数量事件，核对快照增量和错误归因；串口只含本地诊断、无密码、业务socket无调试文本。实际内存/周期与负载需对应镜像实测。历史P03/P04证据不等于整机统计通过。
[R6增量记录](../../specs/features/05-bridge-lifecycle/r6-log-counters.md)保留2026-10-22原批准、目标和258项回归描述；本轮只读发现SPI日志字段存在，bridge_state/bridge_stats缺失，未发现足以重验其全部声明的当前原始产物，不把258改为当前运行结果。
