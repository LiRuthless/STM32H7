# 检索、阅读与数值验证记录

日期：2026-10-07。此文件记录本次实际工作范围，不是论文复现日志。

## 检索范围与方法

实际使用联网检索及网页/PDF全文读取，检索主题包括Woodman惯导误差、Mahony与Madgwick姿态算法、Wheel-INS/Wheel-GINS、AI-IMU、Solà误差状态、Tedaldi标定、Coulter纯追踪、Stanley车辆控制、RPP速度调节以及Pololu光电/gyro迷宫工程。二手索引/博客仅用于找到原文，报告关键技术和数字采用论文、作者代码与官方资料。

原始论文随后以Python urllib从公开链接下载到临时阅读缓存，以pypdf提取全文。下载文件SHA-256、页数及公开GitHub仓库commit由`source_manifest.json`保存。该清单的`files_readable`表示获取成功；实际用于报告的阅读范围见文献索引及本文件。没有把全文提取件或他人论文整篇复制进交付包。

Mahony作者稿本地下载返回403，但网页全文工具读到了16页作者稿；Coulter官方PDF本地证书校验失败，网页全文工具读到了15页。两者没有根据摘要补造内容。Wheel-INS早期v2（7页、WM-INS标题）仅用于核对版本；报告最终引用v4（12页）及2021期刊书目。Wheel-GINS大PDF在网页工具超过大小限制，改由公开PDF下载和本地提取读取，共13页。

## 核对过的内容

- Woodman：测量误差、Allan分析、捷联解算和§6–§8误差/静止漂移实验。
- Mahony：显式观测器公式(32)、方向和激励条件、实验结构；未把图线读成不存在的统一数值指标。
- Madgwick：§3原始IMU/MARG结构与附录版本、§4设备/对齐/采集、§5表1与评价条件。
- Solà：四元数约定、旋转积分、局部旋转扰动、ESKF预测/观测/注入/重置。
- Tedaldi：参数模型、重力模长与相邻姿态损失、静止检测/积分、实验协议和工厂参数对照。
- Gyrodometry：碰障机理、分歧门限选择、图2–4、超声参考及仅直行试验边界。
- Wheel-INS：车轮安装/旋转调制、轮速和约束、初始化、§IV表I–III及Test 2反例；库README、robot.yaml、dataset/ReadMe.pdf。
- AI-IMU：运动约束与协方差网络、训练/评测流程、§V表1及异常序列；当前README预训练数据划分。
- Wheel-GINS：GNSS/杆臂/参数观测及§IV实验、表II–V、人工失锁和高质量PPK输入。
- Coulter：几何推导及前视点实施/参数效应；没有把示意图视作现代benchmark。
- Stanley：估计/规划/控制分工、时戳、速度PI及转向公式(7)和动态补偿。
- RPP：算法调速度、§5仿真和Tiago条件、表1–3；识别原文最大角速度单位不一致。
- Pololu：固定commit的MazeSolver、GridMovement、TurnSensor、RotationResist、LineFollower，以及QTR官方说明；保留源代码注释与条件不一致及超时缺失等局限。
- Fusion：固定commit README与FusionAhrs.c；区别改进AHRS与Madgwick2010，拒绝与恢复不能自动判定持续观测正确。
- robot_localization：固定commit的配置教程，重复来源和二维约束；不采用其可能误导的侧向加速度推论。
- KITTI：官方子轨迹长度与真值/评估条件，只用指标定义，不用当前排行榜冒充论文结果。

## 已运行的本次数值计算

程序：`tools/numerical_examples.py`；Python 3.12.14，Windows 11。它实际计算了加速度偏置、初始倾角、倾角gyro残差、轮程yaw残差、角跨界、差速/阿克曼映射、Pure Pursuit、停车距离、错误到达、标量卡尔曼更新以及两种延迟的一阶角速度模型转弯仿真。输出在`data/results.json`与两份CSV，生成两幅SVG曲线。

这些是确定性自设模型，无随机训练、无实车输入。没有复跑Wheel-INS、AI-IMU、Wheel-GINS、Stanley、RPP或Pololu实机程序；没有声称8维教学滤波器已实测。控制回放不能替代改变命令后的闭环实车实验。

## 基线与仓库边界

已有HTML基线SHA-256：`47FCBD2D28393615A3BF9B129E46ACCF6A002A02EF76DDA7CA49B8B07DC9FBFE`。保留原报告。开始状态已有2993个删除项，另有Material/和Reports/未跟踪；本任务只新增`Reports/惯导系统研究_2026-10-07/`。最终暂存、检查与提交记录另见`review.md`。
