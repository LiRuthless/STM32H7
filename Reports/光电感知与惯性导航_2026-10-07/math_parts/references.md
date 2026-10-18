## 7 参考资料与具体支持范围

链接资料于 2026-10-07 核验。公开文档和源码可能更新；报告代码为独立教学实现。以下列表说明采用的依据，实际结论附近也已放置链接。

| 资料 | 支持的结论 | 不应外推的结论 |
|---|---|---|
| [Pololu QTR 官方文档](https://www.pololu.com/docs/0j19/all)及[当前源码](https://pololu.github.io/qtr-sensors-arduino/_q_t_r_sensors_8cpp_source.html) | 标定后加权平均、条件单调、门限和丢线记忆的公开实现；旧文档示例 API 与当前源码区分 | 具体车速；任意间距均有效 |
| [Hamamatsu PSD 技术说明](https://www.hamamatsu.com/content/dam/hamamatsu-photonics/sites/documents/99_SALES_LIBRARY/ssd/psd_kpsd9001e.pdf) | 连续 PSD 模型的差除和与光强归一化 | 分立阵列严格线性位置；循迹车速度 |
| [Pololu QTR AO](https://www.pololu.com/docs/0J13/3)、[RC](https://www.pololu.com/docs/0J13/2) | 两类读出原理、厂家测量波形、安装高度与噪声影响 | 所有光电模块极性相同 |
| [DFRobot SEN0017](https://wiki.dfrobot.com/sen0017/) | 特定模块黑 LOW 白 HIGH 的一位输出例 | 所有数字模块都相同 |
| [MathWorks PID](https://www.mathworks.com/help/simulink/slref/pidcontroller.html)与[抗饱和示例](https://www.mathworks.com/help/simulink/slref/anti-windup-control-using-a-pid-controller.html) | 离散采样、导数滤波、条件积分/回算思路 | 本例参数的实车稳定性 |
| [Pololu 3pi PID 实现](https://www.pololu.com/docs/0J21/7.c) | 从偏差到 PID 和差速输出的公开工程流程 | PWM 255 等于某个 m/s |
| [VectorNav AHRS](https://www.vectornav.com/resources/inertial-navigation-primer/theory-of-operation/theory-ahrs) | 重力倾角、动态加速度的限制和航向参考需求 | 六轴 IMU 自动获取绝对航向 |
| [VectorNav IMU 标定](https://www.vectornav.com/resources/inertial-navigation-primer/specifications--and--error-budgets/specs-imucal) | 偏置、比例、轴误差、六面/温度标定 | 一次单姿态求全加速度标定 |
| [x-io Fusion 官方工程](https://github.com/xioTechnologies/Fusion) | 四元数姿态融合、反馈增益、观测拒绝和去重力输出的可核验工程 | 本报告简化滤波达到同样性能 |
| [Dellaert 与 Hutchinson 开放教材 5.2](https://www.roboticsbook.org/S52_diffdrive_actions.html) | 差速车运动学及无滑动模型 | 阿克曼车直接用轮差转角 |
| [robot_localization 配置教程](https://raw.githubusercontent.com/cra-ros-pkg/robot_localization/rolling-devel/doc/configuring_robot_localization.rst) | 相同轮式来源的位姿速度相关性，避免重复信息 | 增加 EKF 即可消除不可观测漂移 |
| [Borenstein 与 Feng Gyrodometry ICRA 1996](https://www.cs.cmu.edu/~motionplanning/papers/sbp_papers/integrated2/borenstien_gyro.pdf) | 轮式误差与 gyro/里程计互补的原始工程研究 | 旧平台精度就是本代码精度 |
| [VectorNav INS 原理](https://www.vectornav.com/resources/detail/what-is-an-ins)、[GNSS/INS 原理](https://www.vectornav.com/resources/inertial-navigation-primer/theory-of-operation/theory-gpsins) | 积分漂移、外部观测约束的机制 | 无外部信息仍可永久准确定位 |
| [Woodman 剑桥技术报告 UCAM-CL-TR-696](https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-696.pdf) | MEMS 捷联惯导的误差传播与漂移分析，作主 agent 交叉核验 | 把报告中某 IMU 的实验数值移作本例实测 |

## 8 协作与复核说明

主 agent 先统一约定；A 负责两种光电公式与依据，B 负责数据类型与闭环流程，C 负责惯导与记录修正。第一轮完成后冻结接口，B 与 C 分别实现独立模块，主 agent 编写集成、日志与验证并统一正文。最后由未参与代码编写的 A 独立复核公式、引用、单位、代码与覆盖范围，主 agent 处理其发现后交付。独立复核的两项修订均已关闭；修复后的 30 项验证由主 agent 与复核 agent 分别运行并通过。具体检查结论见验证节。

