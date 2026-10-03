# ESP32-S3-DevKitM-1引脚分配表

仿照你 STM32 表的格式，下面是 **ESP32-S3-DevKitM-1 / ESP32-S3-MINI-1** 的引脚分配表。核心思路是：

**ESP32 在这套小车架构里作为“通信协处理器”**，不是主控。它的主要职责是：

- 通过 **SPI2 从机** 接收 STM32H743 的数据/命令
- 通过 **SPI3 主机** 驱动板载/外接屏幕或传感器
- 预留调试串口和少量 GPIO 给状态指示、复位交互

---

## 一、不可用 / 需避开的引脚

| 引脚 | 原因 | 处理 |
|---|---|---|
| **IO26** | 封装内 PSRAM/Flash 的 SPICS1 | ❌ 不可用（-N4R2 型号固定占用） |
| **IO27~IO32** | 封装内 Flash/PSRAM 的 SPI0/SPI1 总线 | ❌ 不可用（模组内部已用） |
| **IO0** | Strapping（启动模式），上电时低电平进下载模式 | ⚠ 尽量不做普通 IO，若要用需外部上拉 |
| **IO3** | Strapping（JTAG 信号源） | ⚠ 尽量避开 |
| **IO45** | Strapping（VDD_SPI 电压选择） | ⚠ 尽量避开 |
| **IO46** | Strapping（ROM 日志/启动模式） | ⚠ 尽量避开 |
| **IO19 / IO20** | 默认 USB_D- / USB_D+ | ⚠ 若不用 USB 功能可释放，但建议保留 |
| **IO43 / IO44** | 默认 UART0 TX/RX，上电打印 ROM 日志 | ⚠ 调试用，不建议改作他途 |

> ESP32-S3-MINI-1 共引出 **39 个 GPIO**（IO0~IO21，IO26~IO48 中模组实际引出的部分），其中部分被内部占用或限制。

---

## 二、功能分配表（ESP32-S3 作为 SPI 协处理器）

| 功能 | ESP32 引脚 | DevKitM-1 排针 | 方向 | 外设配置 | 说明 |
|---|---|---|---|---|---|
| **SPI2 从机 CS** | **IO10** | J1 附近 | ← STM32 | SPI2 从机 | IO MUX 默认 CS0，与 STM32 PB12 对接 |
| **SPI2 从机 SCK** | **IO12** | J1 附近 | ← STM32 | SPI2 从机 | IO MUX 默认 SCLK |
| **SPI2 从机 MOSI** | **IO11** | J1 附近 | ← STM32 | SPI2 从机 | IO MUX 默认 MOSI（ESP32 接收） |
| **SPI2 从机 MISO** | **IO13** | J1 附近 | → STM32 | SPI2 从机 | IO MUX 默认 MISO（ESP32 发送） |
| **SPI2 数据就绪 INT** | **IO14** | J1 附近 | → STM32 | GPIO 输出 | 可选，ESP32 有数据时通知 STM32 |
| **ESP32 复位** | **EN** | 板载按键 | ← STM32 PD15 | 复位引脚 | 建议开漏或串 1kΩ；不接也可用按键复位 |
| **SPI3 主机 SCK** | **IO36** | J3 | → 外设 | SPI3 主机 | GPIO 矩阵，速率 ≤40MHz |
| **SPI3 主机 MOSI** | **IO35** | J3 | → 外设 | SPI3 主机 | GPIO 矩阵 |
| **SPI3 主机 MISO** | **IO37** | J3 | ← 外设 | SPI3 主机 | GPIO 矩阵 |
| **SPI3 片选 CS0** | **IO34** | J3 | → 外设 | GPIO 输出 | 可作屏/传感器片选 |
| **SPI3 片选 CS1** | **IO33** | J3 | → 外设 | GPIO 输出 | 备用片选 |
| **调试串口 TX** | **IO43** | J1 | → | UART0 TX | 默认 ROM 日志输出，115200 |
| **调试串口 RX** | **IO44** | J1 | ← | UART0 RX | 默认 ROM 日志输入 |
| **状态 LED** | **IO2** | J1 | → | GPIO 输出 | 板载蓝色 LED（高电平点亮） |
| **STM32 复位请求** | **IO21** | J1 | → STM32 NRST | GPIO 输出 | 可选，ESP32 主动复位 STM32 |
| **启动按键** | **IO0** | J1 | ← | GPIO 输入 | 板载 BOOT 键，也可作普通输入（需注意 strapping） |
| **备用 GPIO 1** | **IO1** | J1 | 双向 | GPIO | RTC_GPIO1，触摸/ADC 可用 |
| **备用 GPIO 2** | **IO4** | J1 | 双向 | GPIO | RTC_GPIO4，触摸/ADC 可用 |
| **备用 GPIO 3** | **IO5** | J1 | 双向 | GPIO | RTC_GPIO5，触摸/ADC 可用 |
| **备用 GPIO 4** | **IO6** | J1 | 双向 | GPIO | RTC_GPIO6，触摸/ADC 可用 |
| **备用 GPIO 5** | **IO7** | J1 | 双向 | GPIO | RTC_GPIO7，触摸/ADC 可用 |
| **备用 GPIO 6** | **IO8** | J1 | 双向 | GPIO | 若板上有外部 Flash 需避开，DevKitM-1 一般可用 |
| **备用 GPIO 7** | **IO9** | J1 | 双向 | GPIO | RTC_GPIO9 |
| **备用 GPIO 8** | **IO15** | J1 | 双向 | GPIO | RTC_GPIO15，U0RTS |
| **备用 GPIO 9** | **IO16** | J1 | 双向 | GPIO | RTC_GPIO16，U0CTS |
| **备用 GPIO 10** | **IO17** | J1 | 双向 | GPIO | RTC_GPIO17，U1TXD |
| **备用 GPIO 11** | **IO18** | J1 | 双向 | GPIO | RTC_GPIO18，U1RXD，USB_SEL |
| **USB D-** | **IO19** | 板载 USB | — | USB OTG | 保留，与 Type-C 对接 |
| **USB D+** | **IO20** | 板载 USB | — | USB OTG | 保留 |
| **备用 GPIO 12** | **IO38** | J3 | 双向 | GPIO | 通用 IO |
| **备用 GPIO 13** | **IO39** | J3 | 双向 | GPIO | JTAG MTCK，可复用 |
| **备用 GPIO 14** | **IO40** | J3 | 双向 | GPIO | JTAG MTDO，可复用 |
| **备用 GPIO 15** | **IO41** | J3 | 双向 | GPIO | JTAG MTDI，可复用 |
| **备用 GPIO 16** | **IO42** | J3 | 双向 | GPIO | JTAG MTMS，可复用 |
| **备用 GPIO 17** | **IO47** | J3 | 双向 | GPIO | SUBSPICLK_P_DIFF，可用作普通 IO |
| **备用 GPIO 18** | **IO48** | J3 | 双向 | GPIO | SUBSPICLK_N_DIFF，可用作普通 IO |

