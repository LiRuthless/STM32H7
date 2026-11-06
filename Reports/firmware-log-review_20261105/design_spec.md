<!-- ppt-master-schema: design-spec/v1 -->
# Firmware Log Review - Design Spec

## I. Project Information

| Item | Value |
| --- | --- |
| Project Name | firmware-log-review |
| Canvas Format | ppt169 / 1280 × 720 |
| Page Count | 24 |
| Primary Language | zh-CN |
| Target Audience | 熟悉C与STM32的固件技术评审人员 |
| Communication Intent | 深入讲解日志与上传实现，审查并发、故障和验证边界 |
| Desired Audience Outcome | 能够追踪关键函数、解释数据归属并识别硬件证据缺口 |
| Core Message / Ask / Action | 同一96B记录进入独立Flash与上传队列，Flash及安全优先，导出仅成功前缀 |
| Delivery Context | 现场技术评审25至30分钟 |
| Artifact Afterlife | 可编辑PPTX与讲者备注，团队维护参考 |
| Reading Mode | balanced |
| Content Strategy | 保留必要讲解，减少小字注释，以布局和排版解释关系 |
| Design Style | 数据流主线：浅底深蓝青色，流程/时序/内存示意 |
| AI Image Acquisition Path | not applicable |
| Generation Mode | continuous |
| Spec Refinement | disabled |
| Speaker Notes | enabled — final Stage-2 proactive policy |
| Custom Animations | disabled — final Stage-2 proactive policy |
| Narration Audio | disabled — final Stage-2 proactive policy |
| Created Date | 2026-11-05 |

## II. Canvas Specification

| Property | Value |
| --- | --- |
| Format | ppt169 |
| Dimensions | 1280 × 720 |
| viewBox | `0 0 1280 720` |
| Margins | 56 px |
| Content Area | 56 160 1168 480 |

## III. Visual Theme

### Theme Style

- **Mode**: custom
- **Mode Behavior**: 以同一96B记录为主线，先给整体路径，再沿发布、写入、封包、导出逐层解释边界。
- **Visual style**: custom
- **Visual Style Behavior**: 浅底深蓝标题，青色标记数据流；大字图解，独立关系用位置与线连接，必要解释单独成页。
- **Theme**: 数据流技术评审
- **Tone**: 准确、直接、证据明确

### Color Scheme

| Role | HEX | Purpose |
| --- | --- | --- |
| Background | #FFFFFF | Background |
| Secondary background | #F5F8FB | Secondary background |
| Primary | #123252 | Primary |
| Accent | #007E87 | Accent |
| Secondary accent | #AD551D | Secondary accent |
| Body text | #233647 | Body text |
| Secondary text | #52697B | Secondary text |
| Divider | #CBD9E2 | Divider |

## IV. Typography System

### Font Plan

| Role | Character (Reference) | Primary | English if non-English | Fallback tail |
| --- | --- | --- | --- | --- |
| Title | 清晰无衬线 | Microsoft YaHei | Microsoft YaHei | sans-serif |
| Body | 清晰无衬线 | Microsoft YaHei | Microsoft YaHei | sans-serif |

- **Title stack**: Microsoft YaHei
- **Body stack**: Microsoft YaHei

### Font Size Hierarchy

| Purpose | Anchor Size (px) |
| --- | ---: |
| Body | 30 |
| Title | 42 |
| Subtitle | 30 |
| Annotation | 24 |
| Display | 64 |

## V. Layout Principles

### Deck-wide Direction

- **Hierarchy direction**: 标题→主图→结论；必要限制以正文说明。
- **Composition tendency**: 路径、泳道、分叉和内存区域随语义变化，不重复套卡片。
- **Cross-page continuity**: 青色为上传/流转，深蓝为Flash/状态，橙色为限制。
- **Spacing posture**: 开放，长内容分行而不缩小。
- **Spacing anchors**: page margin 56; block gap 24; column gutter 40; corner radius 12; body leading 44

## VI. Icon Usage Specification

- **Primary bundled library**: none

| Icon Path | Suitable Scenarios |
| --- | --- |

## VIII. Image Resource List

