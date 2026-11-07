# 独立板测软件交付验证（2026-11-06）

规约提交：0d60f8eebc9f1eb10cb4d5763ecc76faf3f842b9。实现提交号以本报告所属Git提交为准。

## 构建结果

15个测试选择在三个隔离源码快照中并行执行未改动的MDK-GCC/build.sh；NONE在最终工作区执行同一脚本。全部原生进程退出0，脚本遇警告或错误会退出非零；编译诊断检查未发现warning/error。
[构建输入摘要](isolated-board-test-build-inputs.json)记录冻结输入；快照后仅澄清BSP GPIO帮助函数的两行Doxygen，不改C语句，最终NONE构建覆盖该注释修订。
后台PowerShell作业对中文状态标题发生字符编码失真，保留原输出；日志末尾ASCII结果根据实际捕获的退出状态补记，不以乱码猜测成功。

| 选择 | 实际结果 | 日志 |
|---|---|---|
| BOARD_TEST_NONE (0) | 0错误/0警告 | [isolated-board-test-build-0.log](isolated-board-test-build-0.log) |
| BOARD_TEST_LCD (1) | 0错误/0警告 | [isolated-board-test-build-1.log](isolated-board-test-build-1.log) |
| BOARD_TEST_IND_HL (2) | 0错误/0警告 | [isolated-board-test-build-2.log](isolated-board-test-build-2.log) |
| BOARD_TEST_IND_VL (3) | 0错误/0警告 | [isolated-board-test-build-3.log](isolated-board-test-build-3.log) |
| BOARD_TEST_IND_VR (4) | 0错误/0警告 | [isolated-board-test-build-4.log](isolated-board-test-build-4.log) |
| BOARD_TEST_IND_HR (5) | 0错误/0警告 | [isolated-board-test-build-5.log](isolated-board-test-build-5.log) |
| BOARD_TEST_GRAY (6) | 0错误/0警告 | [isolated-board-test-build-6.log](isolated-board-test-build-6.log) |
| BOARD_TEST_GYRO (7) | 0错误/0警告 | [isolated-board-test-build-7.log](isolated-board-test-build-7.log) |
| BOARD_TEST_ACCEL (8) | 0错误/0警告 | [isolated-board-test-build-8.log](isolated-board-test-build-8.log) |
| BOARD_TEST_DL1B (9) | 0错误/0警告 | [isolated-board-test-build-9.log](isolated-board-test-build-9.log) |
| BOARD_TEST_ENCODER_LEFT (10) | 0错误/0警告 | [isolated-board-test-build-10.log](isolated-board-test-build-10.log) |
| BOARD_TEST_ENCODER_RIGHT (11) | 0错误/0警告 | [isolated-board-test-build-11.log](isolated-board-test-build-11.log) |
| BOARD_TEST_BATTERY (12) | 0错误/0警告 | [isolated-board-test-build-12.log](isolated-board-test-build-12.log) |
| BOARD_TEST_ADC_KEY1 (13) | 0错误/0警告 | [isolated-board-test-build-13.log](isolated-board-test-build-13.log) |
| BOARD_TEST_ADC_KEY2 (14) | 0错误/0警告 | [isolated-board-test-build-14.log](isolated-board-test-build-14.log) |
| BOARD_TEST_PC13 (15) | 0错误/0警告 | [isolated-board-test-build-15.log](isolated-board-test-build-15.log) |

非法选择16及UNKNOWN_TEST：用同一GCC的-fsyntax-only直接检查配置头，均按预期退出1（分别#error/负数组长度，以及未声明标识符）；不属于合法配置构建错误。
NONE最终体积：text=91432、data=308、bss=231688字节。未运行AC5或双LCD构建，没有新增宿主行为测试或模拟硬件。

## 审查与R08

[逐函数人工审查](../SmartCar/specs/features/02-bsp-platform/board-test-review.md)记录CS-01～15、注释覆盖/格式/内容、调用顺序、状态owner、错误及并发结论。
唯一HAL_TIM_PeriodElapsedCallback和TIM15_IRQHandler经源码定位确认，无重复强符号。最小ADC路径仅启用Stream0，SPI45补丁保留，原ADC DMA地址、D-Cache和计时器配置不改。
R08预检发现工作区CRLF与索引LF摘要差异、旧基线摘要及新增文件引用说明缺项，均已纠正；不修改检查器规则。最终暂存区检查退出0：8个变更范围文件、167条登记记录有效，见[R08结果](isolated-board-test-r08-2026-11-06.log)及Phase02 validation。

## 真实验收边界

未烧录、未接入VOFA+、未做任何板上验收。编译和审查不证明物理接线/方向/精度、灰度700μs、IRQ容差、串口连续吞吐或实际栈峰值。
IMU错误返回问题未修，坏读数仍不可识别；SPI/I2C阻塞位于高优先级ISR，同级SysTick不能保证故障超时退出。串口10ms为HAL超时参数，tick量化和ISR阻塞可能延长墙钟等待，TC也不证明PC收到。

完整宏/入口/ISR对照、切换和烧录步骤、五行屏字段及状态码、VOFA+接线/三通道设置、统计取证和待执行板测清单见[操作契约](../SmartCar/Docs/platform/isolated-board-tests.md#f33)。
