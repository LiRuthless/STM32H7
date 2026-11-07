# 持久参数、镜像与保存

现行功能说明，源码核对基线 `8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e`；2026-11-07补全。阶段保留完整设计与验收，本文只说明当前实现，不表示新增实机验证。

<a id="f15"></a>
## F15 多入口参数应用
### 三种入口与应用层

菜单操作通过system/parameter_application协调；旧UART仅停车解析d/c及vp/vi/xp/xd，字母定参数、1～4十进制位×0.01，沿原4～7字节帧规则，末尾字节不参与值解析。xp/xd写当前模式方向bank，vp/vi写共享活动速度PI并标dirty；后三秒静默保存时才捕获活动PI到镜像。
SCB使用完整八项事务，不复用旧单缓冲解析，也不开放运行换模式/档。ParamBank持四方向bank，store持镜像，system负责应用至PID/Motor和菜单位置，禁止store反调使用者。

### 状态差异与冲突

`Param_GetSpeedGains`和SCB查询镜像可在flush前仍是旧值，而vp/vi活动PI已改变；P17明确保持，R06未解决。风扇值设置/切档只改Motor状态；初始化、控制拍、首次停车才有实际PWM写入，D03未决定即时应用策略。
旧UART把一次读当完整帧，有R04拆包/粘包问题；合法数字检查不等于已实现分帧。本轮不借文档使其变成可靠协议。
验证逐入口路由、保存时点、活动/镜像值、当前模式档及风扇钳位；既有g4_hmi对2999/3000ms和Flash字节有对照。实机串口分帧、风扇应用待验/待设计。

**需求来源：** [P06 FR-05, FR-15, FR-16, FR-17](../../specs/features/06-hmi-tuning/requirements.md)；[P08 FR-07](../../specs/features/08-storage-chain-fix/requirements.md)；[P11 FR-05](../../specs/features/11-hardening-cleanup/requirements.md)。实现入口：[App/system/parameter_application.c](../../App/system/parameter_application.c)、[App/services/wireless.c](../../App/services/wireless.c)、[App/control/motor.c](../../App/control/motor.c)。

<a id="f16"></a>
## F16 参数v3格式
### 当前固定镜像

参数位于W25Q64末尾4KB `0x7FF000..0x7FFFFF`。magic=`0xA55A3C3C`，version=3，sizeof=88；CRC16-CCITT多项式0x1021、初值0xFFFF，覆盖字节8到87，不含前8字节。非打包随意强转不是新的可移植承诺，目标布局由原类型及静态断言约束。
四bank为两模式×两档，每bank方向KP/K2P/KD和基础速度；光电两档默认2/0.008/15/100，K2D固定1，共享速度PI20/0.75、运行/空闲风扇默认1600/1100。模式不写镜像，每次上电光电低档。标定不入镜像。
有效v1(52B)先补两档空闲1100，再补光电；有效v2(56B)保留所有原值并追加光电。不能用新默认覆盖旧电磁/速度/风扇用户值。布局详表见下方从实际声明核对的偏移。

**需求来源：** [P06 FR-12](../../specs/features/06-hmi-tuning/requirements.md)；[P08 FR-06](../../specs/features/08-storage-chain-fix/requirements.md)；[P13 FR-07](../../specs/features/13-control-structure/requirements.md)；[P14 FR-12, FR-15](../../specs/features/14-dual-mode-tracking/requirements.md)；[P15 FR-08](../../specs/features/15-code-style-consistency/requirements.md)。实现入口：[App/services/parameter_image.h](../../App/services/parameter_image.h)、[App/services/parameter_bank.c](../../App/services/parameter_bank.c)、[App/services/parameter_store.c](../../App/services/parameter_store.c)。

<a id="f17"></a>
## F17 迁移保存与失败
### 载入、保存和失败保证

