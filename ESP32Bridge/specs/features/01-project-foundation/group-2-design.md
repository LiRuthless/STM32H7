# Group 2：启动与固定资源增量规约

状态：**用户已确认，纯规约已提交73a2a2f；实现已提交a2947a8，实机部分仍待验**。依据已提交Phase 01的FR-2、FR-3、FR-4、TC-2与 [plan](plan.md)。本增量不改变SCB/1、不决定G-01～G-03，不提前实现阶段03/04/05。

## 必要增量与推荐方案

现有需求已固定启动顺序、双DMA缓冲、UDP16包和EVENT32条，但未固定资源接口、单槽大小、驱动持有状态、部分分配失败的回收规则。plan要求这些内容在Group 2代码前固定。推荐采用“启动分配一次的内部固定池＋可注入启动步骤”；替代方案为各服务分别管理资源，其影响是所有权和失败回收分散，后续更难核对总容量。用户已确认本增量选择前者，先独立提交本规约，再实施。

## 文件与内部接口

已新增 `main/buffer_pool.c/.h`、`main/startup.c/.h` 与 `main/startup_idf.c`，并修改app_main和组件CMake；测试为 [test_foundation.py](../../../tests/test_foundation.py)、[C夹具](../../../tests/host/foundation_harness.c) 及tests/host/include SDK替身。以下C接口仅属本工程内部，不是共享协议。

| 接口 | 输入、输出与责任 |
|---|---|
| `esp_err_t bridge_pool_init(const bridge_pool_ops_t *ops)` | 启动阶段单线程调用；ops提供内部对齐分配、释放及元数据临界区操作。成功才发布完整池；重复初始化拒绝，不覆盖既有资源 |
| `esp_err_t bridge_pool_deinit(void)` | 有应用借出的槽或DMA仍归驱动时拒绝；空闲才释放，不产生悬空已借指针；未初始化时可安全调用 |
| `esp_err_t bridge_pool_dma_acquire(bridge_dma_pair_t *pair)` | 从空闲状态借出唯一TX/RX对；返回两指针、各1024B容量与代际凭据；第二借用者立即拒绝 |
| `esp_err_t bridge_pool_dma_submit(const bridge_dma_pair_t *pair)` | 校验凭据和当前应用所有权后，标记驱动持有；应用自此不得读写两缓冲。阶段03必须先标记交接再调用queue_trans，提交失败时由同一SPI任务撤回 |
| `esp_err_t bridge_pool_dma_reclaim(const bridge_dma_pair_t *pair)` | 只允许持有凭据的SPI任务在驱动返回事务或确认提交失败后调用；恢复应用所有权，不能将逻辑超时当作驱动回收 |
| `esp_err_t bridge_pool_dma_release(const bridge_dma_pair_t *pair)` | 应用所有权下归还至空闲；驱动持有时拒绝。阶段03可在回收后继续准备/提交，无须每事务释放重借 |
| `esp_err_t bridge_pool_claim(bridge_pool_kind_t kind, bridge_pool_slot_t *slot)` | 从UDP/EVENT对应池非阻塞取唯一槽，返回数据指针、容量和代际凭据；耗尽返回容量错误，不ACK任何业务 |
| `esp_err_t bridge_pool_release(const bridge_pool_slot_t *slot)` | 校验池类、索引和代际凭据；重复归还、伪造/过期句柄均拒绝，不能释放另一拥有者的槽 |
| `esp_err_t bridge_pool_snapshot(bridge_pool_snapshot_t *out)` | 复制容量、当前使用和峰值、已分配字节及DMA所有权状态；不返回密码、车辆业务信息或未测量堆数据 |
| `esp_err_t bridge_password_validate(const unsigned char *data, size_t length)` | 按显式长度检查8～63个ASCII字节且无NUL；不打印或保存输入。用于启动前校验及主机错误注入 |
| `esp_err_t bridge_startup_run(const bridge_startup_ops_t *ops, bridge_startup_report_t *report)` | 使用注入步骤和依赖规则执行一次启动尝试，填每步状态及原始错误；不把缺失服务入口视作成功 |

