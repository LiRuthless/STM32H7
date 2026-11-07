# 感知功能总览

当前源码基线 `8d5ab2f`；原码驱动归 platform，业务转换归 sensing，停车编排归 system。功能身份及旧锚点保持。

<a id="f05"></a>
## F05 电感处理

详细算法、状态和参数：[主说明](electromagnetic.md#f05)。

<a id="f06"></a>
## F06 灰度标定与质心

详细算法、状态和参数：[主说明](grayscale.md#f06)。

<a id="f07"></a>
## F07 姿态处理

详细算法、状态和参数：[主说明](attitude.md#f07)。

<a id="filters"></a>
## 滤波共享组件

`Filter_InitLowpass`保存调用者alpha并清last_output；更新为`alpha*input+(1-alpha)*last_output`。高通为`alpha*(last_output+input-last_input)`，随后更新last_input。状态和单位由调用者持有，接口不作参数校验，不访问其他业务状态；当前IMU高通及编码器/电池低通不共用历史。


来源：Phase03/05/14/17；滤波状态由调用者持有，不落Flash。各模块映射见[架构](../architecture.md)。
