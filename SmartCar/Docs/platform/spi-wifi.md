# SPI2 Wi-Fi DMA 事务

现行源码核对基线：8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e（2026-11-07）。本文为实现事实说明，未重新批准算法或硬件参数；未执行固件/实机验收。

<a id="f29-wifi"></a>
## F29.WIFI 物理接口与许可

STM32 SPI2主机Mode0/8bit/MSB/软件CS：PB12 CS低、PB13 SCK、PB15 MOSI、PB14 MISO；PA15 READY下降沿EXTI，低电平启动许可；PD15 ESP32 EN保留不自动复位。BSP只交换固定1024B，不解析SCB帧；帧、ACK、会话/上传归[通信服务](../services/communication.md#scb1)。

Init在短临界区清status、释放CS、清EXTI挂起，读取READY若已低则permits=1并再清挂起，随后清TX/RX。EXTI回调只认WIFI_INT_Pin，每个下降沿permits++，不是前台持续低电平可无限发。status各计数uint32自然回绕，permits由IRQ发布、consumed由前台推进。

GetTx仅state=0返回可写TX，GetRx先DMB仅state=2返回RX；其他情况NULL。GetStatus在PRIMASK临界区复制全结构，调用者指针须有效。ReleaseRx只有state=2才回0，取RX后未Release会阻止下一事务。

## 状态、超时及晚回调

TryExchange若state非0或consumed==permits返回0；否则记started HAL tick、state=1并拉低CS，启动DMA。HAL_BUSY释放CS并回state0且不消费许可；其他返回先consumed++，HAL_OK返回1，错误释放CS、errors++、state3、返回2。启动错误已消费许可，不能当作自动重试。

成功回调只认SPI2且state1，释放CS→completed++→DMB→state2；错误回调同样守卫state1，释放CS→errors++→state3。Service在state1且elapsed>=10ms或state3时进入回收：关DMA1_Stream1/2和SPI2 IRQ，state3→Abort→释放CS→清三IRQ挂起→若超时则timeouts++→state0→重开IRQ。state守卫和清挂起用于隔离晚回调，不允许外层直接写state/缓冲在途内容。此处没有阻塞等待DMA完成；Abort是错误回收动作，不在控制ISR发起通信等待。

<a id="parameters"></a>
## 配置、内存与时钟

| 参数 | 真实值/单位/范围 | 来源、覆盖、生效、持久化 |
|---|---|---|
| BSP_WIFI_TRANSACTION_SIZE | 编译宏1024B/次CS | bsp_wifi.h；缓冲和DMA相同长度，无Flash覆盖 |
| TX / RX | 0x2407F000 / 0x2407F400，各1024B且32B对齐 | bsp_wifi.c/GCC固定链接段；空闲/完成时分别交出指针 |
| SPI2时钟 | 默认/256=1.875MHz；SetFastClock(非0)为/64=7.5MHz | Core spi.c，PLL1Q480MHz推算；仅state0可改，HAL_Init成功返回1；活动链未调用提速，不宣称已高速联调 |
| 超时 | uint32差>=10ms | BSP_WiFi_Service每次前台检查；实际回收延迟受前台阻塞影响 |
| DMA TX / RX / IRQ | DMA1 Stream1 / Stream2，byte、NORMAL、硬件LOW；NVIC3 | Core spi/dma.c；SPI2 IRQ3、READY EXTI4；不抢占TIM6/15 |
| state | uint8 0空闲/1在途/2待读/3错误 | BSP独占，无落盘；Init清零 |

来源：[Phase16 FR1～4](../../specs/features/16-spi-wifi-link/requirements.md)、[协议快照](../../specs/features/16-spi-wifi-link/protocol.md)。实现：[bsp_wifi.c](../../BSP/bsp_wifi.c)、[Core spi.c](../../Core/Src/spi.c)、[dma.c](../../Core/Src/dma.c)、[GPIO](../../Core/Src/gpio.c)。本次核对许可消费、HAL_BUSY/ERROR区别、状态发布和超时顺序。仓库有ESP32Bridge只代表可读另一端实现，SCK/READY、故障注入、吞吐和三端真实联调仍须独立证据。
