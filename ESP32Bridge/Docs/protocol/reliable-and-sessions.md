# 独立可靠窗口与会话

核对441a778，2026-11-07。实现：[reliable_link.c](../../main/reliable_link.c)、[状态声明](../../main/reliable_link.h)、[link_session.c](../../main/link_session.c)、[会话声明](../../main/link_session.h)。设计来源 [Phase02 G2](../../specs/features/02-scb-protocol/group-2-design.md)、[G-02](../../specs/features/02-scb-protocol/g-02-session-change.md)。

<a id="f03"></a>
## F03 范围与owner

SPI/NET各自任务串行持有独立实例、JSON工作区、TX重发图像及最近可靠RX身份，无共享可写协议对象、DMA/socket指针、运行期分配或内部锁。输入是完整帧、真实下游预留事实、实际发送完成及单调毫秒；输出动作由传输owner执行，不替其操作驱动/socket。接管ACK不是APPLIED/SAVED，业务request_id幂等属于STM32。

<a id="f03-window"></a>
## 窗口、预留与精确ACK

reset清全部状态，next_seq=1；session零或非SPI/TCP实例为dead。receive先清输出，完整decode后要求当前session，seq非0且非UINT32_MAX，排除HELLO/HELLO_ACK。可靠seq小于rx_reliable_seq立即STALE；相同seq须头32B+实际payload逐字节相等（含ack/CRC，不含SPI填充），否则INVALID，相等DUPLICATE。新可靠帧reserved=false为BACKPRESSURE。非可靠seq≤rx_high为STALE。可靠身份独立保存，不被非可靠高序号遮蔽。

可靠旧seq及同seq改内容会提前拒绝；非可靠旧seq虽为STALE仍继续检查反向ACK，不交付。通过提前拒绝条件后，pending且sent且ack_seq精确等于tx_seq才清pending/sending/tx_length；背压帧也可确认反向已完整发送TX，错ACK不释放。DELIVER可靠帧复制身份，更新rx_reliable_seq/ack_seq；普通交付更新rx_high。返回view仍借用输入。调用者必须先reserve真正取得槽，再receive，DELIVER后提交复制，之后才允许ACK出线；失败/重复撤销预留，不能仅观察空位就传true。当前生产桥接消费者未接通，见 [F06](../bridge/forwarding-and-lifecycle.md#f06)。

prepare覆盖session/seq/ack；pending时新可靠帧BUSY，IDLE等非可靠帧仍可推进ACK。next_seq==UINT32_MAX置dead并REBUILD，不做回绕比较。输出不可与link对象/不可变重发图像重叠；encode成功才递增序号，可靠图像保存原字节并设置pending/sending、sent=false、retries=0。失败used=0，无新序号提交。

<a id="f03-retry"></a>
## 完成发送与重试

sent只有实例有效、pending、sending且seq匹配才接受：首次不计重发，之后每次完整发送使retries加1，置sent=true/sending=false，采样last_send_ms。短写、部分SPI事务或失败不得调用。ACK后迟到完成通知无作用。

poll先清输出；无pending、发送中、未首次完整发送、时钟回退或距离完成不足200ms均OK。到期且retries<3才置sending并RESEND借出原图像，借用到下一次owner调用前；必须复制到应用拥有的缓冲，不可写驱动DMA。第三次重发完整完成后再等待200ms，仍无ACK才dead/REBUILD。200ms不是从prepare/短写起算，不是每次poll都重发；无发送完成通知不会推进重试。耗尽由SPI重握手或NET关闭TCP，各自处理。

<a id="f03-spi-session"></a>
## SPI握手

init零值WAIT_HELLO。hello要求HELLO的session/seq/ack/request全零，全文JSON对象role解码等于stm32、protocol经精确u32等于1；头flags/count由codec先校验。WAIT_HELLO或READY的新HELLO需随机候选非零、不同当前和previous，失败NEED_RANDOM不改变状态，不能固定回退。编码HELLO_ACK成功才更新会话、保存previous、撤销TCP许可并进入WAIT_IDLE；WAIT_IDLE重复HELLO复用当前id。应答用于后续事务，不代表本次MISO已发。

WAIT_IDLE仅接受当前会话、seq非0/UINT32_MAX、ack/request为0的IDLE，然后READY；READY拒绝HELLO_ACK，普通帧PASS交可靠层。pending_reply只在WAIT_IDLE重编码，不换会话。spi_lost保留previous、清当前id、回WAIT_HELLO并撤销TCP许可，既不释放DMA也不关闭socket。

<a id="f03-tcp-session"></a>
## TCP握手与组合

逻辑open只接受一个连接，清hello/close请求/绑定id，不重置SPI；close仅清TCP状态。hello验证零身份、role=pc、protocol=1，生成SPI快照HELLO_ACK（允许session=0、link_ready=false），成功后才登记握手。业务许可要求open、hello、未请求close、SPI READY，且非零id同时等于SPI及绑定TCP会话。

生产NET调用hello_explicit使用只读SPI快照，不访问可写SPI会话对象；BUSY同样按显式快照编码HELLO_ACK加status=BUSY，seq/ack/request/flags零，无SPI填充，不影响首连接。G-02会话变化撤销旧业务，由NET关闭TCP并清订阅，重连重新HELLO，不由SPI操作socket。session=0状态连接仍可保留，不能因dead可靠实例poll就误当真实链路重试耗尽，具体生产条件见 [网络](../transport/network.md#f05)。

## 参数与证据

| 来源/参数 | 当前值、单位、条件 | owner / 生效与复位 |
|---|---|---|
| reliable_link.h SCB_RETRY_MS | 200u，毫秒，完整可靠发送结束起算 | 传输任务，编译固定；sent更新起点 |
| SCB_MAX_RETRIES | 3u，完整重发次数，不含首次 | 传输任务，prepare/reset清零 |
| scb_link_t tx/last_rx | 各1024B；RX身份只用头+负载 | 任务私有，reset清；精确ACK释放TX有效长度 |
| next_seq/session | uint32，seq从1至UINT32_MAX-1，session非零 | reset建立；近回绕重建 |
| reply payload/digits | 局部128B/10B，握手编码工作区 | 单次调用，无跨调用借用 |
| hello protocol | 精确u32=1，role为stm32/pc | 编译角色约束，每HELLO检查 |

历史 [P02 G2](../validation/phase-02-group-2.md)保留；复用现有test_reliable/test_sessions可检查199/200ms、短写、精确ACK、三次重发后等待、重复身份与旧会话。本次静态逐分支核对，未重跑，不推定跨端或硬件验收。

