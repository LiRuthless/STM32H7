"""整合已核验研究，统一章节与静态公式；代码附录由 build_report.mjs 生成。"""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parent

def subheads(text,prefix):
    text=re.sub(r'^## (\d+)\. (.*)$',lambda m:f'### {prefix}.{m[1]} {m[2]}',text,flags=re.M)
    text=re.sub(r'^### (\d+)\.(\d+) (.*)$',lambda m:f'#### {prefix}.{m[1]}.{m[2]} {m[3]}',text,flags=re.M)
    return text

def formula_blocks(text,pattern,replacements):
    found=re.findall(pattern,text,re.S)
    if len(found)!=len(replacements):raise ValueError((len(found),len(replacements)))
    it=iter(replacements)
    return re.sub(pattern,lambda m:'\n```text\n'+next(it)+'\n```\n',text,flags=re.S)

a=(ROOT/'research/a.md').read_text(encoding='utf8').split('## 7.')[0]
a=a[a.index('## 1.'):]
a=formula_blocks(a,r'\$\$(.*?)\$\$',[
    'L = mean(q_i | y_i>0) ; R = mean(q_i | y_i<0)\ne_dos = (L−R)/(L+R)',
    'S = Σq_i ; e_y = Σ(y_i·q_i)/S [m]\ne_centroid = e_y/y_max ; y_max=0.02 m',
    '−0.02×0.05 −0.01×0.10 +0×0.20 +0.01×0.80 +0.02×0.90 = 0.024 m',
    'q(y)=A·h(y−y_0)\n∫yq(y)dy / ∫q(y)dy = y_0 + ∫u·h(u)du / ∫h(u)du = y_0',
    'e_dos = (0.85−0.075)/(0.85+0.075) = 0.775/0.925 = 0.837837838',
    'de_dos/dy_0 = 2(R·L′−L·R′)/(L+R)²',
    '∂e_y/∂q_i = (y_i−e_y)/S',
    'Var(e_y) ≈ Σ[(y_i−e_y)²·σ_i²]/S²',
    'D=L+R ; ∂e_dos/∂L = 2R/D² ; ∂e_dos/∂R = −2L/D²',
    'dY/dt=vψ ; dψ/dt=−K(Y+ℓψ)\n特征方程 λ²+Kℓλ+Kv=0',
])
a=a.replace('`\\dot ψ=K e_y`','`dψ/dt=K e_y`').replace('`\\dot ψ=k e`','`dψ/dt=k e`')
a='## 2 光电权重法与差比和法\n\n'+subheads(a,'2')
b=(ROOT/'research/b.md').read_text(encoding='utf8').split('## 6.')[0]
b=b[b.index('## 1.'):]
b=formula_blocks(b,r'\\\[(.*?)\\\]',[
    'q_i = clip((r_i−b_i)/(ℓ_i−b_i), 0, 1)',
    'q̄_i,k = q̄_i,k−1 + α_k(q_i,k−q̄_i,k−1)\nα_k = Δt_k/(τ_q+Δt_k)',
    'd_raw,k = (e_k−e_k−1)/Δt_k\nd_k = d_k−1 + Δt_k/(τ_d+Δt_k)·(d_raw,k−d_k−1)',
    'I_try = I_old + K_i·e_k·Δt_k\nu* = K_p·e_k + I_try + K_d·d_k\nu = clip(u*, −u_max, +u_max)',
])
b=b.replace('[180,260,420,740,820]','[140,180,260,740,820]').replace('[0.10,0.20,0.40,0.80,0.90]','[0.05,0.10,0.20,0.80,0.90]').replace('[820,740,580,260,180]','[860,820,740,260,180]')
b=re.sub(r'\*\*几何限制。\*\*.*?\n\n','阵列前置距离的作用见第 2.5 节的小角模型；单排阵列读数混合了横向和航向几何关系，不能独立重建完整车辆位姿。\n\n',b,flags=re.S)
b='## 3 数据类型与从感知到 PID 的闭环流程\n\n'+subheads(b,'3')
c=(ROOT/'research/c.md').read_text(encoding='utf8').split('## 7.')[0]
c=c[c.index('## 1.'):]
c=c.replace('c_L=2πr_L/N_L、c_R=2πr_R/N_R，N 是','c_L=2πr_L/N_rev,L、c_R=2πr_R/N_rev,R，N_rev 是')
c='## 4 惯性导航的实现 记录 输出与修正\n\n'+subheads(c,'4')
base=(ROOT/'report.md').read_text(encoding='utf8')
base=base.replace('<!-- OPTICAL -->',a).replace('<!-- PIPELINE -->',b).replace('<!-- INERTIAL -->',c)
implementation='''## 5 与公式一致的代码接口与运行方法

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

光电示例标定 background=[100]×5、target=[900]×5、ADC 范围 [0,1023]；读数 [140,180,260,740,820] 对应第 2 节 q 算例。反向极性用 background=900、target=100 即可。归一化需 |target−background|≥20 counts，这只是示例对比门限，应按实测噪声设置。RC 接入时需改用计时原始单位、范围和对比门限，完成 GPIO 放电驱动及超时诊断；不能套用默认 ADC counts 参数。

光电低通 τ_q 默认 0（直通）。默认 PID K_p=0.8、K_i=0.1 s⁻¹、K_d=0.02 s、τ_d=0.02 s、u_max=0.5。v_base=0.3 m/s、k_u=0.3 m/s、v_max=0.6 m/s；构造约束 k_u·u_max≤min(v_base,v_max−v_base)，避免下游额外饱和。数字量可令 mode='digital' 并给定 target_level；示例默认低电平表示目标线。

质心 S_min=0.15，差比和 D_min=0.05；控制器当前 q 的 sum<0.15 或 max<0.10 判 LOST。所有 q≥0.8 或 q≥0.5 的响应形成两块以上不连通区域判 AMBIGUOUS。后者只是明确的形状启发式，不能识别所有交叉口/宽线；若实际线会产生多峰，需按扫线数据改判据。丢线和异常时停止、reset PID 与滤波；报告讨论的有界搜索策略并未作为未知车辆的默认动作。

选择 method='dos' 时，左右分母过小而质心仍有效，可输出 method='centroid_fallback'、reason='FALLBACK_CENTER_ONLY' 或其他原始原因；方法切换复位 PID。示例为了展示接口共用一组增益，实车应根据两法局部斜率设置专属参数。低通前检查当前帧丢线/歧义，避免历史滤波值掩盖故障。

IMU 默认初态 q=[1,0,0,0]，假定起始水平且 yaw=0、初速零。非水平起始先用可信静止样本 `initialize_tilt`；该函数不提供真北。k_a=1 s⁻¹ 是重力反馈示例增益，幅值与重力相差不超过 15%、方向创新不超过 π/4 才采用反馈。二者均不能保证没有动态加速度。gyro_weight=0.7、c_L=c_R=0.001 m/count、B=0.20 m，默认角速度差门限 2 rad/s。差异过大时使用 gyro 转角并标记 angular_disagreement，轮程仍可能有平移滑移误差。

采样 dt 可接受 [0.001,0.05] s；10 ms 是名义值，不能将所有间隔硬写为 0.01。IMU 比力范数上限 16g、gyro 范数上限 35 rad/s、每轮绝对线速度上限 5 m/s 为示例异常门限，实际应按型号量程/车辆极限设置。范围内读数仍可能错误，故硬件层还需饱和/通信/新鲜度标志。

### 5.2 记录 输出与恢复语义

`demo.py` 生成 101 帧人工数据，以同一时间戳驱动两个模块，输出 `output/samples.csv`、`metadata.json`、`frames.jsonl` 和 `summary.json`。CSV 保留原始 ADC、SI IMU、累计计数、stationary、时间间隔及偏差、控制、四元数、加速度、速度、位置与 odom；数组列用 JSON 编码。元数据记录坐标、单位、参数和人工数据身份。示例接收时间等于采集时间，仅表示零传输延迟假设；实际驱动应记录真正接收时刻与可用温度。此同步离线写文件方式用于复现，实时控制必须把日志写盘移到后台队列。

第一帧惯导仅建立时间/计数基线，不积分。非单调时间拒绝且不倒退基线；正向大缺口、量程/输入故障和计数跳变拒绝并重新建立可用基线，以免下一帧把未知间隔补积分。丢帧之后以 incomplete_dead_reckoning 标记轨迹不完整，正常下一帧不意味着丢失位移被恢复。编码器缺失时不伪造轮程，恢复首帧重建轮计数基线。valid 表示更新通过程序检查；quality 描述条件或降级，二者都不是统计置信度。

纯 IMU 与平面 odom 的 yaw 分别输出。JSON 用有限数值和状态字段，拒绝把 NaN/Infinity 发送为有效测量；无效时间可用 null。标定工具只在明确 stationary 并经幅值/离散度检查的样本上求 gyro 均值。运行中可信静止标记须同时通过 IMU 与可用轮计数一致性检查，才缓慢更新 gyro 零偏/清零速度；错误静止标记返回 STATIONARY_REJECTED，不抹掉运动。

### 5.3 运行与算例验证

在 `code` 目录运行：

```powershell
python -m unittest -v test_algorithms.py
python demo.py
```

无需第三方包。若系统 python 不在 PATH，使用本机 Python 3.10 及以上解释器绝对路径。代码库没有硬件读写副作用；输出目录可删除后重跑。

<!-- TEST_RESULTS -->

### 5.4 流程图

![光电与惯导统一数据流程](流程图.svg)

### 5.5 思维导图

![算法原理与工程实现思维导图](思维导图.svg)

'''
base=base.replace('<!-- IMPLEMENTATION -->',implementation)
base=base.replace('## 实际硬件接入与验证边界','## 6 实际硬件接入与验证边界')
references='''## 7 参考资料与具体支持范围

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

主 agent 先统一约定；A 负责两种光电公式与依据，B 负责数据类型与闭环流程，C 负责惯导与记录修正。第一轮完成后冻结接口，B 与 C 分别实现独立模块，主 agent 编写集成、日志与验证并统一正文。最后由未参与代码编写的 A 独立复核公式、引用、单位、代码与覆盖范围，主 agent 处理其发现后交付。具体检查结论见验证节。
'''
base=base.replace('<!-- REFERENCES -->',references)
(ROOT/'report.md').write_text(base,encoding='utf8')
print('Integrated',len(base),'characters')
