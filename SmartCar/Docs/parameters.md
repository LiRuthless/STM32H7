# 全局关键参数反查

代码基线8d5ab2f，2026-11-07。此页只作索引，实际值、类型、单位/标度、默认/范围、来源、覆盖、生效和持久化关系由链接的主表维护。源码未限制范围时如实写未校验，不把建议稳定区间当事实；默认值不冒充当前运行值。

| 分类 | 主参数表 | 源码与检索内容 |
|---|---|---|
| 采集/电感 | [具体参数表](sensing/electromagnetic.md#parameters) | Track_ReadAdc/Track_CalculateError，8读去极值、权重/对称度 |
| 采集/灰度原码 | [具体参数表](platform/adc.md#parameters) | ADC rank/DMA/12bit、新转换/700μs；#gray-raw为帧发布机制 |
| 标定/质心 | [具体参数表](sensing/grayscale.md#parameters) | black/white、-1门禁、200/50阈值与整数公式 |
| 姿态/滤波 | [具体参数表](sensing/attitude.md#parameters) | 14.3、100读、0.2Hz/0.002s、0.001梯形 |
| 编码器 | [具体参数表](platform/encoders.md#parameters) | 左右方向宏、读清和硬件量程 |
| 控制器 | [具体参数表](control/controllers.md#parameters) | KP20/KI0.75、±1000/7500、死区500、积分±7000 |
| 差速/反馈 | [具体参数表](control/motor-execution.md#parameters) | 0.88低通、目标/反馈/累计脉冲、风扇运行值 |
| 主态/子态 | [具体参数表](control/profiles.md#parameters) | 全部fields和值矩阵、8000/10000 |
| 元素 | [具体参数表](control/elements-and-dormant-code.md#parameters) | 2800/25/300、5000/13000/32000、−30/±10 |
| 保护/调度 | [具体参数表](system/runtime-and-safety.md#parameters) | 1/2/5ms、0/0/1、1000/1001、900μs、电池300/1220/201、PC13/看门狗 |
| 持久参数 | [具体参数表](services/parameters.md#parameters) | 两模式两档默认/恢复/活动值/镜像、版本布局CRC与保存覆盖 |
| 临时事务 | [具体参数表](services/runtime-tuning.md#parameters) | 八项范围、revision、上下文、完整拍生效/停车保存 |
| 日志 | [具体参数表](services/recording-and-export.md#parameters) | 256B/96B、容量/队列/批写、CRC、导出字段 |
| 通信 | [具体参数表](services/communication.md#parameters) | SCB长度、超时/重试、队列/会话/上传 |
| 物理通信 | [具体参数表](platform/spi-wifi.md#parameters) | 1024B DMA/READY、时钟与10ms回收 |
| UART | [具体参数表](platform/uart.md#parameters) | 115200/256B环形/100ms及返回语义 |
| DL1B | [具体参数表](platform/dl1b.md#parameters) | 10ms HAL、8192无效、mm范围/复位 |
| IMU原码 | [具体参数表](platform/imu660rb.md#parameters) | WHO_AM_I、量程/ODR/CS与错误发布 |
| Flash原语 | [具体参数表](platform/w25q64-and-flash.md#parameters) | 8MB、页/扇区/块与WIP超时 |
| LCD/物理执行 | [具体参数表](platform/lcd.md#parameters) | SPI4、背光/字符缓存/屏配置 |
| PWM/DIR | [具体参数表](platform/pwm-and-direction.md#parameters) | 0～10000、CCR、通道引脚/频率/极性 |
| 时钟/DMA资源 | [具体参数表](platform/clocks-and-dma.md#parameters) | PLL/Cache、DMA地址、中断资源 |
| 采样窗口 | [具体参数表](platform/sampler.md#parameters) | 1ms/2ms累计、原码发布/reset语义 |
| HMI | [具体参数表](hmi/operation.md#parameters) | 模式/页面/按键区间、步进、100ms刷新 |
| 板测 | [具体参数表](platform/isolated-board-tests.md#parameters) | 0～15选择、1024×6B队列、16B帧、460800、60秒/窗口 |

控制参数必须沿默认初始化→有效Flash恢复→模式/档位应用→状态fields部分覆写→本拍临时覆盖追溯。活动速度PI、参数查询镜像和已保存Flash不保证同时相等，参见[R06](known-issues.md)。标定是代码中的实测配置，不属于Flash镜像；原码计数与物理V/mm/RPM之间未经核实的换算不得补猜。
