# W25Q64 与 Flash 访问原语

现行源码核对基线：8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e（2026-11-07）。本文为实现事实说明，未重新批准算法或硬件参数；未执行固件/实机验收。

<a id="f29-flash"></a>
## F29.FLASH 器件、地址和等待

板载SPI1由BSP纯代码接管：PB3 SCK/PB4 MISO/PD7 MOSI/PD6 CS低有效，Mode0、8bit、MSB，PLL1Q480MHz/16名义30MHz。Init配置GPIO与SPI，发0xAB释放掉电后Delay1ms，再0x9F读3字节，必须JEDEC=0xEF4017。所有公开原语0成功/1参数、SPI或等待失败；CS在失败路径释放。

Read范围要求addr<8MiB、len>0且不越界，发0x03+24bit大端地址，再收len；实际HAL长度窄化uint16，没有新增65535限制或分块，超大请求不能因外层范围检查就宣称完整读取。WritePage要求1～256B且不跨256B页、地址有效；先0x06使能写，再0x02+地址+数据，再轮询0x05状态WIP。擦4KiB/64KiB/整片分别0x20/0xD8/0xC7，等待WIP清零。WaitIdle先读状态，成功空闲立即返回，否则按HAL_GetTick无符号差超时；SPI读状态本身也有100ms等待，配置值不是所有组合路径的严格总耗时。

<a id="parameters"></a>
## 布局与固定参数

| 符号/对象 | 数值、单位与约束 | 来源、覆盖、生效和持久化 |
|---|---|---|
| TOTAL / PAGE / SECTOR | 8388608B / 256B / 4096B | bsp_w25q64.h编译常量；物理介质容量，非运行可调 |
| PARAM_ADDR / PARAM_SIZE | 0x7FF000 / 4096B | bsp_flash.c；末扇区；Flash实际持久数据在此 |
| LOG_SIZE | 0x7FF000=8384512B，0～0x7FEFFF | bsp_flash.h；与参数扇区隔离 |
| SPI / 页 / 扇区 / 块 / 整片等待 | 100 / 10 / 500 / 2500 / 110000ms | bsp_w25q64.c；每类命令固定，不从参数镜像覆盖 |
| WORD_SIZE / shadow | 32B日志对齐；4096B静态影子 | bsp_flash.c；无堆分配，前台串行，影子自身不持久 |

## 参数区读改擦写与日志擦除前沿

BSP_Flash_Init把驱动结果存s_ready并清s_log_frontier，接口不返回Init结果；所有访问先检查ready/指针/非零长度/范围。参数Read映射末扇区offset。Write先读整4KiB到静态shadow，memcmp目标段相同则直接成功；不同则更新shadow，擦整个参数扇区，再顺序回写16页。失败无回滚、已擦写内容可能保留部分，不能宣称原镜像原子保留。镜像CRC/版本迁移与保存业务见[参数服务](../services/parameters.md#f16)。

LogErase实际只擦首4KiB，成功frontier=4096；函数内“64KiB全区循环”注释与函数体不符，本文按函数体说明，不修改源码注释。LogWrite要求offset/len均32B对齐；先从frontier顺序擦扇区直到覆盖offset+len，每次成功才推进frontier；再按页边界拆块WritePage。任一页失败立即返回，保留已擦前沿和已写部分，无事务回滚。Init会重置前沿，不能用RAM前沿推定掉电日志恢复能力。LogRead只范围检查后转底层Read，不限制32B对齐。会话、记录、CRC/CSV/二进制导出归[日志](../services/recording-and-export.md)。

只允许前台串行阻塞访问SPI1，不能在ISR擦写；整片擦除当前业务链未调用且仅看门狗启动前维护，110s与6s看门狗不兼容。来源：[Phase02 FR21～25](../../specs/features/02-bsp-platform/requirements.md)、[Phase08](../../specs/features/08-storage-chain-fix/requirements.md)、[Phase11](../../specs/features/11-hardening-cleanup/requirements.md)。实现：[bsp_w25q64.c](../../BSP/bsp_w25q64.c)、[bsp_flash.c](../../BSP/bsp_flash.c)。本次核对检查/擦写/发布顺序和失败副作用；物理Flash、断电、擦写耗时与看门狗仍待实机验收。
