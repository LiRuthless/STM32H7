READY 给许可；TX / RX 缓冲在途期间不可复用。

BSP使用两个1024B AXI缓冲，GCC段固定在0x2407F000与0x2407F400，均32B对齐；当前D-Cache关闭，不是实现了缓存维护。回调不做CRC、JSON或阻塞等待。HAL_BUSY保留许可；启动错误消费此次许可并等待新沿。前台10ms超时或错误态中止，局部屏蔽DMA/SPI IRQ，READY IRQ继续计数。实际SPI时钟必须核算并测量，不能用旧估算频率证明吞吐。

源码与契约：SmartCar/BSP/bsp_wifi.c::BSP_WiFi_TryExchange；BSP_WiFi_Service；BSP_WiFi_ReleaseRx。