| Filename | Dimensions | Ratio | Purpose | Type | Image pattern | Crop Policy | Acquire Via | Status | Reference | text_policy | page_role |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

## IX. Content Outline

### Part 1: 记录、存储、上传与导出

#### Slide 01 - 固件日志记录与上传

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: link: 同一采样记录分别供存储和上传使用
- **Title**: 固件日志记录与上传
- **Core message**: 从 1 ms 采样到 Flash、实时上传与停车导出
- **Content**: 从 1 ms 采样到 Flash、实时上传与停车导出

#### Slide 02 - 一份记录，走向两条独立通道

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: order / link: 数据按页面所示顺序流转
- **Title**: 一份记录，走向两条独立通道
- **Core message**: Flash 保留运行数据；实时上传服务在线观察
- **Content**: TIM6｜2 ms 控制快照；TIM15｜1 ms 合成 96 B 记录；Flash 队列 → W25Q64；上传队列 → SPI2 → ESP32 → 电脑

#### Slide 03 - 职责边界决定状态归谁所有

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: membership: 各模块分别拥有列明状态；link: logger经log_store访问Flash
- **Title**: 职责边界决定状态归谁所有
- **Core message**: 业务状态与协议状态分开，Flash 只有一个 App 访问点
- **Content**: run_record｜头与控制快照；logger｜会话、队列与故障；log_store｜Flash 字节访问；telemetry / scb_link｜上传与链路；log_export｜导出身份与进度

#### Slide 04 - 1 ms 采样，不等于 1 ms 控制

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: order / link: 数据按页面所示顺序流转
- **Title**: 1 ms 采样，不等于 1 ms 控制
- **Core message**: 相邻记录可以共享控制序号，采样 tick 仍逐拍保留
- **Content**: TIM6：完整控制 → 发布快照；TIM15：复制最近快照 → 补本拍采样；控制 / 灰度序号允许重复；tick 保留 1 ms 时间索引

#### Slide 05 - 96 B 固定记录：把时间与状态放在一起

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: membership: 四段字段共同组成96B记录
- **Title**: 96 B 固定记录：把时间与状态放在一起
- **Core message**: Flash 与网络承载相同原始字节，保留区清零
- **Content**: 0–31｜tick、电感、轮速、输出、IMU、距离、电池、状态；32–63｜16 路完整灰度原始 ADC；64–81｜双序号、双误差、保护、模式、首因；82–95｜14 B 保留区

#### Slide 06 - 256 B 会话头：解释这一次运行

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: membership: 格式/传感/控制/容量共同解释当前会话
- **Title**: 256 B 会话头：解释这一次运行
- **Core message**: 版本 2 固定；头部保存起跑时配置快照
- **Content**: 格式｜magic、版本、长度、周期；传感｜黑白标定、权重、阈值；控制｜mode / gear、两模式参数、PI、风扇；容量｜87336 条；CRC16 覆盖字节 8–255

#### Slide 07 - 先建立有效日志，再允许起跑

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: order / link: 数据按页面所示顺序流转
- **Title**: 先建立有效日志，再允许起跑
- **Core message**: 擦除、写头、回读比较全部成功，才发布 active
- **Content**: 导出互斥检查；配置 / 标定检查；擦首扇区 → 写 256 B 头；回读并逐字节比较；header_valid = 1 → active = 1

#### Slide 08 - Flash 队列：1024 槽，最多积压 1023 条

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: link: TIM15生产与前台消费通过环形队列交接；contrast: 分配槽数与可用条数
- **Title**: Flash 队列：1024 槽，最多积压 1023 条
- **Core message**: 预留一个空槽，让 head == tail 唯一表示空队列
- **Content**: TIM15 只推进 head；前台只推进 tail；next(head) == tail → 队满；98304 B 分配；可缓存约 1.023 s 采样

#### Slide 09 - 发布与释放，各有一个不可提前的时点

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: order: 写槽先于发布；完成Flash先于释放；contrast: 成功与失败路径
- **Title**: 发布与释放，各有一个不可提前的时点
- **Core message**: 先写内容再发布 head；Flash 成功后才释放 tail
- **Content**: 生产者｜写完整 96 B → DMB → head；消费者｜固定 tail → 复制批次 → Flash 写入；成功｜written += n → DMB → tail；失败｜保留 tail，锁存写错，不重试

