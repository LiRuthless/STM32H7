# 灰度标定与质心

核对代码基线 `8d5ab2f`，2026-11-07；不代表物理验收。

<a id="f06"></a>
## F06 灰度采集标定与质心
原始采集与完整帧发布主说明见[ADC与灰度采集](../platform/adc.md#gray-raw)。采集失败仅更新 acquisition_status 和 calibration_valid；旧 raw/sequence/error/line_valid 保留，调用者必须同时判定状态码，不能把旧 line_valid 当本拍成功。

### 手工标定和质心

`s_gray_black/white[16]`为代码配置，0～4095且每通道`abs(white-black)>=200`才有效；0合法，-1未填。当前32项全-1，任一失败都禁止两种模式起跑。只能填用户实测值，不提供可直接运行的伪标定。
`q=clamp(1000*(raw-black)/(white-black),0,1000)`用至少int32及C99向零截断；q<50置0，等于50保留；max(q)<200失线，等于200有效。权重`15-2*ch`，`error=100*sum(w*q)/(15*sum(q))`，有效时约±100；无有效线不继续控制。显示percent=`raw*100/4095`。

### 边界与验证

BSP只负责选址/新转换/整帧，App负责标定/归一化/线有效/误差。TIM6从首拍采双传感，停车前台也刷新；1ms记录可以复用同一个2ms灰度序号，不表示同时采样。
核对逐通道范围、对比度199/200、强度49/50/199/200、负分母极性、timeout/回绕/失败不半帧发布；既有dual灰度夹具为可选软件证据。物理方向、极性、串扰、700us和整车900us预算待上机。

**需求来源：** [P14 FR-01, FR-02, FR-03, FR-04, FR-05, FR-06](../../specs/features/14-dual-mode-tracking/requirements.md)；[P15 FR-06](../../specs/features/15-code-style-consistency/requirements.md)。实现入口：[App/sensing/gray_sensor.c](../../App/sensing/gray_sensor.c)、[App/sensing/gray_calibration.h](../../App/sensing/gray_calibration.h)、[BSP/bsp_gray.c](../../BSP/bsp_gray.c)、[BSP/bsp_adc.c](../../BSP/bsp_adc.c)、[BSP/bsp_time.c](../../BSP/bsp_time.c)。


<a id="parameters"></a>
## 参数与更新链

| 参数/字段 | 值/单位/边界 | 来源、覆盖、生效/保存 |
|---|---|---|
| black/white[16] | int16，默认全-1，合法0～4095，绝对差≥200 ADC码 | gray_calibration.h手工实测编译配置，无运行写入、无Flash；重编译上电生效 |
| q | 1000倍整数归一化；<50置0，>1000截顶 | Gray_Normalize固定公式，每帧，无持久化；极性由white-black决定 |
| line_valid | 最大强度≥200且总和非零；校准无效清0 | Gray_ComputeError，成功采集后生效；采集失败保留旧标志，必须联查状态 |
| 位置权重 | ch0～15为+15,+13,…,-15 | 固定编译公式；物理左右须板测；不持久化 |
| error/percent | error为int16 ±100标度；percent[16]为uint16 0～100，raw×100/4095整数百分比 | 每次完整帧发布；无单位换算为距离，不存Flash |

共享滤波公式见[filters](sensors-and-calibration.md#filters)；保护动作见[起停](../system/runtime-and-safety.md#f02-stop)。
