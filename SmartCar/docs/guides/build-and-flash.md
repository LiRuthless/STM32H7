# 构建、烧录与再生成

现行功能说明，核对基线 `68cb508`；2026-10-31迁移。需求经[功能索引](../traceability.md)进入本页，再由实现表定位源码。
当前行为不等于全部已批准/实机验收，冲突见[已知问题](../known-issues.md)。

<a id="f30"></a>
## F30 构建烧录与再生
### 工具入口与实际前提

仓库根用Git Bash：`bash SmartCar/MDK-GCC/build.sh`；脚本按自身路径定位源码，当前EIDE GCC/objcopy/size/ar硬编码个人安装目录，换机必须核对实际安装，不擅自升级。正式扫描Core、App根及五类、BSP/lcd和HAL；tests/build副本不参与正式源码扫描。
产物ELF/HEX/BIN在MDK-GCC，build中日志/临时源码是证据或一次性脚本，不应批量执行或纳入固件。`bash SmartCar/MDK-GCC/flash.sh`会先构建再OpenOCD烧录，需用户任务授权及ST-Link接线/供电，不能当只读命令。
Keil打开SmartCar.uvprojx，锁定AC5，超评估32KB需正式授权；GCC通过不替代AC5。没有已维护ARMCLANG迁移入口。

### CubeMX与兼容

再生需同步IOC/生成配置、保留USER CODE/PLL3与SPI45SEL恢复、BSP接管边界和App调用。实际GCC_Min_Stack_Size及KeilStack_Size均0x2000，IOC ProjectManager.StackSize仍0x400；D05为再生成回退风险，不是当前运行栈1KB。后续独立配置任务先规约，不能本轮运行codegen。
当前配置零警告构建仅固件/工程改动强制；本次Markdown核对不构建。双LCD/资源按需，不覆盖历史产物；修改宏时恢复原字节。

**需求来源：** [P16 FR-23](../../specs/features/16-spi-wifi-link/requirements.md)；[P17 FR-03](../../specs/features/17-app-architecture/requirements.md)。实现入口：[MDK-GCC/build.sh](../../MDK-GCC/build.sh)、[MDK-GCC/flash.sh](../../MDK-GCC/flash.sh)、[MDK-GCC/STM32H743VITx.ld](../../MDK-GCC/STM32H743VITx.ld)、[MDK-ARM/SmartCar.uvprojx](../../MDK-ARM/SmartCar.uvprojx)、[MDK-ARM/startup_stm32h743xx.s](../../MDK-ARM/startup_stm32h743xx.s)、[SmartCar.ioc](../../SmartCar.ioc)。

## 本页实现与模块反查

下表是当前源码定位；公共/私有辅助函数归所属功能，不把声明视为另一实现。Core生成部分只作边界，用户区按原标记维护。

| 文件 | 主功能 | 定义/配置定位 |
|---|---|---|
| [MDK-ARM/SmartCar.uvprojx](../../MDK-ARM/SmartCar.uvprojx) | 本页跨模块协作 | 工程配置；本轮只读 |
| [MDK-ARM/startup_stm32h743xx.s](../../MDK-ARM/startup_stm32h743xx.s) | 本页跨模块协作 | 工程配置；本轮只读 |
| [MDK-GCC/STM32H743VITx.ld](../../MDK-GCC/STM32H743VITx.ld) | 本页跨模块协作 | 工程配置；本轮只读 |
| [MDK-GCC/build.sh](../../MDK-GCC/build.sh) | 本页跨模块协作 | 工程执行入口；核对工具环境及授权 |
| [MDK-GCC/flash.sh](../../MDK-GCC/flash.sh) | 本页跨模块协作 | 工程执行入口；核对工具环境及授权 |
| [SmartCar.ioc](../../SmartCar.ioc) | 本页跨模块协作 | 工程配置；本轮只读 |
