# 时钟、DMA 与公共板级资源

现行源码核对基线：8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e（2026-11-07）。本文为实现事实说明，未重新批准算法或硬件参数；未执行固件/实机验收。

<a id="f29-clock-dma"></a>
## F29.CLOCK-DMA 启动配置与接管边界

Core main上电I-Cache开、D-Cache不开；HAL_Init后SystemClock_Config，再USER CODE配置PLL3，随后全部MX外设初始化，最后App_Init。BOARD_TEST_SELECT!=NONE走独立最小初始化分支，详见[F33](isolated-board-tests.md#f33)，不能把其外设集合当正常启动集合。BSP_Runtime_ConfigureSpiClock在正常Lifecycle初始化开头再次恢复SPI45SEL→PLL3Q，避免MX_SPI4的D2PCLK1选择覆盖main补丁。CubeMX再生须保留两处补丁；IOC不是TIM15/SPI1/IWDG及全部运行行为的唯一事实源。

| 时钟/调度 | 当前配置与推算 | 主说明 |
|---|---|---|
| SYSCLK / HCLK / APB | HSE25MHz，PLL1 M5/N192/P2/Q2/R2；SYS480MHz，AHB/2=240MHz，APB1～4 /2=120MHz | main.c / SystemClock_Config |
| SPI1/2内核 | PLL1Q480MHz；SPI1/16=30MHz；SPI2/256=1.875MHz | [Flash](w25q64-and-flash.md#f29-flash)、[WiFi](spi-wifi.md#f29-wifi) |
| SPI4/5 | PLL3 M5/N64/P2/Q4/R2，Q80MHz；SPI4/8=10MHz | [共享LCD](lcd.md#f29-lcd)、[IMU](imu660rb.md#f29-imu) |
| TIM15 / TIM6 / TIM7 | 1 / 2 / 5ms；优先级0 / 0 / 1 | [采样器](sampler.md#f29-sampler)、[运行链](../system/runtime-and-safety.md#f01) |
| ADC DMA / SPI DMA / SPI IRQ / READY EXTI | NVIC 2 / 3 / 3 / 4 | Core dma/spi/gpio.c |

<a id="parameters"></a>
## 内存与时基参数

| 对象/符号 | 地址、大小、单位或范围 | 来源、覆盖、生效、持久化 |
|---|---|---|
| ADC静态DMA | 0x30000000，10×uint16=20B，D2 SRAM1 | bsp_adc.c与链接固定段；无动态分配/Flash覆盖 |
| SPI2 TX / RX | 0x2407F000 / 0x2407F400，各1024B，32B对齐 | bsp_wifi.c；AXI保留4KiB WIFI_DMA，非DTCM |
| GCC默认RAM / 栈 | AXI 0x24000000起508KiB；栈预留0x2000=8KiB | MDK-GCC链接脚本，AC5另有自身布局；静态预算不代替实测峰值 |
| BSP_GRAY_BUDGET_CYCLES | 336000 cycle，700us@480MHz | bsp_time.h；[灰度](adc.md#gray-raw)整帧上限 |
| BSP_CONTROL_BUDGET_CYCLES | 432000 cycle，900us@480MHz | bsp_time.h；控制返回后检查，不保证阻塞中可抢救 |
| DWT cycle / HAL tick | uint32周期计数 / uint32 ms，自然回绕 | BSP_Time_Init只开TRCENA/CYCCNTENA不清零；差值须无符号，DWT约8.95s回绕 |

DMA1不可达DTCM；新增DMA不能一概要求D2，须按实际总线/链接布局。D-Cache保持关闭，当前无需Cache维护；开启必须另行设计。BSP_Time_EnterCritical返回旧PRIMASK并关可屏蔽中断，Exit恢复旧值而非强制开中断；MemoryBarrier只DMB，不替代互斥或整包快照。

BSP_Runtime_StartTimers/StopTimers依次开关TIM6、TIM7，忽略HAL返回；唯一强HAL_TIM_PeriodElapsedCallback在bsp_runtime按实例和编译选择分派正常App或BoardTest。TIM15入口由采样器独占，不能再注册第二强入口。

<a id="keys"></a>
## PC13 物理按键

bsp_key持有raw/stable/armed/pressed/long和两个毫秒时戳。PC13高有效，双沿稳定30ms才接纳；RequireRelease读取当前电平但令stable=按下并清武装，必须先稳定释放。短按在释放时发START，稳定按下满1000ms发一次TOGGLE，长按释放不再发START。StartPressed内部调用PollEvent，会消费事件，不能同拍再轮询一次。参数为bsp_key.c编译常量，不持久化；ADC菜单键不走此状态机，业务分发见[HMI](../hmi/operation.md#f14)。

<a id="watchdog"></a>
## IWDG 与板灯

APP_WDG_ENABLE=1时bsp_wdt直接接管IWDG1（HAL模块未启用）。LSI就绪和PVU/RVU/WVU同步轮询无显式软件超时；PR=4为/64，RLR=2999，WINR=0xFFF禁用窗口，名义32kHz下约6s，非实测时间。解锁→设置→等待同步→reload→enable；首次起跑启动，一旦开启不能软件关闭。宏0时Init/Feed为空操作，不保存Flash；运行TIM7和停车前台喂狗见[系统](../system/runtime-and-safety.md#f01)。PE3蓝灯由runtime设置/翻转，非控制状态owner。

来源：[Phase01](../../specs/features/01-runtime-architecture/requirements.md)、[Phase02](../../specs/features/02-bsp-platform/requirements.md)、[Phase14](../../specs/features/14-dual-mode-tracking/requirements.md)、[Phase16](../../specs/features/16-spi-wifi-link/requirements.md)。实现：[main.c](../../Core/Src/main.c)、[runtime](../../BSP/bsp_runtime.c)、[time](../../BSP/bsp_time.c)、[key](../../BSP/bsp_key.c)、[wdt](../../BSP/bsp_wdt.c)、[GCC链接](../../MDK-GCC/STM32H743VITx.ld)。本次核对启动补丁、优先级/固定地址和关键分支；AC5、真实时钟/中断时序/栈峰值仍待验。
