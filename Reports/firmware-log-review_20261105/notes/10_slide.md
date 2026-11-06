单次运行追加记录；新起跑覆盖上一会话。

W25Q64总容量8MiB，日志区0..0x7FEFFF，共8384512B，减256B头再按96B整除，容量87336条。剩余不可整除字节不用作记录。第87336条被接受后触发容量满停车，不等待下一条才停车。上电不扫描旧Flash来恢复有效记录条数，Flash非易失不等于应用具有掉电恢复能力。地址示意不按实际空间比例绘制。

源码与契约：SmartCar/BSP/bsp_flash.h；SmartCar/App/services/datalog.h；SmartCar/App/services/logger.c::Datalog_Push。