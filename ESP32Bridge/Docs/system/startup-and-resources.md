# 启动与固定资源

> 核对：2026-11-05 工作区源码；工程未纳入当前 Git 历史。历史证据见阶段记录，本轮未构建或上机。需求索引见 [traceability](../traceability.md)。

<a id="f01"></a>
## F01 启动、配置与固定池

### 用途、状态与输入输出

已实现。由 app_main 单次启动，使用本地编译配置密码和 IDF 运行时；输出逐步启动报告、固定池及服务快照。密码为显式长度 8～63 个非 NUL ASCII 字节；配置缺失在构建检查报错，运行时非法则阻塞 AP/socket，不输出密码。

| 资源 | 容量/单位 | owner / 生效时点 |
|---|---|---|
| TX/RX DMA | 各 1024B，64B 对齐，内部 DMA 可达 | 池管理额度，SPI 服务独占，驱动提交后应用不得访问 |
| UDP / EVENT 池 | 16×1016B / 32×1024B | 启动一次分配；当前只有池能力，不代表桥接队列已接入 |
| 数据区 | 51072B | 容量推导；元数据、SDK 及任务占用另计，不是实测总内存 |

### 流程、状态与失败

先将 GPIO14 锁存高电平再配置输出，撤销 READY；校验密码，建立固定池，再依次尝试 NVS → netif → event loop → AP → sockets → SPI。启动协调器按依赖记录 NOT_ATTEMPTED、SUCCEEDED、FAILED、BLOCKED、UNIMPLEMENTED 和原错误。网络链失败不阻塞独立 SPI；池失败阻塞依赖服务；READY 保护失败阻塞 SPI。缺失回调不能记成功。NVS 失败不自动擦除 Flash 或重启。

池四次分配任一失败须回滚本轮已取得内存，不发布半池；重复初始化拒绝。槽句柄包含类、索引和代际，重复/过期/伪造归还拒绝；代际不能静默回绕。DMA 为 IDLE → APPLICATION → DRIVER → APPLICATION → IDLE。只有驱动成功返回原事务，或排队失败证明未接受时，同一 SPI owner 才能 reclaim。软件超时、会话变化和断网均不是回收证据。

元数据由启动适配提供的短临界区保护；分配/释放不在锁内执行。init/deinit 只在服务停止且无并发使用时执行；槽借出或 DMA 忙时不得销毁。池不实现 FIFO、丢旧策略、可靠 ACK 或车辆存储。

### 实现与状态归属

| 文件 / 关键符号 | 职责与 owner / 上下文 |
|---|---|
| [app_main.c](../../main/app_main.c)：app_main | IDF 启动任务；读取报告并打印本地诊断 |
| [startup.c](../../main/startup.c)、[startup.h](../../main/startup.h)：bridge_startup_run、bridge_password_validate | 可注入依赖协调，无共享运行期服务状态 |
| [startup_idf.c](../../main/startup_idf.c)：bridge_startup_idf_run、pool_alloc、pool_lock | READY/密码/内存与实际 SDK 绑定；只启动一次 |
| [buffer_pool.c](../../main/buffer_pool.c)、[buffer_pool.h](../../main/buffer_pool.h)：bridge_pool_init/deinit、bridge_pool_dma_acquire/submit/reclaim/release、bridge_pool_claim/release/snapshot | 池持有额度与代际；借出数据由唯一持有者访问，锁仅保护元数据 |

### 验证与限制

方法：复用 test_foundation，对启动各步失败、缺回调、密码 7/8/63/64 字节与非法字节、分配失败、忙池销毁、旧句柄和 DMA 状态逐项观察。判据是调用/依赖与报告一致，无部分池发布、无错误释放，网络失败后仍尝试 SPI。
历史软件/目标证据：[P01 G2](../validation/phase-01-group-2.md)。真实 DMA 能力、启动波形、堆栈水位按对应镜像和设备记录，不由主机结果推定；已有上机及修复后待验边界见 [roadmap](../../specs/roadmap.md)。

## 本轮存量整改追溯

2026-11-05：[Phase01～04格式与注释整改契约](../../specs/research/2026-11-05-source-style-remediation-draft.md)；当前规约准备，现行功能、状态owner和模块映射不变。阶段验收及硬件待验不因本次整改自动完成。
