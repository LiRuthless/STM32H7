# 固定池与DMA所有权

<a id="f01-pools"></a>
## F01 固定额度、凭据与生命周期

当前已实现，不等于已接通Phase05队列。源码主映射为 [buffer_pool.c](../../main/buffer_pool.c) 的 bridge_pool_init/deinit/claim/release/snapshot，设计来源 [Phase01 Group2](../../specs/features/01-project-foundation/group-2-design.md)。启动适配提供内部内存分配与短临界区；init/deinit要求服务停止、无并发，运行期借还只锁元数据，借出字节仅唯一持有者访问。

| 参数/真实符号 | 当前值/单位/范围 | 来源、owner与生效 |
|---|---|---|
| BRIDGE_DMA_BYTES / ALIGNMENT | TX、RX各1024B / 64B对齐 | buffer_pool.h，启动分配，SPI独占 |
| BRIDGE_UDP_SLOTS / BYTES | 16槽×1016B，索引0～15 | 同头文件，池额度，启动生效 |
| BRIDGE_EVENT_SLOTS / BYTES | 32槽×1024B，索引0～31 | 同头文件，启动生效 |
| BRIDGE_POOL_DATA_BYTES | 2×1024+16×1016+32×1024=51072B | 仅四数据块，元数据、任务、SDK另计 |
| 普通池对齐 | 4B | buffer_pool.c::bridge_pool_init，分配时 |
| generation | uint64非零，UINT64_MAX后永久耗尽 | 全池共用s_next_generation，锁内推进，跨deinit保留 |

初始化先验证四回调齐全、尚未初始化，再依次取得TX、RX、UDP整块、EVENT整块。每块检查对齐、地址加容量无溢出、与先前块不重叠；NULL返回NO_MEM，其余无效内存返回INVALID_STATE。任一失败扫描本轮已得块，跳过NULL及重复相同地址，锁外free；不发布半池、不清零无效数据。全部通过才清四块并发布initialized；任意重叠地址的free安全不能由重复地址去重保证，分配器必须满足独立块契约。

claim验证类别及输出指针，锁内找首个代际为零的槽，取得新代际才增加used/peak并发布kind/index/data/capacity/generation。满池返回NO_MEM，代际耗尽返回INVALID_STATE；失败保留原输出。release同时核验类别、索引、非零且相等代际、精确地址和容量；成功清槽代际、减used，数据指针立即失效；重复/旧/伪造凭据不减计数。deinit未初始化幂等成功；DMA非IDLE或任一used非零拒绝；成功撤initialized后锁外释放并清池，独立代际计数保留，旧池凭据不能在新池复活。

<a id="f01-dma"></a>
## DMA成对独占状态

acquire仅允许IDLE→APPLICATION，生成新代际并返回TX/RX/容量；submit核验整份凭据后APPLICATION→DRIVER。DRIVER期间应用不能访问字节或事务描述。reclaim仅凭同一SPI owner的驱动成功返回原描述，或queue失败证明未接受，转换DRIVER→APPLICATION。软件等待超时、会话切换或断网均不是回收证据。release只允许APPLICATION→IDLE并清当前代际，不能直接释放DRIVER。每次转换同时检查源状态、代际、两个指针和容量，失败保持状态。

池自身不检查SDK回收事实，由[SPI事务owner](../transport/spi.md#f04-ownership)保证。快照不转移数据指针；计数和复位见[诊断](diagnostics.md#f09-pool)。未来3/6/1槽和新增10240B属于Phase05候选，未并入当前预算。历史证据见[Phase01 G2](../validation/phase-01-group-2.md)，本轮仅源码/文档核对，真实DMA/水位待对应镜像实测。
