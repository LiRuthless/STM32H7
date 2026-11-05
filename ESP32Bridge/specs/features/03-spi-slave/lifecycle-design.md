# Phase 03 Group 2/3：事务循环与协议适配细化

2026-11-05职责迁移：现行/目标契约见[F04 SPI事务与协议适配](../../../Docs/transport/spi.md#f04)。本页保留原动机、候选、用户决定、提交及验证声明的冻结原文，不继续维护现行详细接口；原“待确认/已完成”等只对应记录时点，当前冲突见[问题页](../../../Docs/known-issues.md)。

<details>
<summary>原设计、决定及阶段验证（冻结原文，不作为现行规则）</summary>

~~~~~markdown
# Phase 03 Group 2/3：事务循环与协议适配细化

2026-10-18，Group 1已提交712fb41；用户明确取消阶段内逐组人工等待，阶段末审查一次，并确认G-06。此设计及三件套先独立纯规约提交，再实施。SCB/1不变，不提前宣称Phase 04/05的网络/业务队列完成。

## requirements

### 03/FR-3、FR-4、TC-1、TC-2：驱动所有权

`spi_bridge.c`在已初始化的Core1任务中运行深度1循环：应用准备TX/清RX/描述→pool_dma_submit→queue_trans(ticks=0)→驱动post_setup拉低READY→post_trans拉高READY→get_trans_result成功且返回原描述→pool_dma_reclaim→先检trans_len==8192→处理/下一次准备。两个回调均IRAM_ATTR，仅内联gpio_ll_set_level访问GPIO14；不读写DMA、日志、解析或锁。显式IRAM配置保持，验证最终ELF回调段和调用链。

排队失败证明本次未接受，可撤回pool所有权、撤销READY、停止该服务；get错误或返回意外描述不能作为本缓冲回收证据，保持驱动所有权/固定内存有效，READY高，停止服务且报错，不销毁池、不复位。截短（含0bit）和可观察超长均不解析/不ACK，重排恢复；S3硬件可能把超长trans_len饱和到配置长度，等于8192不能证明没有多余时钟，最终必须波形核对。

get_trans_result每次有界阻塞10ms（换算至少1 tick），超时仅服务独立逻辑计时/统计，不访问驱动持有的描述/TX/RX、不把超时当回收；不是忙等。无主机时钟时继续等待；不得为了重发改写已排队DMA。

### 03/FR-5、FR-6：已定协议接入

新增`spi_protocol.c/.h`，SPI任务独占session、独立reliable实例（复用Phase 02每实例2×1024B固定图像契约）、计数与发送元数据；JSON工作区1492B由该任务栈独占，复用已批准6144B栈。无新增业务队列、整份日志缓存或额外任务。可注入RNG/时间供测试，生产适配使用esp_random/esp_timer_get_time，不用固定成功/固定会话回退。新会话随机数至多尝试4次，失败本帧不握手，待对端重发。

- 启动/失效等待HELLO时发送合法空IDLE(session=0/seq=0)表示无已建链路；不接管旧帧。合法HELLO生成随机非零会话，后续事务发送既有HELLO_ACK；等待IDLE期间重复发相同会话ACK。新会话IDLE后才ready，普通seq从1开始；已ready新HELLO换会话、旧可靠窗口清理并通知上层失效。
- 完整物理事务先确认上一TX实际发送完成，再处理RX：坏RX CRC不抹掉实际TX完成事实；物理截短则不确认可靠发送完成，下一应用缓冲继续发送同一可靠图像。可靠计时只在完整发送后开始，原200ms/三次重发不变。
- 独立poll只在reliable缓存上标记重发待办；必须待实际回收后才能复制重发到DMA。期间新RX精确ACK可取消待办；短写/截短不算已完成重发。耗尽/近回绕只撤销逻辑ready、重握手，不触碰仍由驱动持有内存。
- SPI正常入站只接受IDLE/TELEMETRY及STM32 RESULT/EVENT/LOG_DATA，方向错误COMMAND/HELLO_ACK不接管。EVENT/遥测异步request_id=0，RESULT/LOG_DATA请求编号非零；不解释op业务含义或设备参数。
- 引入启动前一次配置的内部`bridge_spi_port_t`：reserve/commit/cancel（预留后才ACK）、telemetry（必须立即复制或丢弃）、peek_tx/tx_consumed（从上层固定槽编码至独占可靠图像后归还）、session_changed（只通知，不调用socket）。所有回调非阻塞、无网络I/O，由SPI所有者调用，不可重入。frame view仅在回调内有效，不能保留RX指针。配置在服务启动后拒绝，禁止并发换表。
- 本阶段真实生产尚无网络消费者：默认无可靠容量，不ACK可靠业务，遥测直接丢弃并计包/记录；不是虚构成功回调。后续Phase 05在启动前绑定真正固定池生命周期端口；不把EVENT池冒充未审查的可靠队列。本阶段测试绑定有界接管夹具以验证真实接口和可靠发送链，但不声称生产网络已存在。

## plan

Group 2先落实READY/排队/回收/计时等待和错误路径，Group 3再把spi_protocol与既有codec/session/reliable接入；组内各自测试后连续推进，阶段末统一交审。`link_session.c/.h`增加复用已有HELLO_ACK编码器的SPI待确认回复入口，不加字段。`spi_bridge.h`公开启动前端口配置及只读统计快照；快照在任务上下文短临界区发布/复制，不在锁内解析、复制完整帧、驱动或网络I/O。记录驱动错误、事务/截短/CRC/无效/去重/背压、遥测丢包/记录、重试/重建/session/ready及任务栈最低余量，调试只串口。更新app_main事实描述及组件esp_timer依赖；无无限高频日志。

## validation

- 03/V-3/V-4/V-7：直接编译生产spi_bridge+pool，用SDK驱动边界替身在queue前、持有期间、return时断言所有权；哨兵/快照证明10ms超时不修改描述/两DMA；post_setup/post_trans对应READY顺序；0/8191/8192/可见8193bit、坏CRC/JSON、排队失败、get错误/错描述。
- 03/V-5/V-6：通过实际SPI任务接口注入独立SCB帧，验证后续事务HELLO_ACK/随机数异常/重复HELLO/旧会话/重建；满可靠额度不ACK、预留后仅交付一次、重复可靠只ACK，独立IDLE可ACK；借用视图在回收处理期间内复制，不流出驱动RX。
- 可注入时钟测试199/200ms/最多3重发、无主机时钟仍记待办而不改DMA、重发待办收到ACK取消、截短可靠TX仍保留原图像及会话变化清理。
- 真实v6.0.0 build/size、ELF/map确认生产入口与IRAM回调；全回归及文档链接检查。真实频率/超长时钟/READY电气/堆与栈高水位未实测一律待验；不烧录。Phase 03软件交付后统一交审，未将Phase 04～06工具/桥接/实机提前勾选完成。
~~~~~

</details>
