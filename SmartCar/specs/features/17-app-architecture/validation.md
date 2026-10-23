# Phase 17 验证

状态：G0/G1已提交；G2已提交；G3实现已验证待提交，G4/G5规约已提交而实现未启动；G6未实施。下表是验收标准，实际结果见后文。

| 项 | 需求 | 验证方法与唯一通过标准 |
|---|---|---|
| V-01 | FR-01/02/10 | 全量文件/函数/对象清单可定位；G1映射前后除include/保护宏外token完全一致 |
| V-02 | FR-03/12 | LCD=0和1各执行MDK-GCC/build.sh，均0警告0错误；Keil源清单存在、无重复、覆盖全部App C |
| V-03 | FR-05/06/12 | control_equivalence与dual_mode_tracking原断言不删减；逐拍状态输出一致 |
| V-04 | FR-07/12 | runtime_tuning、参数版本迁移原断言通过 |
| V-05 | FR-08/09/12 | datalog_v2、scb_protocol、scb_link、spi_wifi_state、spi_log_export通过 |
| V-06 | FR-04/11 | App无HAL/CubeMX头/寄存器访问；状态写入有唯一归属；无新增反向依赖 |
| V-07 | FR-13 | CS-01～15和R08按组检查；源码/依赖摘要一致；历史记录保留 |
| V-08 | FR-11/12 | GCC RAM/栈与基线比较；AC5构建及硬件700/900us/波形/吞吐/实车另验 |

R07外部文件缺失导致的完整双模式阻塞保持记录；不能删除断言或使用当前实现自证替代历史算法。
未执行项目写“未执行”，不能用GCC代替AC5、宿主代替实机。

## G1 实际结果（2026-10-23）

- V-01：app_layout_check.py --base 4745aa2339209d5df9a739449685fc7f1cb66429 通过；44个C/H包含前缀规范化后字节保持，21个App C在Keil各一次，五类分组齐备。
- V-02：LCD=0/1 GCC均零警告零错误；0配置text/data/bss=90556/296/231744，1配置=93220/300/231760；配置恢复0。
- V-03～05：control_equivalence、datalog_v2、runtime_tuning、scb_protocol（608项）、scb_link、spi_wifi_state、spi_log_export通过。dual_mode_tracking完整入口因原C:/Users/Li/Desktop/LQ_Track.c缺失阻塞；gray已运行，fsm/param/app独立子项通过。
- V-07：CS-01～15按G1适用范围检查：只移动完整文件和include，保留名称、函数体、注释、布局和预处理；未借本批修复继承内容缺陷。R08证据按新旧路径分别登记；旧记录保留。
- Phase16静态契约通过；AC5和实机未执行。G2～G6尚未实施，本阶段未完成。
- 日志：MDK-GCC/build/phase17-g1-*，不是自动纳入提交的产物。

## G2 实际结果（2026-10-23）

- G2-V1：最终LCD=0/1均零警告零错误；text/data/bss分别90764/296/231736和93428/300/231752，较G1静态BSS减少8B、text增加208B；配置与受跟踪ELF原字节恢复。
- G2-V2/3/6：control_equivalence、datalog_v2、runtime_tuning、scb_protocol、scb_link、spi_wifi_state、spi_log_export通过。独立fsm/param/app子项全部通过；App测试保持顶部检查后中断注入、全部停车与再次起跑断言。完整dual入口仍被R07原始文件缺失阻塞。
- G2-V4/5：g2_system真实BSP包装、ADC窄化/滤波、低压严格边界/201次、短路读次数和NONE无副作用测试通过。
- G2-V7：53个C/H结构、25个Keil源各一次检查通过；App无可执行HAL/CubeMX/main依赖及g_app残留；Phase16静态契约通过。人工逐项复核见g2-review.md，暂存区R08记录绑定本组源码与依赖。
- 日志：MDK-GCC/build/phase17-g2-final-*。AC5、栈峰值与700/900us/波形/吞吐/实车未执行，不用text/BSS结果替代。
- G0/G1/G2已完成软件可执行部分，G3～G6未实施；全阶段继续未完成。

## G3 实际结果（2026-10-23）

- 逐模块/注释/状态复核见g3-review.md；控制器与profile/legacy隔离已实现，完整拍与监视字段没有合并。
- LCD=0/1 GCC均零警告零错误；text/data/bss=90740/308/231736和93388/312/231752；配置及受跟踪ELF恢复。
- 67个C/H、32个App源的布局/Keil唯一编译检查通过。
- control_equivalence、runtime_tuning、g2_system、datalog_v2、scb_protocol(608项)、scb_link、spi_wifi_state、spi_log_export及独立FSM/参数/App子项通过。
- g3_control新增真实G2/当前对照通过：角度执行Motor调用/参数/当时状态，增量左右历史跨reset，IMU校准/更新/reset及纯角度无Motor动作。
- 日志phase17-g3-*；R07完整入口原始文件、AC5、栈峰值和实机预算继续待验。G4～G6仍未完成。
