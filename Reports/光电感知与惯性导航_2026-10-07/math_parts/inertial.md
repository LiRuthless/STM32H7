## 4 惯性导航的实现 记录 输出与修正

### 4.1 输出的区别与方案边界

姿态是车体相对导航系的三维旋转，可用四元数 $\mathbf q_{nb}$ 或 roll/pitch/yaw 表示；航向通常指 yaw，需要说明参照方向。本例航向相对启动方向，不是真北或磁北。速度是位置对时间的变化，位置是相对启动点的坐标，轨迹是带时间的连续位置序列，不能把一张 yaw 曲线称为位置轨迹。

| 信息 | 纯 6 轴 IMU | IMU＋左右轮编码器 |
|---|---|---|
| roll/pitch | 静止或线加速度较小时可用重力方向修正陀螺积分 | 同左；编码器本身不测倾斜 |
| yaw | 已知初始 yaw 后积分；重力不提供 yaw 观测，长期漂移 | 陀螺与轮差提供相对转角；仍无绝对航向 |
| 速度 | 去重力后的比力积分，需初速，误差增长 | 由轮程/时间直接估计前向速度，需轮径、传动比、计数分辨率与无滑动假设 |
| 位置、轨迹 | 再积分，需初始位置；低成本 IMU 通常不宜独立长期定位 | 轮程与融合航向递推局部平面轨迹；打滑与航向误差仍累积 |
| 主要前提 | 标定、初始姿态/速度、准确时间与安装变换 | 另需差速运动学、有效轮距、左右轮径与同步计数 |

