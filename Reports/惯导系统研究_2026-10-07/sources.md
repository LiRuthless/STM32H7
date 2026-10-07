# 文献与公开案例索引

检索和阅读日期：2026年10月7日。编号与报告中的 S01–S18 一致。以下记录区分论文年份、实际读取的稿本版本、公开代码版本；代码能证明实现结构，不能替代独立实车性能数据。章节与表号按所链接版本，不按期刊排版版自动换算。

代码commit日期按GitHub原始UTC记录；论文日期保留出版/存档记录。报告研究日期使用北京时间。

全文阅读的含义：获取并阅读可检索全文，重点复核正文所用公式、实验方法及结果位置；不是仅依据摘要。PDF原件与文本暂存于本次研究缓存，下载状态、PDF页数、SHA-256和代码commit记录在 `source_manifest.json`。Mahony与Coulter本地下载失败，但联网全文工具成功读取；清单中的下载失败不表示只能访问摘要。未实际运行任何作者项目、训练模型或复跑公开数据集。报告内自编数值算例另行标注。

## S01：惯导基础与误差传播

**Oliver J. Woodman. _An introduction to inertial navigation_. 2007.** University of Cambridge Computer Laboratory Technical Report UCAM-CL-TR-696，37页。[官方报告](https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-696.pdf)

- 访问：已下载和阅读全文；技术报告未采用未经核验的DOI。
- 定位：§2捷联架构；§3–§4传感器及误差；§5 Allan方差；§6导航解算、静止日志漂移；§7噪声模拟；§8.1磁观测修正。表2/3列误差源，图19–21展示误差模拟，§8.1给出60 s后平均位置误差5.21 m。
- 支持：初始姿态、比力变换、重力泄漏和积分漂移的解释；特定Xsens Mtx示例中60 s超过150 m以及磁观测改善的结果。
- 不支持：当代所有IMU统一精度、车辆实测性能或无辅助长期精确定位；作者2007年技术判断不作为2026年器件上限。

## S02：Mahony互补滤波

