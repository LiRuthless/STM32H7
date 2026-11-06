从 1 ms 采样到 Flash、实时上传与停车导出。

本次评审围绕当前 STM32 端实现展开，建立数据生成、持久化、上传与导出的完整心智模型。重点审查并发所有权、发布与释放顺序、异常时保留的数据，以及软件证据和硬件证据的区别。ESP32 与电脑网络部分按公共契约说明，不能将其当成本仓已完成的桥接固件。

源码与契约：SmartCar/Docs/services/recording-and-export.md；SmartCar/Docs/services/communication.md。