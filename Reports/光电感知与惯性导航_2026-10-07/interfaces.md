# 第二轮冻结接口与定义

第一轮 A/B/C 已完成，采用 conventions.md 与下述决定。Python 3.10+ 标准库；B 仅写 code/optical.py，C 仅写 code/inertial.py；主 agent 写 demo.py/test_algorithms.py 与报告。

## 光电

- OpticalResult dataclass: e:float, e_y_m:float|None, valid:bool, reason:str, method:str, strength:float；无效 e=0 是占位，禁止用于 PID。
- normalize(raw, background, target, raw_min=0.0, raw_max=1023.0, min_span=20.0) -> list[float]；逐通道 clip((raw-bg)/(target-bg))。参数标定错误/长度/非有限值/原始超量程 ValueError。极性可正可负。不把 ADC=0/1023 必然判断线，真实硬件诊断由外层补充。
- digital(levels, target_level=0) -> list[float]；只允许 0/1。
- centroid(q, positions_m=(-.02,-.01,0,.01,.02), min_strength=.15) -> OpticalResult；e_y=Σyq/Σq,e=e_y/max|y|。
- difference_over_sum(q, positions_m=同上, min_denominator=.05) -> OpticalResult；L/R 对正负位置通道取均值，中心排除；D 小而中心强 reason=CENTER_ONLY，否则 LOW_DENOMINATOR。
- PID(kp=.8,ki=.1,kd=.02,u_max=.5,tau_d=.02).step(e,dt_s)->float；reset()；首帧导数 0，后向欧拉导数低通、条件积分；dt_s 在 [.001,.05] s 内，否则 ValueError；可调参数构造时检验。属性 integral, derivative 可审计。
- OpticalController(method='centroid'|'dos', background=五个100,target=五个900,positions_m=同上,tau_q=0,pid=可选PID,v_base=.3,k_u=.3,v_max=.6)。step(t_s,raw,mode='analog',target_level=0)->dict。
- 结果 dict: t_s,q,e,e_y_m,valid,reason,method,strength,u,v_left_m_s,v_right_m_s。首帧有效使用名义 dt=.01；后续 actual dt 范围 [.001,.05]。tau_q 可选一阶低通，首帧初始化为测量。
- controller 显式检查 shape：所有 q>=.8 或强响应阈值 .5 形成两个不连通块 -> AMBIGUOUS 并停止（阈值示例，不保证识别任意路口）。total/peak 低 -> LOST。
- DOS invalid 时若 centroid valid 且无空间歧义，回退 result.method='centroid_fallback', reason='FALLBACK_'+原原因；PID reset 以避免尺度切换脉冲。两法局部尺度不同，示例共用参数仅演示，生产需专属增益。
- 任何异常输入及时序无效 controller 返回 valid=False, reason，u=0,轮速=0, reset PID/滤波。非单调时间不倒退基线；正向大缺口重新基线，以便下一帧恢复。禁止沿用旧偏差为正常数据。
- vL=v_base-k_u*u,vR=v_base+k_u*u；构造时约束 k_u*u_max<=min(v_base,v_max-v_base)，避免下游限幅破坏抗饱和。

## 惯导

- IMUSample(t_s:float,accel_m_s2:tuple[3],gyro_rad_s:tuple[3],count_left:int|None=None,count_right:int|None=None,stationary:bool=False) dataclass；SI 车体系且编码器已同步。
- G=9.80665；四元数 [w,x,y,z]，Hamilton，b→n；姿态 ω_c=ω-b_g+k_a*(f_hat×R^T up)，仅幅值/创新门限通过时反馈；gyro 比例校正属于驱动层。默认 k_a=1 s^-1 可调；初始已知单位姿态或可信静止 initialize tilt。
- Navigator(k_a=1.,gyro_bias=(0,0,0),accel_bias=(0,0,0),gyro_weight=.7,meters_per_count_left=.001,meters_per_count_right=.001,wheelbase_m=.2,...)，update(sample)->dict；第一帧记录时间/计数基线、不积分（返回 reason=INITIALIZED）。
- 输出 dict: t_s,valid,reason,quality,q_nb,a_n_m_s2,v_n_m_s,p_n_m,pure_yaw_rad,roll_rad,pitch_rad,odom（None 或 {x_m,y_m,yaw_rad,speed_m_s,quality}）；姿态与 odom yaw 分开。无效回包不伪装更新成功。
- 纯积分：p+=v_old dt+.5*a_n dt²；v+=a_n dt。恒加速测试设置 k_a=0 且 q 初始单位，t0 帧只建立基线，然后 100步。静止外部标记且IMU幅值/gyro与轮计数均一致时可 ZUPT/缓慢 gyro_bias 更新，绝不自动根据低gyro判匀速为静止；不回滚p。非法 stationary 标记不清速度并标记 STATIONARY_REJECTED。
- 平面 odom：ds_L=cL*dcountL,ds_R=cR*dcountR,ds均值,dψ_enc=(dsR-dsL)/B,dψ_g=(gyro_z-bg_z)*dt,dψ=λ*dψ_g+(1-λ)*dψ_enc；位置中点 ds cos/sin(yaw_old+dψ/2)，yaw wrap[-π,π)，speed=ds/dt。报告明确小角近似，不冒充精确圆弧。
- dt有效范围[.001,.05]；非单调时间拒绝不改基线；向前大缺口拒绝并重建时间/计数基线。所有读数有限，accel范数<=16G，gyro范数<=35rad/s；计数跳变用每轮abs(ds)/dt<=5m/s守门；缺失编码器时 odom不更新、轮基线失效，恢复首帧重新基线。
- 两角速度差超过可调 threshold（默认2rad/s）时 lambda=1，quality='angular_disagreement'，仍记录轮程；这不能修复平移打滑。
- 原始输入非法/量程超限时不积分，向前时间建立新基线以免下一帧补积异常间隔。代码说明位置已因失帧不完整，quality不能声称高精度。配置错误抛 ValueError。
- 可提供 stationary_gyro_bias(samples) 辅助标定，须确认stationary+幅值+gyro离散程度；不声称单姿态校准一般三轴加速度偏置。

## 记录与主程序

主 agent 负责 CSV 原始+状态完整记录、旁车 metadata.json、JSON 上位机输出，demo 同目录导入两个模块，tests 根据以上接口编写。代码开发任务不得修改报告和其他 agent 文件；有接口问题先消息主 agent，不猜测。
