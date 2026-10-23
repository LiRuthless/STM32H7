# Phase 17 验证

状态：G0编制中；G1～G6未实施。下述为验收标准，不是通过结果。

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
