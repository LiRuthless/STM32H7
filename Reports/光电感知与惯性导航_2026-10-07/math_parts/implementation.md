## 5 与公式一致的代码接口与运行方法

下载包的 `code/optical.py` 实现 `normalize`、`digital`、`centroid`、`difference_over_sum`、`PID` 和 `OpticalController`；`code/inertial.py` 实现 `IMUSample`、`Navigator`、四元数运算、可信静止零偏标定和可选初始倾角初始化。完整代码见报告附录，单独文件可直接导入。

```text
原始模拟数值 + 背景/目标标定 -> normalize() -> q_i
一位电平 + target_level -> digital() -> q_i
q_i -> centroid()/difference_over_sum() -> OpticalResult
时间戳 + 原始光电数组 -> OpticalController.step() -> 有效偏差与轮速命令
IMUSample(t_s, f_b, ω_b, count_left, count_right, stationary)
    -> Navigator.update() -> 纯IMU状态 + 独立平面odom状态
```

### 5.1 初始化与可调参数

光电示例标定 `background` 为五个 $100\ \mathrm{counts}$、`target` 为五个 $900\ \mathrm{counts}$、ADC 范围为 $[0,1023]\ \mathrm{counts}$；读数 $[140,180,260,740,820]\ \mathrm{counts}$ 对应第 2 节 $\boldsymbol q$ 算例。反向极性用 `background` 为 $900\ \mathrm{counts}$、`target` 为 $100\ \mathrm{counts}$ 即可。归一化需 $\lvert\ell_i-b_i\rvert\ge20\ \mathrm{counts}$，这只是示例对比门限，应按实测噪声设置。RC 接入时需改用计时原始单位、范围和对比门限，完成 GPIO 放电驱动及超时诊断；不能套用默认 ADC counts 参数。

光电低通 $\tau_q$ 默认 $0$（直通）。默认 PID $K_p=0.8$、$K_i=0.1\ \mathrm{s}^{-1}$、$K_d=0.02\ \mathrm{s}$、$\tau_d=0.02\ \mathrm{s}$、$u_{\max}=0.5$。参数 `v_base`、`k_u`、`v_max` 分别对应 $v_0=0.3\ \mathrm{m/s}$、$k_u=0.3\ \mathrm{m/s}$、$v_{\max}=0.6\ \mathrm{m/s}$；构造约束为

$$
k_u u_{\max}\le\min(v_0,v_{\max}-v_0),
$$

避免下游额外饱和。数字量可令 `mode='digital'` 并给定 `target_level`；示例默认低电平表示目标线。

质心 $S_{\min}=0.15$，差比和 $D_{\min}=0.05$；控制器当前 $\boldsymbol q$ 满足 $\sum_i q_i<0.15$ 或 $\max_i q_i<0.10$ 时判 `LOST`。所有 $q_i\ge0.8$ 或 $q_i\ge0.5$ 的响应形成两块以上不连通区域时判 `AMBIGUOUS`。后者只是明确的形状启发式，不能识别所有交叉口/宽线；若实际线会产生多峰，需按扫线数据改判据。丢线和异常时停止、`reset` PID 与滤波；报告讨论的有界搜索策略并未作为未知车辆的默认动作。

选择 `method='dos'` 时，左右分母过小而质心仍有效，可输出 `method='centroid_fallback'`、`reason='FALLBACK_CENTER_ONLY'` 或其他原始原因；方法切换复位 PID。示例为了展示接口共用一组增益，实车应根据两法局部斜率设置专属参数。低通前检查当前帧丢线/歧义，避免历史滤波值掩盖故障。

IMU 默认初态 $\boldsymbol q=[1,0,0,0]$，假定起始水平且航向角 $\psi=0$、初速零。非水平起始先用可信静止样本 `initialize_tilt`；该函数不提供真北。$k_a=1\ \mathrm{s}^{-1}$ 是重力反馈示例增益，幅值与重力相差不超过 $15\%$、方向创新不超过 $\frac{\pi}{4}$ 才采用反馈。二者均不能保证没有动态加速度。`gyro_weight` 为 $0.7$，$c_L=c_R=0.001\ \mathrm{m/count}$、$B=0.20\ \mathrm{m}$，默认角速度差门限 $2\ \mathrm{rad/s}$。差异过大时使用 gyro 转角并标记 `angular_disagreement`，轮程仍可能有平移滑移误差。

采样间隔可接受 $\Delta t\in[0.001,0.05]\ \mathrm{s}$；$10\ \mathrm{ms}$ 是名义值，不能将所有间隔硬写为 $0.01\ \mathrm{s}$。IMU 比力范数上限 $16g$、gyro 范数上限 $35\ \mathrm{rad/s}$、每轮绝对线速度上限 $5\ \mathrm{m/s}$ 为示例异常门限，实际应按型号量程/车辆极限设置。范围内读数仍可能错误，故硬件层还需饱和/通信/新鲜度标志。

### 5.2 记录 输出与恢复语义

`demo.py` 生成 101 帧人工数据，以同一时间戳驱动两个模块，输出 `output/samples.csv`、`metadata.json`、`frames.jsonl` 和 `summary.json`。CSV 保留原始 ADC、SI IMU、累计计数、`stationary`、时间间隔及偏差、控制、四元数、加速度、速度、位置与 `odom`；数组列用 JSON 编码。元数据记录坐标、单位、参数和人工数据身份。示例接收时间等于采集时间，仅表示零传输延迟假设；实际驱动应记录真正接收时刻与可用温度。此同步离线写文件方式用于复现，实时控制必须把日志写盘移到后台队列。