上述姿态观测边界由 [VectorNav AHRS 官方说明](https://www.vectornav.com/resources/inertial-navigation-primer/theory-of-operation/theory-ahrs)支持：加速度计可提供重力倾角，但动态加速度破坏其假设；yaw 通常需磁或其他航向参考。本文不把厂商带磁力计的 AHRS 表现直接套用到 6 轴 IMU。

### 4.2 纯 IMU 如何实现

#### 4.2.1 原始测量、标定与初始化

加速度计测的是比力 $\mathbf f_b$，不是已经去掉重力的线加速度。静止水平时，$\mathbf f_b\approx(0,0,+g)^{\mathsf T}$。先把寄存器数值按量程换成 $\mathrm{m\,s^{-2}}$、$\mathrm{rad\,s^{-1}}$，再做轴映射、偏置、比例因子和非正交修正：

$$
\begin{aligned}
\mathbf f_b &= \mathbf C_a(\mathbf f_m-\mathbf b_a),\\
\boldsymbol\omega_b &= \mathbf C_g(\boldsymbol\omega_m-\mathbf b_g).
\end{aligned}
$$

$\mathbf C_a$ 与 $\mathbf C_g$ 合并比例与轴间误差；$\mathbf b_a$ 单位 $\mathrm{m\,s^{-2}}$，$\mathbf b_g$ 单位 $\mathrm{rad\,s^{-1}}$。静止均值可估计陀螺零偏；加速度计在一个静止姿态只观察到重力与偏差之和，不能一般地一次确定三轴零偏、比例和安装误差。六面静态标定可利用每轴 $\pm g$；陀螺比例因子需要已知转速或可靠角度基准。温度变化时可建立 $\mathbf b(T)$ 的表或拟合。此标定模型、六面重力测试、转台与温度标定依据见 [VectorNav IMU Calibration & Characterization](https://www.vectornav.com/resources/inertial-navigation-primer/specifications--and--error-budgets/specs-imucal)。校准减少可重复结构误差，不能消除随机噪声。

对于已校正、已确认静止的初始比力，采用欧拉角旋转次序，roll、pitch、yaw 分别记为 $\phi$、$\theta$、$\psi$：

$$
\begin{aligned}
\mathbf R_{nb} &= \mathbf R_z(\psi)\,\mathbf R_y(\theta)\,\mathbf R_x(\phi),\\
\phi &= \operatorname{atan2}(f_y,f_z),\\
\theta &= \operatorname{atan2}\!\left(-f_x,\sqrt{f_y^2+f_z^2}\right),\\
\psi &= 0.
\end{aligned}
$$

$\psi=0$ 表示人为设定初始前向。加速度计不能验证这个 yaw 是否等于真北。初始位置默认 $\mathbf 0$，初速默认 $\mathbf 0$，只有确实静止时该初速成立。

#### 4.2.2 姿态更新与重力补偿

通用姿态积分用四元数而非逐轴直接积分欧拉角。$\mathbf q_{nb}$ 采用标量在前 $[w,x,y,z]^{\mathsf T}$，Hamilton 乘法：

$$
\begin{aligned}
\dot{\mathbf q}_{nb} &= \frac{1}{2}\mathbf q_{nb}\otimes
\begin{bmatrix}0\\\boldsymbol\omega_c\end{bmatrix},\\
\mathbf q_{nb} &\leftarrow \frac{\mathbf q_{nb}}{\lVert\mathbf q_{nb}\rVert}.
\end{aligned}
$$

每步积分后进行上述归一化。

可加入一个易解释的重力方向比例反馈（工程示例，属于互补滤波思想，并非宣称完整复现某论文）：

$$
\begin{aligned}
\mathbf u_b &= \mathbf R_{nb}^{\mathsf T}\begin{bmatrix}0\\0\\1\end{bmatrix},\\
\widehat{\mathbf f}_b &= \frac{\mathbf f_b}{\lVert\mathbf f_b\rVert},\\
\mathbf e_g &= \widehat{\mathbf f}_b\times\mathbf u_b,\\
\boldsymbol\omega_c &= \boldsymbol\omega_b+k_a\mathbf e_g.
\end{aligned}
$$

$k_a$ 单位 $\mathrm{s^{-1}}$。在方向可靠时，反馈拉回 roll/pitch；不观测 yaw。$k_a$ 越大，倾角校正更快，也越容易把运动加速度当重力。比力幅值偏离 $g$、饱和、非有限数或姿态创新过大时暂停反馈；仅检查 $\lVert\mathbf f_b\rVert\approx g$ 并不能证明没有动态加速度，持续转弯仍可能误导倾角。因此纯 IMU 双积分演示可设置 $k_a=0$ 及已知初态，用于验证代数；实车选择 $k_a$ 应基于振动与加减速记录。[x-io Fusion 官方工程](https://github.com/xioTechnologies/Fusion)公开了陀螺积分与反馈、增益折衷、加速度拒绝与四元数/去重力加速度输出；此引用支持设计思想与可核验实现，本文示例不是其二进制或算法性能复现。

导航系线加速度与积分如下；$\Delta t$ 表示实际采样时间差：

$$
\begin{aligned}
\mathbf a_n &= \mathbf R_{nb}\mathbf f_b+\mathbf g_n,\\
\mathbf v_k &= \mathbf v_{k-1}+\mathbf a_{n,k}\Delta t,\\
\mathbf p_k &= \mathbf p_{k-1}+\mathbf v_{k-1}\Delta t
+\frac{1}{2}\mathbf a_{n,k}(\Delta t)^2.
\end{aligned}
$$

后一式是采样区间内加速度恒定近似；快速变化时使用更高采样率或梯形/中点积分。本文短程局部例不建模地球自转、科里奥利与曲率，不能作为长距离精密 INS。IMU 不在车体参考点时还需处理角加速度/向心加速度的杆臂项；示例假定安装在近似参考点。

**数值算例（报告推导，不是实测）**：姿态单位四元数、$\mathbf f_b=(1,0,9.80665)^{\mathsf T}\,\mathrm{m\,s^{-2}}$，$\mathbf a_n=(1,0,0)^{\mathsf T}\,\mathrm{m\,s^{-2}}$，初速初位为零、$\Delta t=0.01\,\mathrm s$，则第一步：

$$
\begin{aligned}
\mathbf v_1 &= (0.01,0,0)^{\mathsf T}\,\mathrm{m\,s^{-1}},\\
\mathbf p_1 &= (0.00005,0,0)^{\mathsf T}\,\mathrm m.
\end{aligned}
$$

相同恒定加速度运行 100 步、保持已知姿态而不启用重力反馈，应得 $v_x=1\,\mathrm{m\,s^{-1}}$、$p_x=0.5\,\mathrm m$。启用基于加速度的倾角反馈会违反本算例恒定前向加速度下的重力假设，不能期待相同结果。

### 4.3 IMU＋编码器如何实现

左右轮单个计数对应距离为：

$$
\begin{aligned}
c_L &= \frac{2\pi r_L}{N_{\mathrm{rev},L}},\\
c_R &= \frac{2\pi r_R}{N_{\mathrm{rev},R}}.
\end{aligned}
$$

$N_{\mathrm{rev}}$ 是每轮转一圈最终解码计数，需包含减速比和四倍频，不能把编码器铭牌 PPR 直接当 $N_{\mathrm{rev}}$。

$$
\begin{aligned}
\Delta s_L &= c_L\Delta n_L, & \Delta s_R &= c_R\Delta n_R,\\
\Delta s &= \frac{\Delta s_L+\Delta s_R}{2},\\
\Delta\psi_{\mathrm{enc}} &= \frac{\Delta s_R-\Delta s_L}{B},\\
v_{\mathrm{enc}} &= \frac{\Delta s}{\Delta t}.
\end{aligned}
$$

$B$ 为有效轮距。右轮走得更多则车向左转，符合 yaw 正号。无侧滑时 $\mathbf v_b=(v_{\mathrm{enc}},0,0)^{\mathsf T}$。这是差速车运动学，阿克曼转向车需用转角、轴距与速度替换 yaw 模型，不能照搬轮差公式。运动学与无滑动假设由 [Dellaert、Hutchinson 开放教材 5.2](https://www.roboticsbook.org/S52_diffdrive_actions.html)支持。

平面示例可用角增量加权融合：

$$
\begin{aligned}
\Delta\psi_g &= \omega_{b,z}\Delta t,\\
\Delta\psi &= \lambda\Delta\psi_g+(1-\lambda)\Delta\psi_{\mathrm{enc}},
\qquad 0\le\lambda\le 1.
\end{aligned}
$$

这里 $\omega_{b,z}$ 已完成校正，不能再次扣零偏。示例 $\mathbf C_g=\mathbf I$、只做偏置修正，因此 $\Delta\psi_g=(\omega_{m,z}-b_{g,z})\Delta t$；代码输入 gyro_rad_s 是 SI 单位的测量值 $\boldsymbol\omega_m$，Navigator 内部减去所配置零偏一次。

$$
\begin{aligned}
\psi_k &= \operatorname{wrap}(\psi_{k-1}+\Delta\psi),\\
x_k &= x_{k-1}+\Delta s\cos\!\left(\psi_{k-1}+\frac{\Delta\psi}{2}\right),\\
y_k &= y_{k-1}+\Delta s\sin\!\left(\psi_{k-1}+\frac{\Delta\psi}{2}\right).
\end{aligned}
$$

gyro 的 $z$ 分量等于 yaw 角速度仅在水平平面假设下成立；非平面情形应使用姿态运动学。中点位置递推近似小角圆弧；大角区间可用精确圆弧：把上述 $\Delta s$ 乘 $\dfrac{\sin(\Delta\psi/2)}{\Delta\psi/2}$。

在两种角增量误差独立、零均值、方差已知时，最小方差线性权重为（数学推导）：

$$
\lambda=\frac{\sigma_{\mathrm{enc}}^2}{\sigma_g^2+\sigma_{\mathrm{enc}}^2}.
$$

常数 $\lambda$ 是教学简化，系统性轮径误差、打滑与温漂不满足这些统计条件。编码器可信时增加其权重；出现滑移疑似、角速度创新 $\left\lvert\dfrac{\Delta\psi_g}{\Delta t}-\dfrac{\Delta\psi_{\mathrm{enc}}}{\Delta t}\right\rvert$ 过大时可临时令 $\lambda=1$ 并标记降级，但 gyro 只能改善转向，无法恢复未知打滑造成的轮程误差。两个来源不一致也可能是时间错位、轴映射或标定错误，不能直接判定某个传感器坏了。

**数值算例（报告假设）**：$c_L=c_R=0.001\,\mathrm{m\,count^{-1}}$，$B=0.20\,\mathrm m$，$\Delta t=0.01\,\mathrm s$，$\Delta n_L=8$、$\Delta n_R=12$，则：

$$
\begin{aligned}
\Delta s &= 0.010\,\mathrm m,\\
v_{\mathrm{enc}} &= 1\,\mathrm{m\,s^{-1}},\\
\Delta\psi_{\mathrm{enc}} &= 0.020\,\mathrm{rad}.
\end{aligned}
$$

扣零偏后的 $\omega_{b,z}=1.8\,\mathrm{rad\,s^{-1}}$ 得 $\Delta\psi_g=0.018\,\mathrm{rad}$。取 $\lambda=0.7$ 得 $\Delta\psi=0.0186\,\mathrm{rad}$。从 $(0,0,0)$ 起步，中点公式给出：

$$
\begin{aligned}
x &\approx 0.0099995676\,\mathrm m,\\
y &\approx 0.00009299866\,\mathrm m,\\
\psi &\approx 1.0657353^{\circ}.
\end{aligned}
$$

$\lambda=0.7$ 仅为检验接口的示例，不能作为未测量传感器的最优参数。

更完整 EKF 可采用状态 $[x,y,\psi,v,b_{g,z}]^{\mathsf T}$，以 gyro 和运动模型预测，以编码器速度、轮差角速度及可信静止约束更新；协方差应来自静态噪声和实际轮地试验。相同编码器已算出的 pose、速度与 yaw 不应当作相互独立的新信息重复融合，见 [robot_localization 官方传感器配置教程](https://raw.githubusercontent.com/cra-ros-pkg/robot_localization/rolling-devel/doc/configuring_robot_localization.rst)。该教程证明工程接口和相关性注意事项，不证明本文简化融合达到某精度。

### 4.4 数据如何记录与同步

记录必须保留能够重新运行算法的输入与配置，而不只保存最终轨迹。以下是一份建议字段表；单位和坐标系必须写入文件头/旁车元数据，不靠读者猜测。

| 字段 | 单位/坐标 | 用途 |
|---|---|---|
| seq, t_sample_ns, t_receive_ns | 整数；同一单调时钟时间域 | 查丢帧、时间间隔与通信延迟 |
| accel_m_x/y/z, gyro_m_x/y/z | $\mathrm{m\,s^{-2}}$、$\mathrm{rad\,s^{-1}}$；车体 $b$；尚未减本次偏置 | 离线重做标定；另可存原始寄存器整数与量程 |
| temperature | $^{\circ}\mathrm C$，若传感器支持 | 检验温漂 |
| count_L/R, t_encoder_ns | 累计解码 count、采样时刻 | 重建轮程，避免只保留微分后的量化速度 |
| dt_s, b_a_xyz, b_g_xyz | $\mathrm s$、$\mathrm{m\,s^{-2}}$、$\mathrm{rad\,s^{-1}}$；$b$ | 审计状态估计与在线偏置 |
| q_nb_w/x/y/z | 无量纲，标量在前、$b\to n$ | 姿态可复算 |
| a_n_xyz, v_n_xyz, p_n_xyz | $\mathrm{m\,s^{-2}}$、$\mathrm{m\,s^{-1}}$、$\mathrm m$；$n$ | 纯 IMU 结果，必须标注 drift_demo |
| yaw_rad, speed_m_s, x_m, y_m | $\mathrm{rad}$、$\mathrm{m\,s^{-1}}$、$\mathrm m$；planar 局部 $n$ | IMU＋编码器估计 |
| stationary, valid, quality, fault_flags | 布尔/枚举/位图 | 区分异常、拒绝观测与降级 |

元数据另存传感器型号/量程/ODR、安装旋转、轮径轮距、$N_{\mathrm{rev}}$、标定 ID、算法版本与参数。若记录解算 covariance，必须写清顺序、坐标与单位平方，不能用未经估计的数值冒充置信度。

同步原则是对齐采集时刻而非串口到达时刻：MCU 用同一硬件计时器在 IMU DRDY 时记录时间、快照编码器；编码器异步时，对同时间区间内累计计数差计算轮程。不同设备时钟需估计偏移与漂移；记录接收时间只能帮助诊断，不可替代采样时间。示例接口假定 IMU 与编码器已对齐至同一时间戳。延迟 $\delta t$ 引入约 $\omega\delta t$ 的角度错位及 $v\delta t$ 的位置关联偏差，是量纲明确的一阶推导。

采样频率依所需车体动态带宽、传感器低通、振动混叠、编码器最小计数与控制延迟选择。理论上采样率应高于有效最高频率的两倍，并留工程余量；先在采样前/传感器端抗混叠，不能期待采样后滤波恢复混叠信号。$100\,\mathrm{Hz}$、$10\,\mathrm{ms}$ 是教学值。姿态/控制输出可同步 $100\,\mathrm{Hz}$，上位机绘图可降至 $10\text{–}20\,\mathrm{Hz}$；原始数据宜保持估计所用频率。CSV 可人工检查，JSONL 可带状态字段；二进制适合高率存储，但需版本、字节序、长度、CRC 和格式说明。日志应通过缓冲/后台写出，不能把文件 I/O 延迟反馈进采样周期。

示例输入记录（$\mathbf f_m$ 为已换 SI 单位但未减本次零偏的原始测量；时间从启动计）：

```csv
seq,t_sample_ns,accel_m_x,accel_m_y,accel_m_z,gyro_m_x,gyro_m_y,gyro_m_z,count_L,count_R,stationary
0,0,0,0,9.80665,0,0,0,0,0,1
1,10000000,0,0,9.80665,0,0,1.8,8,12,0
```

这两行用于运动学/接口算例；不是声称完整重现转弯中实际加速度（真实转弯包含向心加速度）。

### 4.5 结果如何输出

控制程序应读带时间戳、有效性、更新周期和单位的结构，不读面向显示的字符串。角速度闭环用 gyro $\mathrm{rad\,s^{-1}}$，速度闭环用 speed $\mathrm{m\,s^{-1}}$，航向控制用相对 yaw $\mathrm{rad}$；横向路径控制还需 $x/y$ 与已知路径。上位机可显示姿态度数、yaw 时间曲线、速度时间曲线和 $x/y$ 轨迹，并标注参照起点；绘图取样不能改变估计 $\Delta t$。

```json
{"t_s":0.01,"mode":"imu_encoder_planar","frame":"startup_local_x_forward_y_left_z_up","yaw_rad":0.0186,"speed_m_s":1.0,"x_m":0.0099995676,"y_m":0.00009299866,"valid":true,"quality":"relative_dead_reckoning"}
```

valid=true 仅代表本次输入与更新通过程序检查，不代表位置等于真值。纯 IMU 输出需同时带 $\mathbf q_{nb}$、$\mathbf a_n$、$\mathbf v_n$、$\mathbf p_n$，并明确纯惯性位置为短程演示；不可用平面融合的 yaw 默默覆盖独立 AHRS 四元数中的 yaw 而不说明。

### 4.6 如何修正以及修正的边界

| 误差/修正 | 修正什么 | 必需条件 | 不能解决什么 |
|---|---|---|---|
| 静态 gyro 均值、温度表 | 启动零偏及可重复温漂 | 真实静止、热状态明确；持续若干样本检验离散度 | 运动中不可见漂移；比例误差 |
| 六面加速度标定、安装标定 | 零偏、比例、坐标失准 | 已知重力和多方向数据/机械基准 | 随机噪声、重力方向与动态加速度混淆 |
| 低通、互补滤波 | 高频噪声、短期倾角漂移 | 带宽和延迟预算；倾角观测有效 | 恒定偏置和长期位置漂移；滤波增加延迟 |
| ZUPT（零速更新） | 确认静止时的速度漂移；滤波器中部分偏置 | 可信静止信息，不仅是低 gyro | 已累计位置误差、绝对 yaw；误判会抹掉匀速运动 |
| 编码器辅助 | 用轮程代替加速度双积分；改善部分转角估计 | 无/可识别滑动、有效轮参数与同步 | 相同方向双轮打滑、侧滑、绝对位置航向 |
| 轮参数标定/双向方形路程 | 轮径比例、左右不等、有效轮距 | 有真值或可测终点，平稳地面，多次往返 | 随机地面扰动与每次打滑 |
| 非完整约束 $v_{b,y}=0$ | 限制侧向速度误差 | 平面、无侧滑、参考点正确 | 漂移中的绝对位置；滑移时会引入错误 |
| GNSS、视觉、测距/已知标记 | 提供独立位置或航向参考、限制相应漂移 | 外部观测可用、外参时间标定及噪声模型 | 遮挡/磁扰/错误匹配等观测故障 |

低角速度、$\lVert\mathbf f_b\rVert\approx g$ 也可能是匀速行驶，不能自动当成静止。因此示例只允许经外部可信 stationary 标记并通过 IMU 条件检查后更新零偏/清零速度。电机不转、编码器无变化也不是所有情况下的车体静止证明（例如被搬动）。ZUPT 不回滚过去轨迹。

**误差增长算例（数学推导）**：恒定未补偿加速度偏置 $\delta a=0.02\,\mathrm{m\,s^{-2}}$，忽略其他误差，$t=10\,\mathrm s$ 后：

$$
\begin{aligned}
\delta v &= \delta a\,t=0.2\,\mathrm{m\,s^{-1}},\\
\delta p &= \frac{\delta a\,t^2}{2}=1\,\mathrm m.
\end{aligned}
$$

初始水平倾角误差 $1^{\circ}$ 会把重力投影为约 $g\sin 1^{\circ}=0.17115\,\mathrm{m\,s^{-2}}$，若未校正，$10\,\mathrm s$ 位置误差约 $8.56\,\mathrm m$；若 yaw 初始误差 $1^{\circ}$，直行 $10\,\mathrm m$ 的侧向位置差约 $10\sin 1^{\circ}\,\mathrm m=0.1745\,\mathrm m$。gyro 恒定 yaw 偏置导致 yaw 随 $t$ 增长；倾角 gyro 偏置还会使错误重力补偿积累，短时小角近似为：

$$
\delta p\approx\frac{g\,b_g\,t^3}{6}.
$$

误差积分机制与姿态对速度位置的影响见 [VectorNav INS 官方原理](https://www.vectornav.com/resources/detail/what-is-an-ins)与 [GNSS/INS 原理](https://www.vectornav.com/resources/inertial-navigation-primer/theory-of-operation/theory-gpsins)。本文数值由给定假设计算，没有移用厂商典型器件表作实测结论。

轮式里程计系统性误差和非系统性打滑的区分可核验于 [Borenstein、Feng，Gyrodometry: A New Method for Combining Data from Gyros and Odometry in Mobile Robots（ICRA 1996）](https://www.cs.cmu.edu/~motionplanning/papers/sbp_papers/integrated2/borenstien_gyro.pdf)。该文支持轮误差分类、航向误差会扩散为位置误差、gyro/里程计互补的工程思路；其旧平台实验数值不是本文代码或现代 IMU 精度指标。

IMU＋编码器增加运动学、同步与轮参数标定，但避免了把加速度零偏双积分当成前向位置的主要来源，通常更适合地面车的短程局部运动估计。它仍属于航位推算，只有加入适当独立观测才能约束相应长期误差；滤波器复杂度本身不会凭空创造可观测信息。
