# Phase 06: 人机交互与调参 — 需求

> as-built 追认规约：以下需求逆向自现状代码，描述可观测、可检验的现有行为。
> 计划见 [plan.md](plan.md)，验证见 [validation.md](validation.md)。

## 功能需求

### 显示

- FR-1: 系统提供编译期二选一的显示方案，由 `app_config.h` 的 `LCD_TARGET_EXTERNAL` 选择：
  `=0`（默认）板载 0.96 寸屏单页仪表盘；`=1` 外接屏完整按键菜单。两种方案互斥（共用 SPI4），不可同时启用。
- FR-2: 仪表盘（`dashboard.c`）为单页布局，仅停车态刷新，数值区按 100ms 节流，内容含：
  四路电感滤波值、循迹偏差、方向环输出、电池电压（12bit 值）、DL1B 距离（mm，8192=无效）、
  陀螺 xyz（°/s 取整）、左右轮速、里程、kernel 主状态机状态。
- FR-3: 菜单（`menu.c`）页面结构：主页 HOME(0) 显示 3 个入口项（ADC_ERR / SPD_DIS / GYRO）
  + 底部两路按键 ADC 值（K1/K2，供接线核对）与电池电压；OK 键跳转到 `page = 20 + arrow`
  （即监视页 21/22/23）；监视页按 BACK 返回主页。
- FR-4: 监视页内容——ADC_ERR(21)：四路电感 + ERR 偏差 + SX/SY 对称度；
  SPD_DIS(22)：左右轮速 + 里程；GYRO(23)：陀螺 xyz（°/s，1 位小数）。
- FR-5: 调参页 ADJUST1(11)=慢速档 / ADJUST2(12)=快速档，仅经 ADJUST1/ADJUST2 物理键一键进入，
  进入即加载对应档位参数到全局（`Param_SelectGear`）。每页 5 项，光标循环（1↔5）：
  KP_x / K2P_x / KD_x（LEFT/RIGHT 步进 ±0.001）、base_speed（±100）、风扇值（±100）。
  第 5 项以 OK 在 FAN-R 运行值和 FAN-I 空闲值间切换，LEFT/RIGHT 修改当前选择。
- FR-6: 屏幕只在停车状态刷新；运行中（TIM6/TIM7 启动后）主循环不再调用菜单/仪表盘。

### 按键

- FR-7: ADC 分压键盘两路：KEY_ADC1(PA2) = UP / DOWN / OK / LEFT / RIGHT；
  KEY_ADC2(PA3) = RST / ADJUST1 / ADJUST2 / FOUR / BACK（FOUR 为预留，无动作）。
  每路采样 3 次平均，按 `/100` 区间判定键值，沿检测消抖（同一键持续按住不重复触发）。
- FR-8: RST 键清屏重绘当前页面（不改变页面与光标）。
- FR-9: 启动键 K1(PC13，下拉输入，按下为高电平) 采用按下沿检测 + 30ms 消抖，
  检测有效后进入起跑流程（见 Phase 01/07）。
- FR-10: 上机时可在 HOME 页读取两路按键 ADC 显示值，用于核对两组键盘是否接反
  （对应 README 上机检查清单第 2 条；显示值为低通滤波后值，稳态读数与原始值一致）。

### 参数持久化

- FR-11: 上电初始化即从 W25Q64 加载参数（`Param_Load`）；magic / version / CRC16 任一校验失败时，
  载入默认值（KP_v=20.0、KI_v=0.75、方向环与速度参数为 0、两档运行风扇值 1600、空闲值 1100、page=HOME、arrow=1）并尝试回写 Flash。
- FR-12: 加载后默认挂慢速档（方向环参数取慢速档位），KP_v/KI_v 直接恢复到全局。
- FR-13: 参数保存时机：调参页内修改参数后按键松开（无键按下且参数已脏），或在调参页按 BACK 退出时，
  执行一次 `Param_Save` 写回 Flash（保存内容含当前档位参数、菜单 page/arrow、KP_v/KI_v）；串口命令静默 3 秒后保存，起跑前刷新待保存参数。
- FR-14: 菜单 page（限 HOME/21/22/23/11/12，非法值回主页）与 arrow（限 1~5，非法回 1）随参数持久化，
  掉电重启后恢复到上次页面。

### 串口无线调参

- FR-15: 串口命令仅在停车态解析（USART1，115200 8N1）。单字母命令：
  `d` = 阻塞导出运行日志 CSV（联动 [Phase 07](../07-datalog/requirements.md)）；
  `c` = 复位日志区（回显 `log erasing...` → `log erased` / `erase fail`）。
- FR-16: 参数设置帧格式：2 字节命令字母 + 1~4 位 ASCII 数字 + 1 终止字节（总长 4~7 字节，
  其余长度丢弃），设定值 = 数字 × 0.01。支持的命令字：`vp`=KP_v、`vi`=KI_v、`xp`=KP_x、`xd`=KD_x。
  例：`vp15\n` → KP_v = 0.15。命中时回显 `xx=NN`（NN = 设定值 × 100 四舍五入）。
- FR-17（能力边界，如实记录）: 串口协议**不支持** K2P_x、base_speed、风扇值、速度档切换；
  这些参数只能经菜单调参页修改。串口协议与菜单能力不对齐为现状，非缺陷整改对象（见 plan.md 风险表）。
- FR-18（Phase 08 修复）: 串口改参写 RAM 全局量并在最后一条命令后静默 3 秒保存；起跑前刷新待保存参数。

## 技术约束

- TC-1: App 层只经 `bsp.h` 接触硬件（技术栈硬性约束 1）。现状例外：`dashboard.c` 直用 `HAL_GetTick()`
  （时基服务），已记录为轻微越层，治理见 Phase 11。
