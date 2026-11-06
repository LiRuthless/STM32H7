768 B 批次降低服务频度；跨扇区先擦再写。

前台每次Datalog_Service最多8条，不无限追赶。LogStore_Write只是字节后端，跨页与懒擦由BSP_Flash_LogWrite负责。起跑只擦首个4KiB，维护s_log_frontier，写范围超出前沿时擦后续扇区，再逐页编程。普通批次768B可能从非页首开始，不能固定称每批恰好三次页写。96kB/s为记录负载，不包含擦除、命令、页编程等待等开销。

源码与契约：SmartCar/App/services/logger.c::Datalog_Service；SmartCar/BSP/bsp_flash.c::BSP_Flash_LogWrite。