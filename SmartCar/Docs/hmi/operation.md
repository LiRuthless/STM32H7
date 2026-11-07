# 菜单、按键与使用

现行功能说明，核对代码基线 `8d5ab2f`；2026-11-07复核，保留2026-10-31迁移来源。需求经[功能索引](../traceability.md)进入本页，再由实现表定位源码。
当前行为不等于全部已批准/实机验收，冲突见[已知问题](../known-issues.md)。

<a id="f14"></a>
## F14 菜单按键与显示
### 显示与按键

默认LCD_TARGET_EXTERNAL=0为板载dashboard，停车100ms节流刷新；=1才运行完整ADC菜单。运行不刷屏，停车前须停IMU采样避免SPI4交错。
PC13起跑/模式键不属于ADC键盘。KEY_ADC1为方向/OK，KEY_ADC2为RST/ADJUST1/ADJUST2/FOUR/BACK。扫描每路3读平均、严格区间判键，两路命中时第二路覆盖第一路；显示值另经α=0.7低通，HOME KEY1/KEY2不是瞬时原始值，稳态可核对接线。
页面HOME0，ADJUST1/2为11/12，ADC_ERR/SPD_DIS/GYRO为21/22/23。menu持有页/光标/编辑和按键沿，keys持ADC滤波，render只读绘图缓存，dashboard持时基。非法page回HOME、arrow限1～5。

### 编辑与保存

方向KP/K2P/KD每次±0.001，基础速度±100；第5项OK切FAN-R/FAN-I，±100并钳0～10000。路由到当前模式/档，速度PI与风扇共用。BACK先保存旧页位置再导航，RST当拍早退、下拍按原时点清屏。恢复ADJUST页不恢复编辑s_mode，重新按ADJUST才进入编辑；不以“恢复页面”推定可直接左右改值。
观测页gyro不是角度；当前CSV无法代替全部姿态调试。菜单/旧串口/SCB通道不同，不支持由旧UART设置所有八项。

### 验证

核对严格边界、第二路覆盖、按住不重复、松键保存、BACK旧位置、RST早退和两模式两档；g4_hmi是真实C/LCD调用/Flash镜像对照，但不是板上显示验收。屏方向、键盘接线、长按和PWM物理行为待验。

**需求来源：** [P02 FR-11, FR-18, FR-19, FR-20](../../specs/features/02-bsp-platform/requirements.md)；[P06 FR-01, FR-02, FR-03, FR-04, FR-06, FR-07, FR-08, FR-10](../../specs/features/06-hmi-tuning/requirements.md)；[P08 FR-08](../../specs/features/08-storage-chain-fix/requirements.md)；[P11 FR-08](../../specs/features/11-hardening-cleanup/requirements.md)；[P14 FR-14](../../specs/features/14-dual-mode-tracking/requirements.md)。实现入口：[App/hmi/menu.c](../../App/hmi/menu.c)、[App/hmi/menu_keys.c](../../App/hmi/menu_keys.c)、[App/hmi/menu_render.c](../../App/hmi/menu_render.c)、[App/hmi/dashboard.c](../../App/hmi/dashboard.c)。

## 本页实现与模块反查

下表是当前源码定位；公共/私有辅助函数归所属功能，不把声明视为另一实现。Core生成部分只作边界，用户区按原标记维护。

| 文件 | 主功能 | 定义/配置定位 |
|---|---|---|
| [App/hmi/dashboard.c](../../App/hmi/dashboard.c) | F14 | `Dashboard_Init`、`Dashboard_Update` |
| [App/hmi/dashboard.h](../../App/hmi/dashboard.h) | F14 | 类型/布局/配置；关联同模块C定义 |
| [App/hmi/menu.c](../../App/hmi/menu.c) | F14 | `Menu_HandleKey`、`Menu_Update`、`Menu_GetPage`、`Menu_GetArrow`、`Menu_GetFanEditIdle`、`Menu_RestorePosition` |
| [App/hmi/menu.h](../../App/hmi/menu.h) | F14 | 类型/布局/配置；关联同模块C定义 |
| [App/hmi/menu_keys.c](../../App/hmi/menu_keys.c) | F14 | `Menu_ReadKeyAverage`、`Menu_ScanKey`、`MenuKeys_GetAdc1`、`MenuKeys_GetAdc2` |
| [App/hmi/menu_keys.h](../../App/hmi/menu_keys.h) | F14 | 类型/布局/配置；关联同模块C定义 |
| [App/hmi/menu_render.c](../../App/hmi/menu_render.c) | F14 | `Menu_DrawContent`、`Menu_DrawCursor`、`MenuRender_Reset`、`MenuRender_PreparePage`、`MenuRender_Draw` |
| [App/hmi/menu_render.h](../../App/hmi/menu_render.h) | F14 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_key.c](../../BSP/bsp_key.c) | F14 | `BSP_Key_RequireRelease`、`BSP_Key_Init`、`BSP_Key_PollEvent`、`BSP_Key_StartPressed` |
| [BSP/bsp_key.h](../../BSP/bsp_key.h) | F14 | 类型/布局/配置；关联同模块C定义 |

<a id="parameters"></a>
## 按键与编辑参数

| 项 | 真实数值/范围与默认 | 来源/生效/覆盖/保存 |
|---|---|---|
| LCD_TARGET_EXTERNAL | 默认0板载；1外接完整菜单 | app_config.h编译开关，上电生效，不存Flash；两屏共SPI4互斥 |
| KEY1/KEY2 | 每路三读整数平均，再除100判严格开区间 | menu_keys.c；第二路覆盖第一路；非新转换保证，判键不经显示滤波 |
| 判键区间 | (3,9)/(9,15)/(15,20)/(20,25)/(25,31) | 对应整数商4～8/10～14/16～19/21～24/26～30；边界商不命中 |
| 对应键 | KEY1 UP/DOWN/OK/LEFT/RIGHT；KEY2 RST/ADJUST1/ADJUST2/FOUR/BACK | FOUR预留；按沿执行不自动连发；PC13走独立状态机 |
| 显示低通 | alpha=0.7，初始last=0 | 首扫初始化，判键后更新；不持久化，HOME显示值非判键输入 |
| 页面/光标 | HOME=0；11/12编辑；21/22/23观测；arrow=1～5 | page/arrow可保存镜像，非法页回HOME；编辑模式不随页面恢复 |
| 调整步进 | 方向增益±0.001；速度±100脉冲/2ms；风扇±100万分比并夹0～10000 | 当前模式/档的编辑对象，经应用协调；松键/BACK等原时机保存；状态剖面可再覆写 |
| 仪表刷新 | 停车每100ms，运行不刷新 | dashboard.c固定，无Flash；停止采样后使用共享SPI4 |

跨层：原码通道见[ADC](../platform/adc.md#f26-adc)，生效/保存与镜像差异见[参数](../services/parameters.md#f15)，方向与配置覆盖见[配置](../control/profiles.md#f12)。

显示与编辑不是同一镜像：MenuRender读取活动PID/Motor值，编辑方向参数以当前bank为对象；状态配置可能覆写活动值，旧UART活动速度PI又可领先存储镜像。页面显示不证明所有bank、查询镜像和Flash已同步。