**Robert Mahony, Tarek Hamel, Jean-Michel Pflimlin. _Nonlinear Complementary Filters on the Special Orthogonal Group_. 2008.** IEEE Transactions on Automatic Control 53(5)。[DOI：10.1109/TAC.2008.923738](https://doi.org/10.1109/TAC.2008.923738)，[实际读取的16页作者稿](https://www.mikrocontroller.net/attachment/197058/2007_Mahony.etal_TAC-06-396_v3.pdf)

- 版本/访问：稿件2007年修订，HAL hal-00488376 v1于2010年存档；本地403，联网全文读取成功。
- 定位：§II测量模型；§III直接/被动滤波；§IV在线bias与稳定性；§V公式(32)显式滤波；§VI实验；附录B四元数形式。
- 支持：方向残差反馈、gyro bias估计及条件化稳定性；不支持任意加速/磁干扰下的全姿态收敛或车辆位置精度。只有重力时不能套用完整航向可观测结论。

## S03：四元数与ESKF基础

**Joan Solà. _Quaternion kinematics for the error-state Kalman filter_. 2017.** [arXiv:1711.02508v1](https://arxiv.org/abs/1711.02508v1)，[全文](https://arxiv.org/pdf/1711.02508v1)

- 访问：已下载和阅读95页全文；封面日期2017-11-08，arXiv v1记录为2017-11-03。
- 定位：§3四元数约定；§4.6角速率积分；§5.3–5.4真值、名义与误差状态动力学；§6.1观测、§6.2注入、§6.3重置；附录A中点/RK4、附录B–E协方差与随机扰动离散化。
- 支持：姿态乘法顺序、IMU预测、误差雅可比、注入与重置结构；本文平面八状态模型是按这些原则重新推导的模型，未称为原文轮式算法。此数学文献不提供本文车辆控制实测结论。

## S04：多姿态IMU标定

**David Tedaldi, Alberto Pretto, Emanuele Menegatti. _A Robust and Easy to Implement Method for IMU Calibration without External Equipments_. 2014.** IEEE ICRA。[作者公开全文](https://albertopretto.altervista.org/papers/tpm_icra2014.pdf)

[DOI：10.1109/ICRA.2014.6907297](https://doi.org/10.1109/ICRA.2014.6907297)。

- 访问：已下载和阅读8页全文。
- 定位：§II轴误差/比例模型；§III式(10)/(13)加速度与gyro拟合；§IV采集协议；§V合成与实际数据；表VIII–XII、图5为Xsens实际结果。
- 支持：重力范数约束、多静态姿态、旋转期间积分的标定；原文要求至少9种姿态，实际约50 s初始静止、37次旋转及2–4 s静止间隔。工厂参数是实测对照，坐标转换和舍入有影响。
- 不支持：完整安装外参、时钟、温度迟滞或绝对yaw可由该手持流程全部解决。其bias符号与本报告减法定义不同，需转换。

## S05：陀螺与轮程按故障机理融合

**Johann Borenstein, Liqiang Feng. _Gyrodometry: A New Method for Combining Data from Gyros and Odometry in Mobile Robots_. 1996.** IEEE ICRA，pp.423–428。[公开论文原件](https://www.cs.cmu.edu/~motionplanning/papers/sbp_papers/integrated2/borenstien_gyro.pdf)

- 访问：已下载和阅读7页作者版本。
- 定位：§2误差机理及gyro；§3融合方法；§4实测与图2–4；表1障碍物。
- 支持：gyro/轮差瞬时角增量分歧大时切换信息源。LabMate+Murata Gyrostar在约0.1 m/s、140 s、14 m直线上单侧越过15个6/9/12 mm障碍，墙面双超声给航向参考（约±0.25°）；正文给融合误差通常约±0.5°内。
- 不支持：高速度、任意地形或双轮同步空转检测；参考误差不可忽略。

## S06：Wheel-INS轮上惯导

**Xiaoji Niu, Yibin Wu, Jian Kuang. _Wheel-INS: A Wheel-mounted MEMS IMU-based Dead Reckoning System_. 2021.** IEEE Transactions on Vehicular Technology。[实际读取arXiv v4](https://arxiv.org/pdf/1912.07805v4)，[作者代码固定版本](https://github.com/i2Nav-WHU/Wheel-INS/tree/1a3cc22f61b058612e8e69f6ecdcf5ec1dc95b5f)

[DOI：10.1109/TVT.2021.3108008](https://doi.org/10.1109/TVT.2021.3108008)，由作者仓库书目核验。

- 版本/访问：已读v4全文12页；早期v2的WM-INS标题与结果不替代v4。代码commit `1a3cc22f61b058612e8e69f6ecdcf5ec1dc95b5f`，2025-01-25；读README、`config/robot.yaml`、`dataset/ReadMe.pdf`，未编译或运行代码。
- 定位：§II轮上安装及误差；§III轮速/NHC/ESKF；§IV实验。表I器件、表II轨迹、表III位置漂移率和航向误差，§IV.B明确从起点累积100 m段的指标定义。
- 支持：轮转调制、轮上gyro生成轮速、条件化轮地约束；表III六组数字及平均位置/航向改善23%/15%。Test 2位置和航向均未占优。
- 不支持：每时刻绝对误差≤路程×1.8%、任意三维地形或车体IMU等效效果；真值初始位置/yaw、静态bias及轮参数条件须保留。公开两套例数据不等于完整六试验复现记录。

## S07：AI-IMU惯性航位推算

**Martin Brossard, Axel Barrau, Silvère Bonnabel. _AI-IMU Dead-Reckoning_. 2019预印本，2020 IEEE Transactions on Intelligent Vehicles.** [读取的arXiv v1](https://arxiv.org/pdf/1904.06064v1)，[作者代码固定版本](https://github.com/mbrossar/ai-imu-dr/tree/32967812139af82cf12654c93438bf9065b7a241)

[期刊DOI：10.1109/TIV.2020.2980758](https://doi.org/10.1109/TIV.2020.2980758)。性能定位采用实际阅读的v1，而非推测期刊版表号。

- 访问：已读10页全文和固定README；commit `32967812139af82cf12654c93438bf9065b7a241`，2025-01-08；未训练或执行。
- 定位：§III状态与侧向/竖向约束；§IV不变EKF和网络协方差；§V.A评价、§V.B逐序列排除训练及真值初值；表1平均平移1.10%，§V.C/表2异常数据。
- 支持：100 Hz KITTI IMU、车体约束、网络动态调R和条件化实测效果。原文每条被测序列训练400 epochs且排除该序列，R₀/v₀/p₀来自真值。
- 边界：固定README预训练参数使用00、01、04–11，说明只有02是测试序列；不能在训练序列上宣称独立复现原文。在线只输入IMU不等于无先验、无训练真值的自由运动纯INS。

## S08：Wheel-GINS组合导航

**Yibin Wu, Jian Kuang, Xiaoji Niu, Cyrill Stachniss, Lasse Klingbeil, Heiner Kuhlmann. _Wheel-GINS: A GNSS/INS Integrated Navigation System with a Wheel-mounted IMU_. 2025.** [arXiv:2501.03079v1全文](https://arxiv.org/pdf/2501.03079v1)，[作者项目](https://github.com/i2Nav-WHU/Wheel-GINS)

- 访问：已下载和阅读13页v1预印本；项目链接仅指出作者实现，未固定/审阅该项目代码，不作为源码结论依据。
- 定位：§II坐标与参数；§III GNSS位置和运动约束模型、安装与轮径在线估计；§IV实测；表II器件、表III轨迹、表IV连续GNSS结果、表V人工中断、图9–10安装角估计。
- 支持：三平台三维GNSS/Wheel-IMU组合；30/60/120 s人工失锁平均RMSE减少28/32/37%，报告列出的120 s三平台数值均取表V。每条序列两处失锁。
- 边界：后处理PPK GNSS位置作为观测，初始位置/yaw来自参考，两系统共用GNSS；不是普通实时单点GPS或冷启动纯INS效果。未实验本报告运动控制器。

## S09：Stanley完整车辆工程

**Sebastian Thrun, Mike Montemerlo, Hendrik Dahlkamp等. _Stanley: The Robot that Won the DARPA Grand Challenge_. 2006.** Journal of Field Robotics 23(9):661–692。[DOI：10.1002/rob.20147](https://doi.org/10.1002/rob.20147)，[作者公开全文](https://robots.stanford.edu/papers/thrun.stanley05.pdf)

- 访问：已下载和阅读32页全文。标题中的赛事发生于2005年；论文为2006年。
- 定位：§2车辆及传感器；§3软件、时戳和日志；§4 UKF状态估计；§8路径规划；§9控制，式(7)、图24横向控制及稳定性，速度PI与转向动态补偿；§10赛事结果。
- 支持：惯性/GPS/CAN联合状态、制导与控制分层、20 Hz转向、前轴参考点Stanley纠偏结构，以及约132 mile赛事完成。
- 不支持：仅靠惯导完成赛事、定位模块独立RMSE或只用式(7)即可复现完整车辆控制；本文低速正则项不是原论文式(7)。

## S10：Pure Pursuit原始工程报告

**R. Craig Coulter. _Implementation of the Pure Pursuit Path Tracking Algorithm_. 1992.** CMU-RI-TR-92-01。[CMU官方全文](https://publications.ri.cmu.edu/storage/publications/pub_files/pub3/coulter_r_craig_1992_1/coulter_r_craig_1992_1.pdf)

- 访问：本地下载证书失败，联网全文读取15页成功；正文从PDF第9页起（印刷第3页），勿混淆页码。
- 定位：§2、图1、式(2.1)起为几何推导；§3.1路径数据、§3.3执行步骤；§4.1前视距影响、§4.2曲率和前视点的非唯一性。
- 支持：后轴参考点、目标点→车体系→曲率→执行请求的完整步骤。原文轴定义与本报告x前/y左不同，本文重新表达曲率符号。
- 不支持：其算法是定位器或特定厘米级跟踪精度；报告没有本研究条件下统一独立真值的现代横评。

## S11：Regulated Pure Pursuit与Nav2实现

**Steve Macenski, Shrijit Singh, Francisco Martín, Jonatan Ginés. _Regulated Pure Pursuit for Robot Path Tracking_. 2023.** Autonomous Robots；[实际读取arXiv v1](https://arxiv.org/pdf/2305.20026v1)，[DOI：10.1007/s10514-023-10097-6](https://doi.org/10.1007/s10514-023-10097-6)。DOI另经[作者出版列表](https://fmrico.github.io/publications.html)核对，出版社全文页访问失败不影响v1全文已读。

- 访问：已读11页全文；§2纯追踪、§3曲率/障碍/碰撞约束、§4实现；§5.1仿真、§5.2–5.4实体Tiago；表1–3。表2五次狭窄走廊平均路径距离PP/APP/RPP=0.100/0.059/0.052 m，时间13.0/12.7/13.3 s。
- 条件：仿真用真值，实体用SLAM Toolbox地图和AMCL；没有明确独立动捕真值。原文“最大角速度”的rad/s²量纲有问题，本文不作为角速度参数采用。
- 关联代码：Nav2 commit `235fc5ce55bdf94d9be360fdbca39d89dc0e4f74`，2026-10-06。[固定控制器源码](https://github.com/ros-navigation/navigation2/blob/235fc5ce55bdf94d9be360fdbca39d89dc0e4f74/nav2_regulated_pure_pursuit_controller/src/regulated_pure_pursuit_controller.cpp)，读`computeVelocityCommands`、`calculateCurvature`、`applyConstraints`及README。此2026代码与2023论文不可当作同版复现。
- 支持：曲率及环境调速机制和限定实验；不支持0.052 m是完整真实绝对定位精度或任何场景安全保证。
- 2026关联源码还明确区分全局到达检查与车体系前视路径；其注释说明混用坐标会导致错误方向和误报到达。报告第12章使用这一源码事实，不推断未运行的故障率或2023论文结论。

## S12：原始Madgwick算法报告

**Sebastian O. H. Madgwick. _An efficient orientation filter for inertial and inertial/magnetic sensor arrays_. 2010-04-30.** [x-io原始报告](https://x-io.co.uk/downloads/madgwick_internal_report.pdf)

- 访问：已下载和阅读32页全文；不把2011后续会议论文或当前Fusion替代该版本。
- 定位：§3.1角积分、§3.2梯度下降、§3.3融合、§3.4磁失真、§3.5 gyro bias、§3.6增益；§4设备、光学参考、外参与同步；§5.2表1各欧拉分量静/动态RMS，图7–11增益/采样率等；附录A/B原C实现。
- 支持：IMU与MARG结构不同，人工旋转/Xsens MTx/光学参考条件下姿态评价。表1不能简化为所有分量均小于摘要的汇总阈值。
- 边界：报告摘要IMU运算量109，结论108，内部数值不一致；只能说该实现计算量低，不能未经说明套用统一操作数。没有车辆位置或控制实验。

## S13：x-io Fusion当前公开实现

**x-io Technologies，_Fusion_官方C库。** [固定版本](https://github.com/xioTechnologies/Fusion/tree/a8d7224f36a0ec82345ef49a3db50e65f8d3bab8)

- 版本：commit `a8d7224f36a0ec82345ef49a3db50e65f8d3bab8`，2026-09-18；实际读README及[源码FusionAhrs.c](https://github.com/xioTechnologies/Fusion/blob/a8d7224f36a0ec82345ef49a3db50e65f8d3bab8/Fusion/FusionAhrs.c)，未运行。
- 定位：README算法说明、启动/角速率恢复、加速度/磁拒绝及恢复、settings/内部状态；源码`FusionAhrsUpdate`、`FusionAhrsSetSettings`、`FusionAhrsGetInternalStates`、`FusionAhrsGetFlags`。
- 支持：明确是Madgwick博士论文第7章改进算法，区别2010原版；方向反馈、去重力输出与拒绝/恢复可核验。持续拒绝后的恢复阶段可强制使用观测，不代表物理干扰必定消失。
- 不支持：完整INS位置滤波、任意场景姿态保证、车辆实测漂移或控制效果。

## S14：robot_localization融合配置规则

**robot_localization维护团队，_Configuring robot_localization_官方配置文档。** [固定文档](https://github.com/cra-ros-pkg/robot_localization/blob/7dfb6aa97b2082185d2fac3420888ae8474bfc1a/doc/configuring_robot_localization.rst)

- 版本：rolling-devel commit `7dfb6aa97b2082185d2fac3420888ae8474bfc1a`，2026-07-16；读取该文档，没有审阅整个滤波实现或运行ROS。
- 定位：二维模式、轮程重复来源、零侧向速度伪观测、差分/相对输入、协方差说明。
- 支持：避免重复融合来自同一轮来源的姿态/位置/速度；二维模式是人为施加约束。
- 限制：文档把无侧向速度与无侧向加速度关联的句子不能沿用；转弯车体即使v_by=0，仍可有a_by=vω。本文NHC模型按物理方程推导，未把文档该表述作为依据。

## S15：光电与gyro迷宫工程

**Pololu，_Zumo 32U4 Arduino Library / MazeSolver_官方示例。** [固定代码目录](https://github.com/pololu/zumo-32u4-arduino-library/tree/f4dfe054e23176ba445748b4b91f463701e7eb76/examples/MazeSolver)

- 版本：库`2.0.1`（`library.properties`），commit `f4dfe054e23176ba445748b4b91f463701e7eb76`，2022-09-07；读取`MazeSolver.ino`、`GridMovement.h`、`TurnSensor.h`，未实车执行。
- 定位：`followSegment()`光电误差/路口；`turn()`gyro预转及捕线；`turnSensorSetup()`静态gyro与标定；迷宫学习/解路流程。
- 支持：正交迷宫中光电直接循线和gyro定角分工。B分支实际代码为3×45°即135°，注释125°不一致；分析采用实际条件。
- 不支持：完整全局导航EKF、任意未知曲线丢线接管或定量恢复成功率。原示例假定黑线白底、学习阶段无环。

## S16：QTR反射阵列读数

**Pololu，_QTRSensors Arduino library_与官方应用说明。** [官方说明](https://www.pololu.com/docs/0j19/all)，[固定4.0.0源码](https://github.com/pololu/qtr-sensors-arduino/blob/4a0d1c59842307164735f923f0eb99cbb2538428/QTRSensors.cpp)

- 版本：`library.properties`确认`4.0.0`；commit `4a0d1c59842307164735f923f0eb99cbb2538428`，2019-03-16；读README、源码及版本文件。旧官方应用说明针对3.1.0接口，概念可参考但API名称不混用。
- 定位：`calibrate()`、`readCalibrated()`、`readLinePrivate()`（固定代码第644–687行）。加权位置与失线返回上次方向边界可由源码核验。
- 支持：局部线偏差的形成和失线语义；内部阈值只属于该实现，不是光电通用判据。
- 不支持：单排读数能提供完整位置/yaw、边界返回值是真实线观测、具有独立实测定位精度。

## S17：航向保持与循线控制源码

**Pololu，_RotationResist_、_LineFollower_官方示例。** 同S15的2.0.1和固定commit。[RotationResist](https://github.com/pololu/zumo-32u4-arduino-library/blob/f4dfe054e23176ba445748b4b91f463701e7eb76/examples/RotationResist/RotationResist.ino)，[LineFollower](https://github.com/pololu/zumo-32u4-arduino-library/blob/f4dfe054e23176ba445748b4b91f463701e7eb76/examples/LineFollower/LineFollower.ino)

- 访问：完整读取两份源码及共享转角辅助文件；未烧录或运行车辆。
- 定位：`loop()`中RotationResist以累计gyro角与角速率形成P/D命令；LineFollower以`readLine`偏差和离散差分生成差动电机命令并限幅。
- 支持：公开可读控制闭环、命令映射及饱和的具体实现；代码注释限定电池、电机和赛道条件。
- 不支持：整数电机命令等于m/s、特定真实超调/稳定时间或任何机器人通用增益；示例没有报告独立真值和定量控制性能。

## S18：KITTI数据与评价规范

**KITTI团队（Andreas Geiger、Philip Lenz、Christoph Stiller、Raquel Urtasun等），_The KITTI Vision Benchmark Suite / Visual Odometry–SLAM Evaluation 2012_.** [官方评价页](https://www.cvlibs.net/datasets/kitti/eval_odometry.php)，[官方原始数据说明](https://www.cvlibs.net/datasets/kitti/raw_data.php)

- 访问：联网阅读官方网页，2026-10-07；本项是数据/评价规范，不冒称已经阅读所有关联KITTI论文或下载全数据。AI-IMU实际用的100 Hz原始IMU与odometry位姿的对应条件见S07。
- 定位：评价页“Visual Odometry / SLAM Evaluation 2012”正文的序列划分、指标及2013-10-03更新说明。
- 支持：00–10公开训练真值，11–21隐藏评测真值；所有起点的100–800 m子轨迹，平移百分比/旋转deg/m；小段GPS/OXTS真值误差促使评价改用长段。
- 不支持：榜单百分数可直接评价厘米级停车或跨传感器/数据划分的公平性。本文不引用动态榜单名次作为当前算法优劣结论。

## 索引核验与使用规则

1. 报告实验数字已逐项对照：S06表III、S08表V、S11表2、S07表1与训练协议、S04实际采集段、S12表1；S01回到正文静止日志和磁修正说明。没有根据图片自行估算厘米数。
2. 网络访问成功与否、版本及SHA应以本目录`source_manifest.json`为审计材料；网址可能更新，固定commit链接优先。全文文件是研究阅读缓存，不依赖它们才能离线阅读报告正文。
3. 所有论文数字都保留设备、路线、速度、初始化、真值和指标条件；不形成未经同条件实验的“精度排行榜”。源码审阅不称实验复现。
4. 直接证据不足的部分见`coverage.md`：未知路径通用光电失线恢复、外部观测恢复后控制无扰切换、全部异常的检测阈值，以及本文融合/控制建议的实车效果。
