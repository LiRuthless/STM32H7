# G3 控制拆分人工复核

2026-10-23。实现规约：8505d57；G2行为比较基线：e4c7f9b。
本记录是逐模块事实复核；R08暂存区摘要及依赖证据由本批提交者最后绑定。

## 源码事实

- tracking唯一持有完整拍状态及四个监视标量；原赋值点替换为标量setter，原读点替换getter。
  SetTrackOutput不修改完整拍track_out，前台预览不冒充完整控制拍。
- tracking的模式失败返回、串口文本、reset顺序、Select EM→Select optical→Tuning→Calculate
  EM→Calculate optical→发布观察顺序不变。执行口保持steering和output的不同用途。
- profile独占7项主表、4项环岛表、8000/10000距离门限。逐项比对原fields、权重、base、方向、
  角度常量，未选组不写。四个独立if和Merge的fields按位或保留。未知环岛状态返回TURN空项。
- direction无持久历史，四项表达式及history写点不变，先int16窄化再±1000限幅。
- angle唯一持有前拍误差，先写owner误差、再写输出、再保存历史。PID_CalculateAngle纯计算；
  legacy_control中的PID_UpdateAngle随后把负输出隐式转换到Motor整数参数，原副作用仍存在。
- speed唯一持有左右PI历史；换向清积分、保存目标、计算比例积分、限制积分、补死区、输出限幅、
  float→int32→int16顺序不变。PID façade唯一持有增益及可见输出，无传感器或硬件依赖。
- legacy_pid唯一持有旧共享方向历史和两轮各自增量式静态历史。ResetAll只清旧预览方向历史、
  角度历史及活动PI，不清增量历史。两模式影子历史仍分别由FSM复位。停车预览原调用继续保留。
- legacy_element八个入口与调用边保留；跷跷板进入时间仍窄化int16，两个退出if独立。
- legacy_roundabout保留判断/执行/复位；left/right/sign/三个保留角度合为同一私有对象。
  AbsFloat改为本模块static原分支；配置从profile读取，不复制表或新增setter。
- IMU owner保留s_imu、偏置和独立scalar历史；wrapper用栈指针数组传各自地址，legacy_imu
  无static状态。旧Gyro读取、raw_accel/14.3、gyro+accel历史积分和校准逗号清零均保留，
  不修复旧公式疑点。活动IMU更新和reset顺序不变。
- 活动调度没有新增Control_UpdateLegacy、角度执行、旧元素/环岛、休眠IMU调用。
  纯direction/angle/speed/legacy_pid/pid没有BSP、传感器、Motor或App依赖。

## 注释与格式

CS-01～15适用。C保留完整算法说明及来源，新增接口说明真实输入、输出及状态所有权；H用中文
用途摘要和实际C文件::函数定位。复核修正原环岛未知状态回退、档位setter副作用、跷跷板退出条件、
角度写入顺序及ResetAll方向历史归属的错误措辞。getter/单调用转发/reset简单赋值说明允许省略
details；不以此省略复杂公式和时序说明。全部新模块Doxygen正文有星号前缀。

## 实际验证

| 项目 | 结果 |
|---|---|
| LCD=0 / LCD=1 GCC | 均0警告0错误；text/data/bss分别90740/308/231736与93388/312/231752 |
| 配置/产物恢复 | app_config.h及受跟踪SmartCar.elf状态干净，构建finally原字节恢复 |
| app_layout_check | 67 C/H，32 C源；Keil每源一次，GCC五类扫描与fixture一致 |
| control_equivalence | Phase11原轨迹、浮点输出、7主表/4环岛表、格式检查通过 |
| 独立FSM/param/app | 全部原断言通过，历史FSM源码仍取911edae，未改历史路径或数值 |
| runtime_tuning | 完整拍生效、状态转移、历史、恢复、保存失败回归通过 |
| g3_control新增 | G2/当前角度执行调用数、参数、调用时owner状态逐值一致；左右增量式跨ResetSpeed/All历史一致；IMU校准/更新/reset逐值一致；纯角度无Motor调用 |
| g2_system | HAL包装、时钟/定时/LED/方向顺序、电池窄化与滤波、安全边界原测试通过 |
| datalog/scb/SPI | datalog_v2、scb_protocol 608例、scb_link、spi_wifi_state、spi_log_export通过 |

日志在MDK-GCC/build/phase17-g3-*.log。执行脚本曾复用本轮phase17-g2-*输出路径，已复制G3最终
结果至上述G3路径；没有覆盖用户初始未跟踪g3-lcd0/g3-lcd1/g3-native日志。
scb_protocol功能结果通过，一次进程退出清理临时文件有WinError32，未影响断言或返回码。
完整dual_mode_tracking仍受R07桌面原始采集文件缺失阻塞，独立测试不取消该原文件校验。
AC5、700/900μs和实车项目保持待验。