`bridge_pool_ops_t`包含 `alloc(size_t alignment, size_t bytes, bool dma)`、`free(void *ptr)`、`lock(void)`、`unlock(void)` 函数指针，所有回调生命周期覆盖池的整个使用期。默认IDF适配分别使用 `heap_caps_aligned_alloc`、`heap_caps_free` 和FreeRTOS短临界区；测试替身只用于主机失败注入。分配/释放不在临界区内执行，初始化/销毁只在服务停止且无并发使用时调用。

`bridge_startup_ops_t`包含 `void *context` 及 `nvs`、`netif`、`event_loop`、`ap`、`sockets`、`spi` 六个 `esp_err_t (*)(void *context)` 回调。报告逐项记录 `NOT_ATTEMPTED / SUCCEEDED / FAILED / BLOCKED / UNIMPLEMENTED`；`BLOCKED`还须记录前置依赖，不能与未实现混为一谈。全部实际成功才返回 `ESP_OK`；否则返回首个实际错误，若仅有未实现项则返回 `ESP_ERR_NOT_SUPPORTED`。

## 容量、内存与所有权

| 资源 | 本组固定容量 | 初始化与内存要求 | 当前/后续使用者 |
|---|---:|---|---|
| SPI TX | 1×1024B | 64B对齐，`MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT`，独立分配且检查 | 阶段03唯一SPI任务；驱动窗口1 |
| SPI RX | 1×1024B | 同TX，不能与TX别名 | 同上 |
| UDP帧槽 | 16×1016B | 一次分配16256B内部8bit内存；不依赖PSRAM | 阶段05持有/排队，网络发送完成后归还 |
| EVENT帧槽 | 32×1024B | 一次分配32768B内部8bit内存；上限为头32B＋负载992B | 阶段05持有/排队，可靠发送生命周期结束后归还 |

数据区合计51072B是**本增量的容量推导，不是实测内存占用**。槽位代际、使用位图、峰值、临界区等元数据另占内部静态内存；SDK驱动/任务/网络仍可能动态分配，最终占用由构建及运行测量给出。本组不创建无消费者的网络或SPI任务，不猜测其任务栈大小。

池只负责分配、所有权与额度，不实现遥测丢旧、EVENT_GAP、可靠ACK或消息分发。槽位持有者负责记录有效长度且不得超过容量；实际FIFO和背压由阶段05接入。可靠通道、查询和最终结果的独立保留槽按阶段05 plan另行审查，不能用上述UDP/EVENT池冒充已完成可靠资源设计。

所有槽句柄含池类、索引和非零代际标识，校验成功才改变元数据。初始化回滚、销毁再初始化后，旧句柄保持无效；代际达到上限时拒绝进一步分配，禁止静默回绕。元数据操作在短临界区内完成，不在锁内解析、复制完整帧、分配内存或操作socket；借出数据只有唯一持有者可访问。

DMA所有权为 `IDLE → APPLICATION → DRIVER → APPLICATION → IDLE`。池不是驱动回收证据；正常回收只能由SPI服务在 `spi_slave_get_trans_result`成功后发起。软件超时、网络断线、会话变化均无权释放或改写驱动持有缓冲。Group 2只实现并测试所有权管理，驱动真实接入在阶段03完成。

## 启动流程与失败行为

