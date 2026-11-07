# 参数与临时调参总览

现行功能说明，源码核对基线 `8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e`；2026-11-07补全。阶段保留完整设计与验收，本文只说明当前实现，不表示新增实机验证。

持久参数和运行临时覆盖是两个机制：上电恢复/停车编辑经bank与镜像进入活动控制；运行SCB完整八项覆盖在完整控制拍整组提交，停车后才合入最后成功值并保存。状态配置仍可按fields部分覆盖，不能把查询镜像、活动值和Flash字节合成一份状态。

<a id="f15"></a>
## F15 多入口参数应用
主说明：[持久参数应用](parameters.md#f15)，含菜单、旧UART和SCB区别、活动值与镜像差异。

<a id="f16"></a>
## F16 参数v3格式
主说明：[镜像、四bank及版本](parameters.md#f16)；[具体参数表](parameters.md#parameters)。

<a id="f17"></a>
## F17 迁移保存与失败
主说明：[载入与失败保证](parameters.md#f17)，[精确覆盖和保存顺序](parameters.md#parameters)。器件级读改擦写归[Flash平台](../platform/w25q64-and-flash.md#f29-flash)。

<a id="f18"></a>
## F18 完整拍临时调参
主说明：[事务与控制拍](runtime-tuning.md#f18)，[状态、限值及生效矩阵](runtime-tuning.md#parameters)。

## 本页实现与模块反查
持久参数模块见[parameters](parameters.md)，事务模块见[runtime-tuning](runtime-tuning.md)，不在本兼容页复制详细值表。

## param_image_v3_t 当前布局
完整88B布局迁至[镜像布局](parameters.md#image-layout)，字段/偏移和阶段原格式保持。