- TC-2: 编译开关集中在 `app_config.h`：`LCD_TARGET_EXTERNAL`（显示目标）、`DATALOG_ENABLE` / `DATALOG_DIV`（联动 Phase 07）。
- TC-3: 屏幕刷新与 IMU 采样共用 SPI4 总线，屏幕刷新只允许发生在停车态（技术栈硬性约束 3 的架构前提）。
- TC-4: 参数存储后端为板载 W25Q64（SPI1），不使用片内 Flash；参数区固定为末尾 4KB 扇区（0x7FF000），
  经 BSP 读-改-擦-写服务访问。
- TC-5: `param_store_t` 落盘布局即兼容性契约：`PARAM_VERSION` 变更意味着旧数据不兼容（校验失败后回默认值）。
- TC-6: 占空比对外一律 0~10000 万分比语义（`fan_duty`、`FAN_DUTY_*` 同此标度，技术栈硬性约束 4）。

## 接口约定

### 页面与按键行为表

| page | 名称 | 内容 | UP/DOWN | OK | BACK | LEFT/RIGHT |
|---|---|---|---|---|---|---|
| 0 | HOME | 3 入口项 + K1/K2 按键值 + 电池 | 光标循环（1↔3） | 跳 page=20+arrow | 无动作 | 无动作 |
| 21 | ADC_ERR | 电感×4 / ERR / SX / SY | 光标循环（1↔4） | 无动作 | 回 HOME | 无动作 |
| 22 | SPD_DIS | SpL / SpR / Dis | 同上 | 无动作 | 回 HOME | 无动作 |
| 23 | GYRO | GX / GY / GZ | 同上 | 无动作 | 回 HOME | 无动作 |
| 11 | ADJUST1 | 慢速档 5 项（右上角标 "1"） | 光标循环（1↔5） | 第 5 项切换 FAN-R/FAN-I | 保存并回 HOME | 参数 −/+ 步进 |
| 12 | ADJUST2 | 快速档 5 项（右上角标 "2"） | 同上 | 同上 | 保存并回 HOME | 同上 |

调参页 LEFT/RIGHT 步进：arrow 1~3（KP_x/K2P_x/KD_x）±0.001；arrow 4（base_speed）±100；arrow 5 当前选择的运行/空闲风扇值 ±100，范围 0~10000。OK 仅在 arrow=5 时切换 FAN-R/FAN-I。

### ADC 键盘判定表（3 次平均后 `/100` 区间）

| 区间（ADC/100） | 代表值 | KEY_ADC1(PA2) | KEY_ADC2(PA3) |
|---|---|---|---|
| 4~8 | 约 542 | UP | RST |
| 10~14 | 约 1262 | DOWN | ADJUST1 |
| 16~19 | 约 1840 | OK | ADJUST2 |
| 21~24 | 约 2467 | LEFT | FOUR（预留） |
| 26~30 | 约 3082 | RIGHT | BACK |

### 串口协议（USART1，115200 8N1，仅停车态）

| 帧 | 长度 | 含义 | 回显 |
|---|---|---|---|
| `d` + 终止字节 | 2 | 导出日志 CSV | `dump start` → CSV → `EOF` |
| `c` + 终止字节 | 2 | 复位日志区 | `log erasing...` → `log erased` / `erase fail` |
| `vp` + 1~4 位数字 + 终止字节 | 4~7 | KP_v = 数字×0.01 | `vp=NN` |
| `vi` + 数字 + 终止字节 | 4~7 | KI_v = 数字×0.01 | `vi=NN` |
| `xp` + 数字 + 终止字节 | 4~7 | KP_x = 数字×0.01 | `xp=NN` |
| `xd` + 数字 + 终止字节 | 4~7 | KD_x = 数字×0.01 | `xd=NN` |

终止字节内容不参与解析（任意字节，通常为 `\n`/`\r`）；数字按十进制位权展开，最高 4 位（千位×10.0）。
未知命令字静默丢弃（无回显）。

### 参数存储布局（W25Q64 末尾 4KB 扇区 0x7FF000，`param_store_t`）

| 字段 | 类型 | 说明 |
|---|---|---|
| magic | uint32 | 固定 `0xA55A3C3C` |
| version | uint16 | 结构体版本，当前 = 2；v1 迁移规则见下 |
| crc16 | uint16 | CRC16-CCITT（poly 0x1021，初值 0xFFFF），覆盖自 `page` 起至结构体末尾（不含头部 8 字节） |
| page / arrow / reserved[2] | uint8 | 菜单页面（0/11/12/21/22/23）与光标（1~5） |
| KP_v / KI_v | float | 速度环 PI（4 字节完整存取） |
| KP_x_low / K2P_x_low / KD_x_low | float | 慢速档方向环参数 |
| base_speed_low / fan_run_low / fan_idle_low | int16 | 慢速档基础速度 / 运行风扇值 / 空闲风扇值 |
| KP_x_high / K2P_x_high / KD_x_high | float | 快速档方向环参数 |
| base_speed_high / fan_run_high / fan_idle_high | int16 | 快速档基础速度 / 运行风扇值 / 空闲风扇值 |

> Phase 08 将参数镜像升至 version 2；旧 version 1 的 `fan_duty_low/high` 迁移为运行值，新增空闲值设为 1100。镜像地址不变。

## 非目标（Non-goals）

- 不自建 PC 上位机（复用串口工具 / 逐飞助手 / Excel）。
- 不支持运行中调参与屏幕刷新（架构约束，见 TC-3）。
- FOUR 键仅为协议占位，不分配功能。
- 不扩展串口协议以覆盖 K2P_x / base_speed / fan_duty / 档位切换（现状如实记录，扩展另行排期）。
- 不做参数多槽位/多 profile 管理（仅慢/快双档）。
