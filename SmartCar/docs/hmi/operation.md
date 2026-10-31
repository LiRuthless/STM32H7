# 菜单、按键与使用

现行功能说明，核对基线 `68cb508`；2026-10-31迁移。需求经[功能索引](../traceability.md)进入本页，再由实现表定位源码。
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
| [BSP/bsp_key.c](../../BSP/bsp_key.c) | F14 | `BSP_Key_RequireRelease`、`BSP_Key_Init`、`BSP_Key_PollEvent`、`if`、`BSP_Key_StartPressed` |
| [BSP/bsp_key.h](../../BSP/bsp_key.h) | F14 | 类型/布局/配置；关联同模块C定义 |