> DevKitM-1 的排针位置：**J1 在开发板一侧，J3 在另一侧**。IO35~IO37 位于 J3 侧。

---

## 三、与 STM32H743 表的协同关系

这套引脚分配和你之前 STM32 表里的 **WiFi SPI2 预留** 直接对应：

| STM32 引脚 | ESP32 引脚 | 功能 |
|---|---|---|
| PB12 (51) | IO10 | STM32 → ESP32 CS |
| PB13 (52) | IO12 | STM32 → ESP32 SCK |
| PB15 (54) | IO11 | STM32 → ESP32 MOSI |
| PB14 (53) | IO13 | ESP32 → STM32 MISO |
| PA15 (77) | IO14 | ESP32 → STM32 INT（数据就绪中断） |
| PD15 (62) | EN | STM32 → ESP32 复位（可选） |

**ESP32 端 SPI2 从机配置建议：**

```c
spi_bus_config_t buscfg = {
    .mosi_io_num = 11,
    .miso_io_num = 13,
    .sclk_io_num = 12,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4096,
};
spi_slave_interface_config_t slvcfg = {
    .mode = 0,              // CPOL=0, CPHA=0
    .spics_io_num = 10,
    .queue_size = 4,
    .flags = 0,
};
```

**SPI3 主机（驱动屏/传感器）配置：**

SPI3 在 ESP32-S3 上**没有 IO MUX 默认引脚**，必须走 GPIO 矩阵。速率上限 40 MHz，对大多数屏幕和传感器够用。建议引脚：

```text
SCK  = IO36
MOSI = IO35
MISO = IO37
CS0  = IO34
CS1  = IO33（备用）
```

---

## 四、关键注意事项

1. **SPI2 走 IO MUX，SPI3 走 GPIO 矩阵**：SPI2 用 IO10~13 时驱动自动走 IO MUX，最高可靠时钟 **80 MHz**；SPI3 只能走 GPIO 矩阵，最高 **40 MHz**。

2. **Strapping 引脚**：IO0、IO3、IO45、IO46 上电时有固定电平要求。IO3/IO45/IO46 在本方案中**完全不使用**，IO0 仅作启动按键（板载已有上拉），不影响。

3. **IO26 不可用**：ESP32-S3-MINI-1 模组中 IO26 用于内部 PSRAM/Flash 的 SPICS1，做其他用途会导致启动失败。

4. **IO35~IO37 与 PSRAM 的关系**：在 **ESP32-S3-MINI-1-N8**（无 PSRAM）上，IO35~IO37 是普通 GPIO，可以自由使用；在 **-N4R2**（带 2MB PSRAM）上，IO35~IO37 可能与内部 PSRAM 共享，使用前需确认模组具体型号。DevKitM-1 板载的是 **ESP32-S3-MINI-1-N8**，没有 PSRAM，IO35~IO37 可用。

5. **电源**：STM32 板 5V → ESP32 DevKitM-1 5V 引脚，共地。ESP32 峰值电流较大（Wi-Fi 工作时 >300mA），确保 5V 供电路径足够粗。

6. **固件协议**：连线只是物理层。ESP32 端需要跑 SPI 从机固件。如果只是透传 WiFi 数据，可用 **esp-hosted** 或自定义简单帧协议（命令+长度+数据+CRC）。