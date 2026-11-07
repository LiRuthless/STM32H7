# Phase02 独立板测人工审查（2026-11-06）

规约基线：0d60f8e；构建和R08结果另记validation。本文件记录人工内容判断，不是板测通过证明。

## CS-01～15及注释复核

- CS-01/02：新增App/system和BSP C/H、Core USER CODE及原BSP回调为本批范围；Core生成区/第三方不整理。
- CS-03～05：新增代码4空格、独立括号、一声明一对象、行宽≤100；既有仅改明确区段，保留原换行。协议格式串属于允许例外。
- CS-06～09：BoardTest及BSP_BoardTest前缀；新类型board_test_*_t、静态s_*、配置BOARD_TEST_*；HAL/IRQ/MX固定名保留；App通过公开BSP接触硬件。
- CS-10～12：C文件说明包含owner/数据流/上下文；全部新函数有C说明；有参数、返回者说明其实际含义，H只摘要及定位。配置头仅说明编号与校验，不伪造运行流程。
- CS-13～15：无休眠算法增删或启用；Core生成固定名称不改；NONE原启动顺序与回调执行体保留，测试分支与正常故障策略独立。此项以差异和调用路径审查为证据，不等于硬件等价实测。

## 新函数逐项内容审查

| 定义 | 覆盖/格式 | 人工内容与调用链结论 |
|---|---|---|
| BoardTest_UpdateWindow | C完整 | 同一采集上下文持有窗口；跨一秒先发布旧极值，首窗口占位，按键按真实前台次数 |
| BoardTest_BeginCapture/EndCapture | C完整 | 首拍不记间隔；DWT逐拍无符号差及64bit累积；耗时不包含向量/HAL分派，外测补全 |
| BoardTest_ControlISR | C完整 | 只读选中电感或灰度或DL1B；灰度临时完整帧成功才复制；DL1B先Update再Get，未就绪计无效 |
| BoardTest_TaskISR | C完整 | Battery_Update后读原码/滤波缓存，不低压停车、不喂狗；5ms原周期 |
| BoardTest_SampleISR | C完整 | 单轮唯一读清并累计；IMU读三轴后判满，满样本计溢出，停采但不等待串口；槽的三轴逐项volatile写入先于head发布；不通过去除volatile的memcpy访问共享对象 |
| BoardTest_ServiceUpload | C完整 | float宽度断言，位模式显式拆小端；独立静态16字节缓冲；成功才tail发布，异常停采停发并保留证据 |
| BoardTest_DrawRow | C完整 | 只绘20字符，短字符串尾部补空格，绘图不关中断 |
| BoardTest_FormatPulses | C完整 | uint64幅值覆盖INT64_MIN；十进制反序展开，21字节容量满足19位加符号及结尾 |
| BoardTest_Display | C完整 | 通过保留volatile语义的结构体赋值在短临界区复制一致快照；格式化和SPI绘图在外；灰度4×4及状态行、失败颜色、首帧占位；不重读硬件 |
| BoardTest_RunSelected | C完整 | 状态先清再初始化外设，窗口时基及Battery_Init先于启动；IMU分支无LCD，键盘只在前台；错误不进正常业务 |
| RunLcd/InductanceHL/InductanceVL/InductanceVR/InductanceHR/Gray/Gyro/Accel/Dl1b/EncoderLeft/EncoderRight/Battery/AdcKey1/AdcKey2/Pc13（均BoardTest_前缀） | 每个C定义均完整 | 每项有独立上电入口；编译选择作为前置条件，委托唯一公共实现；不提供运行时切换，不返回 |
| BoardTest_Run | C完整 | 编译选择唯一入口；意外返回永久停留，main再次隔离 |
| BSP_BoardTest_InitPins | C完整 | 端口时钟由调用方先开，仅GPIO配置，不引入EXTI |
| BSP_BoardTest_Init | C完整 | SPI片选先释放；按需MX，不调用整体GPIO；单轮HAL启动并检查；IMU无LCD、UART仅TX且无接收业务；ADC原配置/位置及Stream0原优先级2，不启用Wi-Fi Stream1/2；最终恢复SPI45；未改旧驱动无返回接口的失败语义 |
| BSP_BoardTest_StartTimer/StopTimer | C完整 | 所选TIM6/7分别起停，启动前清UIF及NVIC挂起；TIM15复用原起停，测试停采另禁用NVIC并清UIE/UIF/挂起，防止停止后补采；不执行综合采集体；HAL可检测启动错误上报 |
| BSP_BoardTest_SendFrame | C完整 | READY检查避免旧TxXferCount；HAL_OK且TC才完整；部分提交不表示线路完成，10ms依赖HAL时基，不假称PC确认 |

## 既有入口及依赖增量

- main：仅USER CODE新增包含和SysInit分流；PLL3补丁保留，NONE后续MX顺序与App_Init不变。
- HAL_TIM_PeriodElapsedCallback：唯一强符号保留；NONE仍调App控制/辅助ISR，测试只调板测分派。
- TIM15_IRQHandler：唯一向量保留；测试检查UIF/UIE后清UIF，仅采所选对象；NONE原读清、累计、两组IMU及App钩子顺序未更动。
- BSP ADC/Key/DL1B及对应H：驱动源码未改；main依赖变化只增加测试入口，普通初始化调用仍可达，D2缓冲/D-Cache/XS/PC13语义不变；测试新调用方按其旧契约使用，旧缺陷不关闭。
- imu_proc、motor：NONE采样缓存与累计消费接口未改；板测不调用姿态校准/积分或电机控制，单轮由测试BSP启动、TIM15读清。
- lifecycle、safety、scheduler：NONE原起跑/停车/喂狗/风扇及日志链未改；测试在其初始化前分流，不借用原整体定时启停。
- command_dispatch、tuning_application、run_record：NONE的tick/getter和发布顺序保留；测试不初始化也不服务这些业务，不能把测试head作为正常日志tick。

## 边界

IMU读取错误不可识别、同级SysTick无法保障SPI/I2C故障超时、ADC/LCD既有无反馈初始化、AC5授权构建及全部实机时序/电气/吞吐验收均未补证。观察结束按累计DWT入口间隔达到60秒周期数，仍需对照外部实测时间；DWT采集函数耗时不代表完整IRQ占用。没有新增宿主测试或模拟硬件脚手架。
