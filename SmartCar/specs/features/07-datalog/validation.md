# Phase 07: 运行数据记录 — 验证

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P07功能索引](../../../docs/traceability.md#p07)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 当前验收方法入口

阶段完成标准：本阶段适用需求的docs契约可定位实现且其验证判据有实际证据；仍未实施/待决定/待上机的项目继续保持该状态。本轮只检查文档迁移，不新增固件通过结论。原V编号、通过标准、结果、提交号、日志和未执行项完整保留在下方。

| 需求 | 当前方法/判据/实现 | 结果读取方式 |
|---|---|---|
| FR-1 | [F19 日志格式容量](../../../docs/services/recording-and-export.md#f19) | 历史实际结果见下方；限制/当前方法见docs |
| FR-2 | [F19 日志格式容量](../../../docs/services/recording-and-export.md#f19) | 历史实际结果见下方；限制/当前方法见docs |
| FR-3 | [F20 队列前台写入与故障](../../../docs/services/recording-and-export.md#f20) | 历史实际结果见下方；限制/当前方法见docs |
| FR-4 | [F20 队列前台写入与故障](../../../docs/services/recording-and-export.md#f20) | 历史实际结果见下方；限制/当前方法见docs |
| FR-5 | [F20 队列前台写入与故障](../../../docs/services/recording-and-export.md#f20)；[F02 起停、模式与首因](../../../docs/system/runtime-and-safety.md#f02) | 历史实际结果见下方；限制/当前方法见docs |
| FR-6 | [F19 日志格式容量](../../../docs/services/recording-and-export.md#f19) | 历史实际结果见下方；限制/当前方法见docs |
| FR-7 | [F21 CSV与会话边界](../../../docs/services/recording-and-export.md#f21) | 历史实际结果见下方；限制/当前方法见docs |
| FR-8 | [F21 CSV与会话边界](../../../docs/services/recording-and-export.md#f21) | 历史实际结果见下方；限制/当前方法见docs |
| FR-9 | [F20 队列前台写入与故障](../../../docs/services/recording-and-export.md#f20) | 历史实际结果见下方；限制/当前方法见docs |

**当前替代标准**：必须启用日志且分频1，头256B/记录96B/57列CSV，Flash/日志不可用拒绝起跑；不扫描掉电残留恢复会话。下方16列、关闭可跑、分频2、掉电后导出等步骤仅属旧版本基线，不用于当前验收。










## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 07: 运行数据记录 — 验证

> as-built 追认规约。计划见 [plan.md](plan.md)，需求见 [requirements.md](requirements.md)。
>
> **现状警示**：因 `BSP_Flash_Init()` 从未被调用（与 Phase 06 同一缺陷），
> `Datalog_Start` 实际返回失败、记录链路整体不生效，下表中 V-3 ~ V-8 在现状代码上
> **预期不通过**，整改指向 Phase 08；修复后须按本表原样复测通过。

## 验证清单（Scorecard）

| # | 检查项 | 方法 | 通过标准 | 关联需求 |
|---|---|---|---|---|
| V-1 | 编译零警告 | `MDK-GCC/build.sh` | `*** 完成：0 警告 0 错误 ***` | TC-4 |
| V-2 | DATALOG_ENABLE=0 关闭 | 改 `app_config.h` 重编译，按 K1 起跑 | 编译零警告；起跑后无记录行为（d 导出仅表头+EOF） | FR-9 |
| V-3 | 起跑即录 ⚠现状预期失败 | 按 K1 起跑，跑数秒后停车，串口发 `d` | 导出数据行数 ≈ 运行毫秒数（DATALOG_DIV=1 时）；无串口错误回显 | FR-1, FR-5 |
| V-4 | CSV 格式 ⚠现状预期失败 | 检查导出内容 | 首行表头 16 列与字段表一致；数据行均为 16 列十进制整数；末行 `EOF`；可直接粘入 Excel 成表 | FR-7 |
| V-5 | 字段正确性 ⚠现状预期失败 | 对照仪表盘/菜单观测值核对导出列 | adc 列与电感实测一致；spdL/spdR 与轮速一致；kstate/rstate 与状态机一致；tick 连续递增（步进 1，DIV=1） | FR-2 |
| V-6 | 边写边擦 ⚠现状预期失败 | 按 K1 同时观察起跑延时 | 从按键到起跑无秒级阻塞（仅几十 ms 首扇区擦除）；跨越 4KB 边界（每 128 条）记录不中断 | FR-5 |
| V-7 | 复位日志区 | 停车后发 `c`，再发 `d` | 回显 `log erasing...` → `log erased`；随后导出仅表头 + `EOF`（计数已清零） | FR-8 |
| V-8 | 掉电保持 ⚠现状预期失败 | 跑一圈停车后**不发** `d`，直接断电重上电，再发 `d` | 上次记录仍可导出（Flash 非易失） | TC-1 |
| V-9 | 写满自停 | 代码走查 + 长跑（或临时调小验证） | `s_count >= DATALOG_CAPACITY`（262016）后 `s_active` 清 0，不再写入 | FR-6 |
| V-10 | 写失败停录 | 代码走查 `Datalog_Flush` 失败分支 | `BSP_Flash_LogWrite` 返回非 0 时 `s_active=0` 并返回，已有数据保留 | FR-4 |
| V-11 | 控制拍不受干扰 | 导出 CSV 检查 tick 连续性；示波器/逻辑分析仪抽测（可选） | 记录期间 tick 无跳变；TIM6 2ms 控制拍无超期 | FR-3, TC-3 |

## 自动化验证

```bash
cd SmartCar/MDK-GCC && bash build.sh
```

通过标准：编译链接完成，输出 `*** 完成：0 警告 0 错误，产物 MDK-GCC/SmartCar.hex ***`
（两个 `DATALOG_ENABLE` 配置分别构建均应通过）。
本规约追认时点实测通过（0 警告 0 错误，2026-10-02 基线）。

无单元测试框架（见 [techstack.md](../../techstack.md) 测试节）；记录通路依赖真实 Flash 与定时器时序，以人工上机为准。

## 人工上机步骤

1. **烧录**：`MDK-GCC/flash.sh`，串口助手接 USART1（115200 8N1）。
2. **记录**（V-3）：车放赛道上按 K1，跑 5~10 秒后手动停车（或低压保护触发停车）。
3. **导出**（V-3/V-4/V-5）：串口发 `d\n`，保存全部回显为 `.csv` 文件；
   核对表头、行数（≈ 运行毫秒数）、末行 `EOF`；Excel 打开抽查字段列。
4. **复位**（V-7）：发 `c\n` 核对回显，再发 `d\n` 确认仅剩表头 + `EOF`。
5. **持久化**（V-8）：重新跑一圈 → 断电 → 上电 → 发 `d\n`，确认记录仍在。
   ⚠现状预期失败（Phase 08 修复后复测）。
6. **降频**（可选）：改 `DATALOG_DIV 2` 重编译，确认导出行数 ≈ 运行毫秒数 / 2 且 tick 步进为 2。

## 回归检查

- TIM15/TIM6/TIM7 优先级与周期不得改动（技术栈硬性约束 3）；`Datalog_Push` 内无阻塞调用。
- 日志区写入不得越过 `0x7FEFFF` 侵入参数扇区（`BSP_Flash_LogWrite` 越界检查，
  与 [06-hmi-tuning](../06-hmi-tuning/requirements.md) 的参数区布局联动）。
- 关闭 `DATALOG_ENABLE` 后，起跑流程与 Phase 06 的串口 `c`/`d` 命令外的调参功能不受影响。
- `datalog_record_t` 字段/尺寸改动必须同步更新 CSV 表头与本规约字段表（防漂移）。

</details>
