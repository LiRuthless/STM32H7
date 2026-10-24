# Phase 17 验证

状态：G0～G3已提交；G4实现与本组门禁已通过，待本批提交；G5规约已提交、产品实现待启动；G6工具/文档准备中，最终验收未执行。下表是验收标准，实际结果见后文。

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

## G4 实际结果（2026-10-24）

- G4-V01/02：bank、镜像及system参数应用已分离；原v1/v2/v3布局/CRC/四bank/失败门禁通过。
  g4_hmi使用6125495和当前真实C逐条比较LCD调用与保存镜像；vp/vi活动PI与镜像在
  2999/3000ms边界的差异保持。人工内容/逐函数清单见g4-review.md。
- G4-V03/04/05：runtime_tuning、control_equivalence、scb_protocol608、scb_link、
  datalog_v2、spi_wifi_state、spi_log_export、g2_system、g3_control原入口全部通过。
  独立codec仅增编真实tuning_values.c；原断言未删改放宽。
  g4_transactions新增final占用、事件时点、独立成功值、会话/保护取消及两种revision回绕通过。
- G4-V06：原参数/key子项通过；新增g4_hmi的严格阈值/第二路覆盖、BACK旧位置保存、
  RST当拍早退/下拍清屏、松键保存、两模式两档及风扇钳位历史调用/字节对照通过。
- G4-V07：双LCD GCC均0警告0错误；text/data/bss为91656/308/231688、94220/312/231704。
  app_layout_check确认82个C/H、39个App C在Keil各一次；新拆分103个C定义的标签/EOF/行宽
  无缺项，人工内容、getter/转发类别和例外逐项见g4-review。R08暂存区绑定/提交前检查由提交方执行。
  对G3本批ARM编译帧对比72/118个函数，最大均232B；具体变化见review，完整调用链预算留G6。
- G4-V08/09：g4_boundaries将起停事件全部语句逐token对历史比较一致，Link仅单调用转发，
  mode协调门禁/写入/reset顺序与唯一setter通过；原跨mode、运行BUSY、保存失败及新增非法/
  同mode不早退真实入口断言全部通过。system自身/新header无直接SCB；datalog.h传递闭包留G5/G6。
- 独立FSM/参数/App子项全部通过；完整dual入口仍因C:/Users/Li/Desktop/LQ_Track.c缺失阻塞，
  R07原SHA256断言保留。AC5、700/900us、波形、Flash吞吐及实车继续待验。
- 日志phase17-g4-*。最后构建finally回写ELF遭Windows OSError22后，已用HEAD二进制blob
  恢复并逐字节验证；app_config与受跟踪ELF均恢复且Git状态干净。忽略产物不纳入提交。
- G4-09闭包：2ee3401先行规定保留休眠包装原版本发布副作用；两次自动审批拒绝后停止，
  用户于2026-10-24明确回答“允许，保持原有副作用”，正常受审补丁随后落盘，未启用入口。
  真实g4_legacy_boundary补丁前`TGPORE != TGPOSRE`失败证据保留；补丁后成功/故障精确
  同步顺序及保护优先级通过。runtime_tuning/control_equivalence/g3_control、事务/边界和
  独立FSM/参数/App均复验通过，日志phase17-g4-post-approval-*。
  最终同版双LCD0/1均零警告，体积与前述G4相同；finally逐字节恢复原配置及ELF/HEX/BIN。
  ARM补验原/当前休眠包装栈帧分别16B/16B，角度包装8B/8B；无新增产品活动调用。

- G4暂存区R08实际通过：以2ee34017e5bc42d7f1bb824e7704c176a74456ab为基线，35个变更C/H、144份当前/历史复核记录及依赖摘要一致；人工内容见g4-review，不以门禁代替语义复核。