#### Slide 10 - Flash 空间：日志与参数有明确边界

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: membership: 日志头和记录属于日志区，参数在末扇区；order: 记录顺序追加
- **Title**: Flash 空间：日志与参数有明确边界
- **Core message**: 单次运行追加记录；新起跑覆盖上一会话
- **Content**: 0x000000｜256 B 会话头；0x000100 起｜96 B × 87336 条；0x7FF000 起｜最后 4 KiB 参数区；日志约 87.336 s；不循环覆盖

#### Slide 11 - 前台最多写 8 条，底层按页拆写

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: order / link: 数据按页面所示顺序流转
- **Title**: 前台最多写 8 条，底层按页拆写
- **Core message**: 768 B 批次降低服务频度；跨扇区先擦再写
- **Content**: 运行｜不足 8 条暂不写；停车｜允许不足 8 条的尾包；BSP｜跨 4 KiB 扇区先懒擦；BSP｜拆成不跨 256 B 页的片段

#### Slide 12 - 日志故障停车，网络拥塞不新增停车

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: order: 请求停车先于前台收尾；contrast: 日志故障与上传拥塞
- **Title**: 日志故障停车，网络拥塞不新增停车
- **Core message**: 先锁存故障与首因，前台再有界收尾
- **Content**: 容量满 / 队满 / 写错 → 安全停车请求；ISR 停生产与运动；Flash 留给前台；Datalog_Stop：最多 128 批，每批喂狗；写错后保留成功前缀，尾包不保证落盘

#### Slide 13 - 上传队列独立，丢弃不挪动 Flash 的 tail

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: contrast: 上传槽与Flash槽独立；order: 索引递增先于队满判断
- **Title**: 上传队列独立，丢弃不挪动 Flash 的 tail
- **Core message**: 256 槽全容量；满时丢本条并保留索引缺口
- **Content**: 同一完整记录 → 上传槽副本；head − tail >= 256 → dropped++；index 每次应产生记录都递增；新 run_id 重置索引；旧运行积压被丢弃

#### Slide 14 - 封包有三条边界：运行、版本、连续索引

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: membership: 同一包的记录必须满足三个约束；order: 边界切开相邻批次
- **Title**: 封包有三条边界：运行、版本、连续索引
- **Core message**: 通常每 10 ms 最多 10 条；边界到来可提前封包
- **Content**: 同 run_id；同 revision；index 连续；任一变化 → 先结束旧批次

#### Slide 15 - 24 B 元数据，让缺口与归属可解释

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: membership: 六个u32与N条记录共同组成上传负载
- **Title**: 24 B 元数据，让缺口与归属可解释
- **Core message**: 联合身份是 session_id + run_id + record index
- **Content**: 记录索引 / 参数 revision；STM32 丢记录数 / ESP32 丢包数；首条 tick / run_id；24 + 96 × N；N = 1…10

#### Slide 16 - STM32 到电脑：实现边界必须可见

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: link: STM32经ESP32桥接到电脑；membership: 网络栈属于外部端
- **Title**: STM32 到电脑：实现边界必须可见
- **Core message**: STM32 提供 SCB / SPI2；桥接与网络是跨端契约
- **Content**: STM32｜日志、上传队列、SCB、SPI2 DMA；ESP32｜SPI 从机 ↔ Wi-Fi 桥接；电脑｜TCP 命令 / 导出；UDP 实时数据；TCP 5000；UDP 默认 5001

#### Slide 17 - SCB/1：同一帧，传输外壳不同

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: membership: 头与payload组成帧；contrast: SPI/TCP/UDP封装
- **Title**: SCB/1：同一帧，传输外壳不同
- **Core message**: 头部显式小端编码；先完整验证，再进入业务
- **Content**: 32 B 头｜类型、长度、身份、seq / ack、CRC32；payload ≤ 992 B；SPI｜固定 1024 B，未用尾部清零；TCP｜头 + 有效负载；UDP｜一报一帧