第一帧惯导仅建立时间/计数基线，不积分。非单调时间拒绝且不倒退基线；正向大缺口、量程/输入故障和计数跳变拒绝并重新建立可用基线，以免下一帧把未知间隔补积分。若时间戳为 `NaN`/`Infinity`，故障时刻未知，下一有限有效帧只重建基线。丢帧之后以 `incomplete_dead_reckoning` 标记轨迹不完整，正常下一帧不意味着丢失位移被恢复。编码器缺失时不伪造轮程，恢复首帧重建轮计数基线。`valid` 表示更新通过程序检查；`quality` 描述条件或降级，二者都不是统计置信度。

纯 IMU 与平面 `odom` 的航向角 $\psi$ 分别输出。JSON 用有限数值和状态字段，拒绝把 `NaN`/`Infinity` 发送为有效测量；无效时间可用 `null`。标定工具只在明确 `stationary` 并经幅值/离散度检查的样本上求 gyro 均值。运行中可信静止标记须同时通过 IMU 与可用轮计数一致性检查，才缓慢更新 gyro 零偏/清零速度；错误静止标记返回 `STATIONARY_REJECTED`，不抹掉运动。

### 5.3 运行与算例验证

在 `code` 目录运行：

```powershell
python -m unittest -v test_algorithms.py
python demo.py
```

无需第三方包。若系统 python 不在 PATH，使用本机 Python 3.10 及以上解释器绝对路径。代码库没有硬件读写副作用；输出目录可删除后重跑。

本次在 Python 3.12.14 环境实际运行 **30 项单元验证，全部通过**，并成功导出 101 帧 CSV/JSONL 示例。验证日志包含命令、解释器版本与逐项结果。核心数值如下：

| 验证对象 | 预期值 | 实际核验 |
|---|---|---|
| 光电 $\boldsymbol q=[0.05,0.10,0.20,0.80,0.90]$ 质心 | $e_y=0.011707317073\ \mathrm{m}$，$e=0.585365853659$ | 通过，双精度误差内一致 |
| 同一 $\boldsymbol q$ 的差比和 | $e=0.837837837838$，$D=0.925$ | 通过 |
| PID 第 3.3 节算例 | $u=0.306966666667$；限幅 $0.25$ 时不增长积分 | 通过 |
| 水平静止 $\boldsymbol f=(0,0,g)$ | $\boldsymbol a\approx\boldsymbol 0$、$\boldsymbol v\approx\boldsymbol 0$、$\boldsymbol p\approx\boldsymbol 0$，四元数单位范数 | 通过 100 个更新步骤 |
| 已知姿态、$a_x=1\ \mathrm{m/s^2}$、$k_a=0$ | 首步 $v_x=0.01\ \mathrm{m/s}$，$p_x=0.00005\ \mathrm{m}$；$1\ \mathrm{s}$ 后 $1\ \mathrm{m/s}$、$0.5\ \mathrm{m}$ | 通过 |
| 左/右轮计数 $8/12$、$\omega_z=1.8\ \mathrm{rad/s}$ | $\psi=0.0186\ \mathrm{rad}$、$v=1\ \mathrm{m/s}$、$x=0.009999567553\ \mathrm{m}$、$y=0.000092998659\ \mathrm{m}$ | 通过 |
| 中心仅命中、全无信号、多块/全强 | 显式质心回退、`LOST` 停止、`AMBIGUOUS` 停止 | 通过 |
| `NaN`、超量程、错误长度、非法 $\Delta t$ | 拒绝输入/更新，有限 JSON，按规则恢复 | 通过 |
| 时间缺口、非单调时间、编码器跳变/缺失 | 不跨故障补积分，基线恢复，标记不完整 | 通过 |
| 不可信 `stationary`、倾角初始化与反馈、静态零偏 | 不误清运动速度，反馈符号正确，条件标定 | 通过 |

独立复核发现的无效时间戳恢复问题已修正：`t_s` 为非有限值时，不再跨未知区间积分；下一有限有效帧只建立基线，返回 `REBASELINED_AFTER_INVALID_TIME`，并持续标记 `incomplete_dead_reckoning`。该行为有专门回归测试。实际运动速度、硬件最坏执行时间、控制稳定裕度和轨迹精度不属于本次离线验证结论。

### 5.4 流程图

![光电与惯导统一数据流程](流程图.svg)

### 5.5 思维导图

![算法原理与工程实现思维导图](思维导图.svg)



## 6 实际硬件接入与验证边界

光电端需补充 ADC 分辨率和参考电压、GPIO 有效电平、采样顺序、发射灯控制、安装高度、阵列间距、黑白标定与饱和诊断。IMU 端需补充量程与比例因子、轴映射、数据就绪/FIFO 时间戳、温漂标定、总线超时和传感器饱和标志；读取寄存器成功并不意味着数值物理有效。编码器端需补充计数溢出差分、正反方向、每轮每转计数、轮径、轮距、减速比和重置检测。执行器端需补充 PWM 或舵机标定、速度反馈、死区、限流与停止策略。

先架空或低速核验“左偏→正偏差→左转”和水平静止时的关系

$$
f_z\approx+g\quad\Longrightarrow\quad\boldsymbol a_n\approx\boldsymbol 0.
$$

然后用已知距离直行、左右等角转弯、不同光照扫线和丢线实验检查误差。用地面标记、摄像或外部定位建立轨迹真值，报告横向误差、位置误差、掉帧率、恢复时间、饱和比例以及速度分布。只有这些实测能支持具体车辆的运动速度和精度结论。

