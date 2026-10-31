# 三级文档契约

Phase保存FR/TC、动机、用户决定、任务、依赖、历史验收；当前详细设计通过docs准确条目定位。
docs维护功能规则、数据布局、接口、实现文件/符号/owner与验证。代码实现该契约。
新增功能先在Phase提出变更，docs标明“待实现”，二者纯文档独立提交后才改代码；完成后回填实际结果。

## 编号与模板

保留F01～F32。主锚点为`f01`～`f32`，起停另有`f02-start`、`f02-stop`，导出为`f22-export`。
每功能仅一个主页面；其他页面交叉引用，不重复维护公式/字段。跨文件协作不机械拆成每C/H一篇。
每条写用途/边界、输入输出/单位、状态与正常流程、失败/恢复、上下文与时序、实现定位、验证及限制。
冲突分开记录当前代码、既有要求、待决定和修复入口；不以文档迁移批准缺陷。

## 唯一映射位置

`docs/traceability.md`登记Phase/FR/TC→docs，不登记第二套代码详情；实现表放在各功能页。
模块反查由architecture的归属索引引导至功能页。产品C/H不加注释，不更改既有迁移JSON/证据摘要。
SCB公共协议在communication承接原文；P16/protocol保留转向入口及旧锚点兼容。
规范仍由code-style/AGENTS持有，docs验证页只引用执行规则。

## 32功能主归属

| 功能 | 主页面 | 主条目 |
|---|---|---|
| F01 初始化与三线调度 | `system/runtime-and-safety.md` | `f01` |
| F02 起停、模式与首因 | `system/runtime-and-safety.md` | `f02` |
| F03 低压持续判定 | `system/runtime-and-safety.md` | `f03` |
| F04 fatal与底层启停失败 | `system/runtime-and-safety.md` | `f04` |
| F05 电感偏差与对称度 | `sensing/sensors-and-calibration.md` | `f05` |
| F06 灰度采集标定与质心 | `sensing/sensors-and-calibration.md` | `f06` |
| F07 IMU校准滤波与积分 | `sensing/sensors-and-calibration.md` | `f07` |
| F08 方向角度速度与执行 | `control/tracking-and-controllers.md` | `f08` |
| F09 活动电磁元素 | `control/elements-and-dormant-code.md` | `f09` |
| F10 光电状态机 | `control/tracking-and-controllers.md` | `f10` |
| F11 双FSM与唯一执行 | `control/tracking-and-controllers.md` | `f11` |
| F12 部分配置与全局调参规划 | `control/tracking-and-controllers.md` | `f12` |
| F13 legacy与未接入元素 | `control/elements-and-dormant-code.md` | `f13` |
| F14 菜单按键与显示 | `hmi/operation.md` | `f14` |
| F15 多入口参数应用 | `services/parameters-and-tuning.md` | `f15` |
| F16 参数v3格式 | `services/parameters-and-tuning.md` | `f16` |
| F17 迁移保存与失败 | `services/parameters-and-tuning.md` | `f17` |
| F18 完整拍临时调参 | `services/parameters-and-tuning.md` | `f18` |
| F19 日志格式容量 | `services/recording-and-export.md` | `f19` |
| F20 队列前台写入与故障 | `services/recording-and-export.md` | `f20` |
| F21 CSV与会话边界 | `services/recording-and-export.md` | `f21` |
| F22 二进制流式导出 | `services/recording-and-export.md` | `f22` |
| F23 SCB帧JSON与命令 | `services/communication.md` | `f23` |
| F24 会话可靠传输与回绕 | `services/communication.md` | `f24` |
| F25 独立上传 | `services/communication.md` | `f25` |
| F26 ADC编码器PWM | `platform/board-and-bsp.md` | `f26` |
| F27 UART失败返回 | `platform/board-and-bsp.md` | `f27` |
| F28 DL1B阻塞边界 | `platform/board-and-bsp.md` | `f28` |
| F29 时钟DMA采样与驱动 | `platform/board-and-bsp.md` | `f29` |
| F30 构建烧录与再生 | `guides/build-and-flash.md` | `f30` |
| F31 验证证据与复现 | `guides/validation.md` | `f31` |
| F32 标准R08与维护 | `guides/validation.md` | `f32` |