#### Slide 18 - 可靠结果与实时数据，采用不同取舍

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: order: 发送调度有优先顺序；contrast: 可靠业务和可丢遥测
- **Title**: 可靠结果与实时数据，采用不同取舍
- **Core message**: 传输 ACK 只表示接管；TELEMETRY 不做可靠重传
- **Content**: 每方向最多 1 个可靠帧在途；精确 ack_seq 才释放；200 ms 后重发；最多重发 3 次；800 ms 无有效帧重握手；结果 / 事件优先；实时包可在等待 ACK 时发送

#### Slide 19 - DMA 事务：中断只交接，前台做解析

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: order: 许可→启动→完成→解析→释放
- **Title**: DMA 事务：中断只交接，前台做解析
- **Core message**: READY 给许可；TX / RX 缓冲在途期间不可复用
- **Content**: READY IRQ｜只累计许可；前台｜填帧 → CS 低 → 启动 DMA；完成 IRQ｜CS 高 → 标记 RX 可读；前台｜解析 → ReleaseRx；超时 10 ms 恢复

#### Slide 20 - 停车导出：先冻结成功前缀，再顺序读取

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: order / link: 数据按页面所示顺序流转
- **Title**: 停车导出：先冻结成功前缀，再顺序读取
- **Core message**: 一个 transfer_id，窗口 1，最多 960 B 原始数据 / 块
- **Content**: GET_LOG_META｜只查内存头与 written；LOG_BEGIN｜停车收尾完成，冻结长度；LOG_READ｜按 next_offset 顺序读取；LOG_END｜全部交付，返回整体 CRC32 并解锁

#### Slide 21 - 重读最近块不推进，也不续租

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: contrast: 新读取与重复读取；order: 无进展到期释放
- **Title**: 重读最近块不推进，也不续租
- **Core message**: 只有新的有效读取，才推进 offset、CRC 与 10 s 租期
- **Content**: 重复最近 offset + length → 返回缓存；乱序 / 越界 / 身份错误 → 拒绝；10 s 无新进展 → 释放导出互斥；会话变化立即取消；TCP 断开靠超时释放

#### Slide 22 - 三层校验，各自回答不同问题

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: contrast: 三类校验覆盖不同字节域
- **Title**: 三层校验，各自回答不同问题
- **Core message**: 能验证字节一致性，不能补回未成功落盘的尾包
- **Content**: 头 CRC16｜本次会话配置是否完整；帧 CRC32｜这一次传输是否一致；导出 CRC32｜完整导出字节是否一致；成功前缀｜只包含 written 条，不扫描残留

#### Slide 23 - 离线分析：CSV 与运行参数历史互补

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: contrast: CSV数据与事件参数历史；link: 结合用于分析
- **Title**: 离线分析：CSV 与运行参数历史互补
- **Core message**: 57 列便于分析；参数 revision 历史要另存事件
- **Content**: 串口 d｜元信息 / 标定 → 57 列 → 状态 → EOF；q0…q15｜按会话标定重新归一化；Flash｜不保存动态 revision / 调参历史；电脑事件 JSONL｜补充运行参数变化

#### Slide 24 - 评审落点：机制已落地，实测证据仍需补齐

- **Audience move**: 从知道模块到理解本页机制与边界
- **Relationships**: contrast: 软件记录、硬件待验与待决定事项
- **Title**: 评审落点：机制已落地，实测证据仍需补齐
- **Core message**: 先确认不变量，再用板上证据关闭性能与联调问题
- **Content**: 已记录｜STM32 软件实现、构建与历史软件验证；待实测｜Flash ≥96 kB/s 负载 + 擦写开销；60 s 上传；待确认｜SPI 波形、700 / 900 μs、栈、AC5、实车；待设计｜D08 对端临界序号回绕责任；桥接交付

## X. Speaker Notes Requirements

- **Generation**: enabled
- **Filename**: match SVG basenames
- **Content**: 每页解释调用关系、关键边界与源码定位；不虚构硬件结果。
- **Total duration**: 25至30分钟
- **Notes style**: 正式技术评审讲解
- **Presentation purpose**: 深入讲解并审查日志和上传机制