初始化先Flash再Param_Load。未知magic/version、坏CRC按默认路径报告并尝试写回；读失败或写失败锁存存储不可用，禁止后续起跑存储门禁。有效旧镜像迁移失败，本次RAM已读/已迁移参数可保留，但参数扇区擦写不回滚，重启不保证旧镜像仍能重迁移。
参数写使用static 4096B影子读改擦写，内容不变跳过擦写，修改才擦并回写16页；不是4KB栈局部。写失败可能已部分擦写，不能宣称事务性掉电安全。
保存从指定bank取方向/基础速度，实际flush才捕获活动速度PI和菜单page/arrow。成功清dirty；失败保留dirty并锁存Flash不可用，不靠“每次尝试后清pending”防重试。旧UART最后命令后3000ms保存，起跑/模式切换先尝试flush，失败拒绝操作。菜单松键/BACK按既有时机保存。

### 平台与验证边界

器件页拆写、WIP超时、4KB静态影子读改擦写和懒擦前沿的主说明见[Flash平台](../platform/w25q64-and-flash.md#f29-flash)。参数服务只拥有镜像、bank、dirty及usable，不拥有器件时基或总线。既有参数/日志夹具属于软件模型，真实Flash故障、掉电与吞吐仍待实体。

**需求来源：** [P02 FR-21, FR-22, FR-23, FR-25](../../specs/features/02-bsp-platform/requirements.md)；[P06 FR-11, FR-13, FR-14, FR-18](../../specs/features/06-hmi-tuning/requirements.md)；[P08 FR-01, FR-02, FR-03, FR-04](../../specs/features/08-storage-chain-fix/requirements.md)；[P11 FR-01, FR-02](../../specs/features/11-hardening-cleanup/requirements.md)。实现入口：[App/services/parameter_store.c](../../App/services/parameter_store.c)、[BSP/bsp_flash.c](../../BSP/bsp_flash.c)、[BSP/bsp_w25q64.c](../../BSP/bsp_w25q64.c)。


## 实现与状态主归属

| 文件 | 主功能 | 定义/配置定位 |
|---|---|---|
| [App/control/motor.c](../../App/control/motor.c) | F08 | 协作模块；完整符号表见[F08主条目](../control/tracking-and-controllers.md#f08) |
| [App/services/command_dispatch.c](../../App/services/command_dispatch.c) | F23 | 协作模块；完整符号表见[F23主条目](communication.md#f23) |
| [App/services/parameter_bank.c](../../App/services/parameter_bank.c) | F16 | `ParamBank_LoadBanks`、`ParamBank_StoreBanks`、`Param_GetDirection`、`ParamBank_SetDirection` |
| [App/services/parameter_bank.h](../../App/services/parameter_bank.h) | F16 | 类型/布局/配置；关联同模块C定义 |
| [App/services/parameter_image.h](../../App/services/parameter_image.h) | F16 | 类型/布局/配置；关联同模块C定义 |
| [App/services/parameter_store.c](../../App/services/parameter_store.c) | F17 | `Param_CalculateCrc16`、`Param_CalculateCrcV1`、`Param_CalculateCrcV2`、`Param_CalculateCrcV3`、`Param_ClampFanDuty`、`Param_SetOpticalDefaults`、`Param_SetDefaults`、`Param_WriteMirror`、`Param_GetFan`、`Param_MarkDirty`、`Param_GetSpeedGains`、`ParamStore_Load`、`ParamStore_SetFan`、`ParamStore_SetSpeedGains`、`ParamStore_TryFlushPending`、`ParamStore_GetPosition`、`ParamStore_IsUsable`、`ParamStore_IsDirty`、`ParamStore_RequestSave`、`ParamStore_IsSaveDue` |
| [App/services/parameter_store.h](../../App/services/parameter_store.h) | F17 | 类型/布局/配置；关联同模块C定义 |
| [App/services/wireless.c](../../App/services/wireless.c) | F15 | `Wireless_Adjust` |
| [App/services/wireless.h](../../App/services/wireless.h) | F15 | 类型/布局/配置；关联同模块C定义 |
| [App/system/parameter_application.c](../../App/system/parameter_application.c) | F15 | `Param_ApplyCurrentDirection`、`Param_SetDirection`、`Param_SelectGear`、`Param_Save`、`Param_ServiceSave`、`Param_FlushPending`、`Param_Load`、`Param_SetFan`、`Param_SetSpeedGains`、`Param_TryFlushPending` |
| [App/system/parameter_application.h](../../App/system/parameter_application.h) | F15 | 类型/布局/配置；关联同模块C定义 |

<a id="image-layout"></a>
## param_image_v3_t 当前布局

小端目标布局；源头为实际声明及偏移/尺寸断言。单位和语义随字段保留。

| 偏移 | 类型/数量 | 字段 | 说明 |
|---:|---|---|---|
| 0 | uint32_t | `magic` | 原参数魔数，保持 32 位。 |
| 4 | uint16_t | `version` | 持久化版本，v1/v2 仅解码，v3 写入。 |
| 6 | uint16_t | `crc16` | 从字节 8 开始的原 CCITT 校验。 |
| 8 | uint8_t | `page` | 保存的菜单页面编号；加载校验后应用。 |
| 9 | uint8_t | `arrow` | 保存的光标，加载只接受 1..5。 |
| 10 | uint8_t×2 | `reserved` | 原两字节保留区，布局和 CRC 覆盖保持。 |
| 12 | float | `kp_v` | 两模式共用速度比例增益。 |
| 16 | float | `ki_v` | 两模式共用速度积分增益。 |
| 20 | float | `kp_x_low` | 电磁低档方向比例增益。 |
| 24 | float | `k2p_x_low` | 电磁低档方向非线性增益。 |
| 28 | float | `kd_x_low` | 电磁低档方向差分增益。 |
| 32 | int16_t | `base_speed_low` | 电磁低档基础速度，脉冲/2 ms。 |
| 34 | int16_t | `fan_duty_low` | 低档运行风扇万分比，0..10000。 |
| 36 | float | `kp_x_high` | 电磁高档方向比例增益。 |
| 40 | float | `k2p_x_high` | 电磁高档方向非线性增益。 |
| 44 | float | `kd_x_high` | 电磁高档方向差分增益。 |
| 48 | int16_t | `base_speed_high` | 电磁高档基础速度，脉冲/2 ms。 |
| 50 | int16_t | `fan_duty_high` | 高档运行风扇万分比，0..10000。 |
| 52 | int16_t | `fan_duty_idle_low` | 低档空闲风扇万分比，v2 起保存。 |
| 54 | int16_t | `fan_duty_idle_high` | 高档空闲风扇万分比，v2 起保存。 |
| 56 | float | `optical_kp_low` | 光电低档方向比例增益，v3 新增。 |
| 60 | float | `optical_k2p_low` | 光电低档方向非线性增益，v3 新增。 |
| 64 | float | `optical_kd_low` | 光电低档方向差分增益，v3 新增。 |
| 68 | int16_t | `optical_speed_low` | 光电低档基础速度，脉冲/2 ms。 |
| 70 | uint16_t | `optical_reserved_low` | 光电低档两字节保留区，写零。 |
| 72 | float | `optical_kp_high` | 光电高档方向比例增益，v3 新增。 |
| 76 | float | `optical_k2p_high` | 光电高档方向非线性增益，v3 新增。 |
| 80 | float | `optical_kd_high` | 光电高档方向差分增益，v3 新增。 |
| 84 | int16_t | `optical_speed_high` | 光电高档基础速度，脉冲/2 ms。 |
| 86 | uint16_t | `optical_reserved_high` | 光电高档两字节保留区，写零。 |

<a id="parameters"></a>
## 参数默认、来源与覆盖矩阵

数值来自 parameter_store.c 的 Param_SetDefaults / Param_SetOpticalDefaults、parameter_bank.c、system/parameter_application.c 及 app_config.h；四 bank 唯一持有者是前台写入的 s_direction[2][2]，控制拍只读。增益按既有 ADC/偏差和2ms控制标度使用，没有从这里推导 SI 物理量纲。

| 对象 / 真实字段 | 类型 / 默认值 | 范围与写入约束 | 初始化 / 覆盖 / 生效 / 持久化 |
|---|---|---|---|
| 电磁低、高档 kp/k2p/kd/base_speed | float×3/int16_t；两档均0/0/0/0 | store 对有效镜像保持原值，不用 SCB 的0～100限制反向过滤旧镜像 | memset默认→有效Flash覆盖→四bank；选中模式/档才分发活动控制；保存来自bank；默认0不是可跑标定 |
| 光电低、高档同四字段 | 两档2.0/0.008/15.0/100 | base_speed标度脉冲/2ms；旧镜像不做新的数值认证 | 无效镜像及v1/v2迁移补此默认；v3恢复覆盖；停车编辑bank，活动状态配置可再覆写；v3持久化 |
| kp_v / ki_v | float；20.0 / 0.75 | 载入CRC正确不等于增益范围/实车效果验证；SCB单独有界校验 | 上电镜像→PID；Param_SetSpeedGains同时写镜像和PID，旧UART vp/vi只写PID；真正flush抓活动值保存；两模式两档共享 |
| fan_duty_low/high | int16_t；1600万分比 | 恢复及写入钳位0～10000 | 每档一个运行值，两个模式共享；选档或设当前档更新Motor状态，实际PWM时点见电机执行；v1～v3保存 |
| fan_duty_idle_low/high | int16_t；1100万分比 | 同上 | v1迁移补默认，v2/v3恢复；停车输出不因修改镜像自动同步，见D03；v2起保存 |
| page / arrow | uint8_t；PARAM_IMAGE_DEFAULT_PAGE / ARROW = 0 / 1 | page由Menu恢复接口校验；arrow只接受1～5 | 加载时交Menu，真正保存时捕获当前位置；不持久化mode或gear |
| mode / gear | uint8_t；光电1 / 低档0 | 两模式两档；运行切换不开放 | 上电Tracking初态；停车切换先保存门禁；选档依次改gear→方向→运行风扇→空闲风扇；不保存选择 |
| dirty / dirty_tick | uint8_t / uint32_t；加载清dirty | 静默时间无符号减法≥3000ms | Param_MarkDirty重置计时；RequestSave只置dirty；保存成功才清；失败保留dirty且usable锁0，仅RAM状态不落盘 |

覆盖链不是单一等式：**默认镜像→有效Flash→显式bank/共享镜像→选中目标的活动值→每拍状态部分配置→普通赛道调参临时覆盖**。状态配置只写 fields 指定项，详见[完整配置矩阵](../control/profiles.md#f12)。临时覆盖不是回写bank，停车收尾才合入最后成功值。查询 values 读bank/镜像，effective读活动快照；两者在旧UART尚未flush及状态覆写时可以不同，R06未关闭。

### 载入与写入精确顺序

Param_Load→ParamStore_Load先建默认RAM，再读8B头，根据version读52/56/88B，分别CRC覆盖8起44/48/80B。读错只报告并锁不可用，不写默认；魔数/版本/CRC无效保留默认并要求重写。有效v1/v2复制旧字段，只补缺失项；四个风扇钳位后先发布bank，再执行必要v3重写。失败保留已发布RAM，不承诺Flash回滚。应用层随后依次恢复活动速度PI、低档方向/风扇和菜单位置。

Param_TryFlushPending先判断usable，再判断dirty；无脏不读PID/菜单。需要保存时按kp_v→ki_v→page→arrow捕获瞬时快照；store复制快照、从四bank整理方向字段，写magic/version/CRC后交BSP。底层影子读改擦写和器件超时的主说明见[Flash平台](../platform/w25q64-and-flash.md#f29-flash)。参数相对地址0对应末尾4KB；保存不是双副本事务，失败可能已擦除或部分写入，重启不保证恢复旧镜像。

### 人工核对结果与边界

本次对照上述实现及 parameter_image.h，核对四bank映射、88B偏移、v1/v2补值、3000ms、dirty/usable失败路径和活动/镜像差异；没有运行固件或行为测试。历史来源P06/P08/P13/P14/P17仍按各阶段正文解释。物理增益依据、风扇即时应用D03及UART分帧R04未由本次说明纠错作出新决定。
