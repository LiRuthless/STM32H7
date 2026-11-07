# 启动与固定资源

> 核对：2026-11-05 工作区源码；原工程基线35e5627。历史证据见阶段记录，本轮目标构建未验证、未上机。需求索引见 [traceability](../traceability.md)。

<a id="f01"></a>
## F01 启动、配置与固定池

### 用途、状态与输入输出

已实现。由 app_main 单次启动，使用本地编译配置密码和 IDF 运行时；输出逐步启动报告、固定池及服务快照。密码为显式长度 8～63 个非 NUL ASCII 字节；配置缺失在构建检查报错，运行时非法则阻塞 AP/socket，不输出密码。

固定池额度与句柄/DMA完整机制的唯一主说明见[资源池](resource-pools.md#f01-pools)。

### 流程、状态与失败

先将 GPIO14 锁存高电平再配置输出，撤销 READY；校验密码，建立固定池，再依次尝试 NVS → netif → event loop → AP → sockets → SPI。启动协调器按依赖记录 NOT_ATTEMPTED、SUCCEEDED、FAILED、BLOCKED、UNIMPLEMENTED 和原错误。网络链失败不阻塞独立 SPI；池失败阻塞依赖服务；READY 保护失败阻塞 SPI。缺失回调不能记成功。NVS 失败不自动擦除 Flash 或重启。

资源池回滚、代际、销毁约束见[固定池](resource-pools.md#f01-pools)；回收证据见[DMA](resource-pools.md#f01-dma)。

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

2026-11-05：[Phase01～04格式与注释整改契约](../../specs/research/2026-11-05-source-style-remediation-draft.md)；47文件静态整改与人工复核已完成，目标构建未验证，见[本轮报告](../validation/2026-11-05-source-style-review.md)；现行功能、状态owner和模块映射不变。阶段验收及硬件待验不因本次整改自动完成。

<a id="f01-startup"></a>
## 启动依赖细节

bridge_startup_run先初始化逐步报告。池错误优先阻塞全部六步；网络链按NVS→netif→event→AP→sockets依赖传播，SPI无网络前置依赖。AP另受密码检查约束，SPI另受READY保护约束。缺回调是UNIMPLEMENTED/NOT_SUPPORTED，不能记成功。返回首个真实错误，未完成且无其他错误则NOT_SUPPORTED；每步仍保留原dependency/error。READY先锁存GPIO14高再设输出，不自动擦NVS或复位。

<a id="f01-config"></a>
## 配置及资源入口

密码只记录配置键CONFIG_BRIDGE_AP_PASSWORD及8～63字节ASCII/无NUL规则，启动读取并校验，不记录值。固定池详细机制及参数主说明见[资源池](resource-pools.md#f01-pools)，[DMA状态](resource-pools.md#f01-dma)；本页保留原F01入口兼容。来源startup_idf.c与startup.c，owner启动任务，配置在构建/启动时生效，无运行期覆盖。
