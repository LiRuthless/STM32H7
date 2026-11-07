# USART1 字节通道

现行源码核对基线：8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e（2026-11-07）。本文为实现事实说明，未重新批准算法或硬件参数；未执行固件/实机验收。

<a id="f27"></a>
## F27 接收、阻塞发送与错误返回

USART1 PA9 TX/PA10 RX，115200、8数据位、无校验、1停止位，NVIC优先级3。BSP_UART_Init把head/tail置0并武装1字节中断，忽略HAL接收启动结果。RxCplt回调只处理huart1，先PushRx，再挂下一字节。Error回调按ORE/FE/NE/PE清标志并重新Receive_IT，重武装结果仍未检查，不能把此机制说成任何硬件错误均自动恢复成功。

256字节静态环使用(head+1)&255，留一槽区分满空，实际可读0～255。PushRx满时先推进tail丢最旧字节，再写buf[head]和发布head；因此ISR也写tail，前台Read同样写tail，不能概括成严格单写者队列。Read逐字节读到len或追上head即停，空指针返回0；RxAvailable为瞬时索引差，RxFlush把tail=head，不屏蔽中断。这些接口无整帧边界与一致快照承诺，业务解析另见[通信](../services/communication.md)。

Write(buf,len)空指针或0长度直接0；调用HAL_UART_Transmit超时100ms，HAL_OK返回0，失败返回len−uint16(TxXferCount)，即按已发数量折算。Phase02 FR5要求“未发数量、0全发完”，两者冲突，甚至传输前失败可能返回0；这是D01/M01待决定问题，文档不批准现状。WriteString(NULL)返回1，否则strlen窄化uint16后转Write；fputc只发ch内存中的1字节、忽略发送结果、返回ch。发送前台串行使用，不允许Flash/LCD等调用中的打印与另一发送交错。

<a id="parameters"></a>
## 参数与来源

| 参数 | 类型/单位/值 | 来源、覆盖、生效和持久化 |
|---|---|---|
| USART1配置 | 115200 baud / 8N1 / 优先级3 | Core usart.c；初始化生效，无Flash覆盖 |
| BSP_UART_RX_BUF_SIZE | 编译宏256B，容量255B | bsp_uart.c；掩码回绕要求2的幂，无运行设置/落盘 |
| Write超时 | HAL参数100ms | 每次阻塞发送；依赖HAL时基，非硬ISR安全上界 |
| Write len / Read len | uint16，0～65535B | 调用参数；0长度不传输；发送失败可能已产生外部字节 |

独立IMU板测不复用公共Write错误语义，而由bsp_board_test检查HAL结果和TC，见[F33 IMU](isolated-board-tests.md#f33-imu)。来源：[Phase02 FR3～5](../../specs/features/02-bsp-platform/requirements.md)。实现：[bsp_uart.c](../../BSP/bsp_uart.c)、[Core usart.c](../../Core/Src/usart.c)。本次核对满缓冲、读/刷新副作用和成功/失败公式；硬件串口、并发与异常接收待验，不以现有软件记录替代。
