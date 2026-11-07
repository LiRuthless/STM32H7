# 电感处理

核对代码基线 `8d5ab2f`，2026-11-07；不代表物理验收。

<a id="f05"></a>
## F05 电感偏差与对称度
### 采集、偏差和对称度

`Track_ReadAdc`四路横左/竖左/竖右/横右各读8次，去一个最大和最小，剩6次整数平均；来自12bit标度0～4095。不宣称8次均来自不同转换。
偏差为`100*(weight_x*横差 + weight_y*竖差)/(weight_xx*横和 + weight_abs*abs(竖差))`，分母零返回0；原权重15/20/22/10。对称度`abs(左右差)*100/左右和`，各对和零时独立返回0。
track_sensor唯一持有原始/滤波值、监视误差、对称度与权重。活动电磁FSM用自己的同拍测量/配置，不应通过修改共享监视量来改变活动元素判断。光电误差不写入电感误差。

### 上下文、失败与验证

停车前台和运行TIM6按既有互斥状态调用；滤波/差分整数顺序保持。权重输入由模块写接口设置，const状态地址寿命为全程序但不是并发快照。F08方向控制消费显式偏差。
对照8读/去极值、零分母、符号/截断、私有测量分离；control_equivalence既有数值基线可按需使用，不证明ADC接线、刷新速率或实车已通过。

**需求来源：** [P03 FR-01, FR-02, FR-03](../../specs/features/03-track-following/requirements.md)；[P11 FR-03](../../specs/features/11-hardening-cleanup/requirements.md)；[P13 FR-01](../../specs/features/13-control-structure/requirements.md)；[P17 FR-05](../../specs/features/17-app-architecture/requirements.md)。实现入口：[App/sensing/track_sensor.c](../../App/sensing/track_sensor.c)、[App/sensing/filter.c](../../App/sensing/filter.c)。


<a id="parameters"></a>
## 参数与更新链

| 参数/字段 | 值、范围和标度 | 来源/覆盖、生效及保存 |
|---|---|---|
| 读取次数/去极值 | 每路8次，去1最大1最小，除6；12bit 0～4095 | Track_ReadAdc 固定；每次调用生效；不持久化，不保证新转换 |
| weights x/xx/y/abs | 初始15/20/22/10；int8，无量纲，setter不校验 | track_sensor静态初值→FSM私有配置、Control_ApplyProfile及Track_PublishObservation；下一次计算/发布生效；仅RAM，不在参数镜像/Flash |

权重字段顺序是 x、xx、y、abs：分子用x及y，分母用xx及abs，不能把初始化顺序误读为横纵系数顺序。

共享滤波公式见[filters](sensors-and-calibration.md#filters)；保护动作见[起停](../system/runtime-and-safety.md#f02-stop)。