1. 先撤销READY：设置GPIO14输出高电平，再配置输出，检查返回值。不将EN作为输出，不复位对端。GPIO保护失败记录为SPI的阻塞原因，不妨碍独立网络初始化；主机测试不是上电波形证据。
2. 校验编译后的密码，记录不含输入的错误。非法密码阻塞AP/socket，不作为SPI永久停用条件。
3. 初始化完整固定池。四次分配任一步失败、地址不满足内部/DMA能力或对齐要求时，回收本轮已取得的全部内存，清空未发布指针，返回错误；依赖池的服务不启动、不接管业务，READY保持撤销。
4. 按 `NVS → netif → event_loop → AP → sockets → SPI` 尝试。NVS失败阻塞本网络启动链，netif/event失败阻塞下游网络；AP失败仅阻塞socket。无论网络链在哪一步失败，只要固定池和READY保护可用，仍尝试独立SPI入口。
5. NVS使用实际 `nvs_flash_init`，netif/event使用SDK实际初始化。NVS错误不自动擦除Flash、不自动重启；逐步保留原始错误。后续AP/socket的资源撤销由对应服务适配承担，启动协调器不能把部分初始化记成成功。
6. Group 2未实现的AP/socket/SPI入口保持空回调，报告 `UNIMPLEMENTED` 或依赖 `BLOCKED`；不增加返回固定成功的桩，不启动假服务。主机测试使用显式测试回调验证正常及失败顺序；默认固件日志明确区分本组已执行步骤和服务尚未实现。
7. 普通运行期socket错误仍由阶段04处理，不调用整机重启、不影响SPI；本组不虚构已完成运行期网络错误恢复。

## 验证方法与通过标准

以下保留规约73a2a2f提交时的验证计划；软件执行结果见实施追溯，不计入Group 1的16项历史记录：

| 关联项 | 真实检查与输入 | 本组通过标准及限制 |
|---|---|---|
| V-2 / FR-2 | 编译生产startup C，由主机测试替身分别注入NVS/netif/event/AP/socket/SPI错误、缺失回调、READY保护失败及密码非法 | 正常调用顺序正确，失败只阻塞相应依赖；网络失败后SPI仍被尝试；所有报告与实际调用一致，缺失入口不是成功；运行期socket恢复仍待阶段04 |
| V-3 / FR-3 | 编译生产密码校验C，输入NULL、7/8/63/64字节、NUL、非ASCII及合法ASCII | 长度与字节边界正确；诊断无输入；不以主机测试宣称真实AP启动通过 |
| V-4 / FR-4 | 编译生产池C，注入第1～4次分配失败、能力/对齐异常、重复初始化，循环claim/release及DMA状态转换 | 无部分池发布、无泄漏/双重释放；满池有明确错误；运行循环无新分配，峰值正确，重复/过期句柄拒绝，驱动持有时无法销毁/借出 |
| V-8 / TC-2 | 主机容量/对齐/状态测试，目标编译与map/size；固件保留实际内部堆和最低余量诊断入口 | 软件证明2×1024B、16×1016B、32×1024B和窗口1；目标链接占用按本次结果填。真实DMA能力、启动堆与任务栈尚未实测时仍待验 |
| V-1、V-7、V-9 | 复跑Group 1检查，v6.0.0目标构建，密码/产物/范围检查 | 回归无新增失败；只有ESP32Bridge明确路径，规约增量提交先于对应实现 |

主机测试须编译并调用生产C逻辑，不能只用Python重新实现同一算法。优先核对已安装编译器的主机目标能力；不可用时记录具体错误，不自动安装。所有主机、目标构建和实机结果分别回填。

## 审查及提交边界

本增量包含本文件和关联plan/requirements/validation/roadmap/CHANGELOG/traceability的同步。建议纯规约提交信息：`docs(spec): 明确 Phase 01 Group 2 启动与固定池设计`。用户确认后才独立提交；随后实施Group 2，按原要求验证后交审查，不能连同实现混提。

## 本次静态核对

2026-10-17实际执行：12份本任务Markdown变更，工程实现保持8750d6a原样；34份Markdown的152个本地链接有效，六阶段58条需求仍在plan/validation可定位，`git diff --check`通过，暂存区为空，SCB/1及来源副本未改。证据摘要位于被忽略的`.local/group-2-spec-check.json`。期间出现的其他任务`Reports/`新增文件保留，未纳入提交。本次没有重跑主机/目标构建或实机测试；该次检查时为待确认草案；用户随后明确确认并授权独立提交后实施。

## 实施追溯

上述接口已按73a2a2f实施，实际文件、符号及本次59项主机/目标构建结果见 [Group 2报告](../../../docs/validation/phase-01-group-2.md)。此前静态核对保留为规约审查历史；实机测量仍未执行，Group 2实现已单独提交a2947a8；该组报告保留其原交付时的审查状态。
