# Phase → Docs 功能追溯

2026-11-05；第一段映射唯一维护于此。Phase完整保留其基线设计映射，当前文件/关键符号与owner在所属功能页维护；[模块反查](architecture.md)，[历史任务书M映射](../specs/traceability.md)。

## 功能注册

| ID | 功能 / 唯一主条目 | 当前状态 |
|---|---|---|
| F01 | [F01 启动、配置与固定池](system/startup-and-resources.md#f01) | 已实现，验证范围见主条目 |
| F02 | [F02 显式帧编解码与有界JSON](protocol/framing-and-links.md#f02) | 已实现，验证范围见主条目 |
| F03 | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03) | 已实现，验证范围见主条目 |
| F04 | [F04 SPI事务与协议适配](transport/spi.md#f04) | 已实现，验证范围见主条目 |
| F05 | [F05 网络与单控制会话](transport/network.md#f05) | 已实现，验证范围见主条目 |
| F06 | [F06 可靠接管与请求信用](bridge/forwarding-and-lifecycle.md#f06) | 2026-11-07生产已接入，最终代码验收待完成 |
| F07 | [F07 遥测与源事件](bridge/forwarding-and-lifecycle.md#f07) | 2026-11-07生产已接入，最终代码验收待完成 |
| F08 | [F08 生命周期与单块导出](bridge/forwarding-and-lifecycle.md#f08) | 2026-11-07生产已接入，最终代码验收待完成 |
| F09 | [F09 本地诊断与统计](system/diagnostics.md#f09) | 统一桥接统计已接入，验收待完成 |
| F10 | [F10 工具、验证与交付](guides/validation.md#f10) | 既有工具存在，最终交付未完成 |

## Phase 01

| 需求/约束 | 主条目 | 协作 | 状态/限制 |
|---|---|---|---|
| [P01 FR-1](../specs/features/01-project-foundation/requirements.md#fr-01) | [F10 工具、验证与交付](guides/validation.md#f10) |  | 软件基础已存在；阶段实机项未全部验收 |
| [P01 FR-2](../specs/features/01-project-foundation/requirements.md#fr-02) | [F01 启动、配置与固定池](system/startup-and-resources.md#f01) |  | 软件基础已存在；阶段实机项未全部验收 |
| [P01 FR-3](../specs/features/01-project-foundation/requirements.md#fr-03) | [F01 启动、配置与固定池](system/startup-and-resources.md#f01) | [F05 网络与单控制会话](transport/network.md#f05) | 软件基础已存在；阶段实机项未全部验收 |
| [P01 FR-4](../specs/features/01-project-foundation/requirements.md#fr-04) | [F01 启动、配置与固定池](system/startup-and-resources.md#f01) |  | 软件基础已存在；阶段实机项未全部验收 |
| [P01 FR-5](../specs/features/01-project-foundation/requirements.md#fr-05) | [F10 工具、验证与交付](guides/validation.md#f10) |  | 软件基础已存在；阶段实机项未全部验收 |
| [P01 FR-6](../specs/features/01-project-foundation/requirements.md#fr-06) | [F10 工具、验证与交付](guides/validation.md#f10) |  | 软件基础已存在；阶段实机项未全部验收 |
| [P01 TC-1](../specs/features/01-project-foundation/requirements.md#tc-01) | [F10 工具、验证与交付](guides/validation.md#f10) |  | 软件基础已存在；阶段实机项未全部验收 |
| [P01 TC-2](../specs/features/01-project-foundation/requirements.md#tc-02) | [F01 启动、配置与固定池](system/startup-and-resources.md#f01) | [F04 SPI事务与协议适配](transport/spi.md#f04)；[F07 遥测与源事件](bridge/forwarding-and-lifecycle.md#f07) | 软件基础已存在；阶段实机项未全部验收 |
| [P01 TC-3](../specs/features/01-project-foundation/requirements.md#tc-03) | [F10 工具、验证与交付](guides/validation.md#f10) |  | 软件基础已存在；阶段实机项未全部验收 |

## Phase 02

| 需求/约束 | 主条目 | 协作 | 状态/限制 |
|---|---|---|---|
| [P02 FR-1](../specs/features/02-scb-protocol/requirements.md#fr-01) | [F02 显式帧编解码与有界JSON](protocol/framing-and-links.md#f02) |  | 本端协议已实现；候选向量三端未核对 |
| [P02 FR-2](../specs/features/02-scb-protocol/requirements.md#fr-02) | [F02 显式帧编解码与有界JSON](protocol/framing-and-links.md#f02) |  | 本端协议已实现；候选向量三端未核对 |
| [P02 FR-3](../specs/features/02-scb-protocol/requirements.md#fr-03) | [F02 显式帧编解码与有界JSON](protocol/framing-and-links.md#f02) |  | 本端协议已实现；候选向量三端未核对 |
| [P02 FR-4](../specs/features/02-scb-protocol/requirements.md#fr-04) | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03) |  | 本端协议已实现；候选向量三端未核对 |
| [P02 FR-5](../specs/features/02-scb-protocol/requirements.md#fr-05) | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03) |  | 本端协议已实现；候选向量三端未核对 |
| [P02 FR-6](../specs/features/02-scb-protocol/requirements.md#fr-06) | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03) | [F05 网络与单控制会话](transport/network.md#f05) | 本端协议已实现；候选向量三端未核对 |
| [P02 FR-7](../specs/features/02-scb-protocol/requirements.md#fr-07) | [F10 工具、验证与交付](guides/validation.md#f10) | [F02 显式帧编解码与有界JSON](protocol/framing-and-links.md#f02) | 本端协议已实现；候选向量三端未核对 |
| [P02 TC-1](../specs/features/02-scb-protocol/requirements.md#tc-01) | [F02 显式帧编解码与有界JSON](protocol/framing-and-links.md#f02) |  | 本端协议已实现；候选向量三端未核对 |
| [P02 TC-2](../specs/features/02-scb-protocol/requirements.md#tc-02) | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03) | [F04 SPI事务与协议适配](transport/spi.md#f04)；[F05 网络与单控制会话](transport/network.md#f05) | 本端协议已实现；候选向量三端未核对 |
| [P02 TC-3](../specs/features/02-scb-protocol/requirements.md#tc-03) | [F06 可靠接管与请求信用](bridge/forwarding-and-lifecycle.md#f06) | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03) | 本端协议已实现；候选向量三端未核对 |

## Phase 03

| 需求/约束 | 主条目 | 协作 | 状态/限制 |
|---|---|---|---|
| [P03 FR-1](../specs/features/03-spi-slave/requirements.md#fr-01) | [F04 SPI事务与协议适配](transport/spi.md#f04) |  | SPI事务/协议已接入；电气和吞吐待验 |
| [P03 FR-2](../specs/features/03-spi-slave/requirements.md#fr-02) | [F04 SPI事务与协议适配](transport/spi.md#f04) |  | SPI事务/协议已接入；电气和吞吐待验 |
| [P03 FR-3](../specs/features/03-spi-slave/requirements.md#fr-03) | [F04 SPI事务与协议适配](transport/spi.md#f04) |  | SPI事务/协议已接入；电气和吞吐待验 |
| [P03 FR-4](../specs/features/03-spi-slave/requirements.md#fr-04) | [F04 SPI事务与协议适配](transport/spi.md#f04) |  | SPI事务/协议已接入；电气和吞吐待验 |
| [P03 FR-5](../specs/features/03-spi-slave/requirements.md#fr-05) | [F04 SPI事务与协议适配](transport/spi.md#f04) | [F06 可靠接管与请求信用](bridge/forwarding-and-lifecycle.md#f06) | SPI事务/协议已接入；电气和吞吐待验 |
| [P03 FR-6](../specs/features/03-spi-slave/requirements.md#fr-06) | [F07 遥测与源事件](bridge/forwarding-and-lifecycle.md#f07) | [F04 SPI事务与协议适配](transport/spi.md#f04)；[F09 本地诊断与统计](system/diagnostics.md#f09) | SPI事务/协议已接入；电气和吞吐待验 |
| [P03 TC-1](../specs/features/03-spi-slave/requirements.md#tc-01) | [F04 SPI事务与协议适配](transport/spi.md#f04) | [F01 启动、配置与固定池](system/startup-and-resources.md#f01) | SPI事务/协议已接入；电气和吞吐待验 |
| [P03 TC-2](../specs/features/03-spi-slave/requirements.md#tc-02) | [F04 SPI事务与协议适配](transport/spi.md#f04) |  | SPI事务/协议已接入；电气和吞吐待验 |
| [P03 TC-3](../specs/features/03-spi-slave/requirements.md#tc-03) | [F04 SPI事务与协议适配](transport/spi.md#f04) |  | SPI事务/协议已接入；电气和吞吐待验 |

## Phase 04

| 需求/约束 | 主条目 | 协作 | 状态/限制 |
|---|---|---|---|
| [P04 FR-1](../specs/features/04-network-services/requirements.md#fr-01) | [F05 网络与单控制会话](transport/network.md#f05) |  | 网络已接入；修复后实机流程待复测 |
| [P04 FR-2](../specs/features/04-network-services/requirements.md#fr-02) | [F05 网络与单控制会话](transport/network.md#f05) |  | 网络已接入；修复后实机流程待复测 |
| [P04 FR-3](../specs/features/04-network-services/requirements.md#fr-03) | [F05 网络与单控制会话](transport/network.md#f05) |  | 网络已接入；修复后实机流程待复测 |
| [P04 FR-4](../specs/features/04-network-services/requirements.md#fr-04) | [F05 网络与单控制会话](transport/network.md#f05) |  | 网络已接入；修复后实机流程待复测 |
| [P04 FR-5](../specs/features/04-network-services/requirements.md#fr-05) | [F05 网络与单控制会话](transport/network.md#f05) |  | 网络已接入；修复后实机流程待复测 |
| [P04 FR-6](../specs/features/04-network-services/requirements.md#fr-06) | [F05 网络与单控制会话](transport/network.md#f05) |  | 网络已接入；修复后实机流程待复测 |
| [P04 FR-7](../specs/features/04-network-services/requirements.md#fr-07) | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03) | [F05 网络与单控制会话](transport/network.md#f05)；[F08 生命周期与单块导出](bridge/forwarding-and-lifecycle.md#f08) | 网络已接入；修复后实机流程待复测 |
| [P04 TC-1](../specs/features/04-network-services/requirements.md#tc-01) | [F05 网络与单控制会话](transport/network.md#f05) | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03) | 网络已接入；修复后实机流程待复测 |
| [P04 TC-2](../specs/features/04-network-services/requirements.md#tc-02) | [F05 网络与单控制会话](transport/network.md#f05) | [F09 本地诊断与统计](system/diagnostics.md#f09) | 网络已接入；修复后实机流程待复测 |

## Phase 05

| 需求/约束 | 主条目 | 协作 | 状态/限制 |
|---|---|---|---|
| [P05 FR-1](../specs/features/05-bridge-lifecycle/requirements.md#fr-01) | [F06 可靠接管与请求信用](bridge/forwarding-and-lifecycle.md#f06) |  | 2026-11-07生产已接入 / 验收待完成 |
| [P05 FR-2](../specs/features/05-bridge-lifecycle/requirements.md#fr-02) | [F06 可靠接管与请求信用](bridge/forwarding-and-lifecycle.md#f06) |  | 2026-11-07生产已接入 / 验收待完成 |
| [P05 FR-3](../specs/features/05-bridge-lifecycle/requirements.md#fr-03) | [F07 遥测与源事件](bridge/forwarding-and-lifecycle.md#f07) |  | 2026-11-07生产已接入 / 验收待完成 |
| [P05 FR-4](../specs/features/05-bridge-lifecycle/requirements.md#fr-04) | [F07 遥测与源事件](bridge/forwarding-and-lifecycle.md#f07) |  | 2026-11-07生产已接入 / 验收待完成 |
| [P05 FR-5](../specs/features/05-bridge-lifecycle/requirements.md#fr-05) | [F08 生命周期与单块导出](bridge/forwarding-and-lifecycle.md#f08) |  | 2026-11-07生产已接入 / 验收待完成 |
| [P05 FR-6](../specs/features/05-bridge-lifecycle/requirements.md#fr-06) | [F08 生命周期与单块导出](bridge/forwarding-and-lifecycle.md#f08) |  | 2026-11-07生产已接入 / 验收待完成 |
| [P05 FR-7](../specs/features/05-bridge-lifecycle/requirements.md#fr-07) | [F08 生命周期与单块导出](bridge/forwarding-and-lifecycle.md#f08) |  | 2026-11-07生产已接入 / 验收待完成 |
| [P05 FR-8](../specs/features/05-bridge-lifecycle/requirements.md#fr-08) | [F09 本地诊断与统计](system/diagnostics.md#f09) |  | 2026-11-07生产已接入 / 验收待完成 |
| [P05 FR-9](../specs/features/05-bridge-lifecycle/requirements.md#fr-09) | [F08 生命周期与单块导出](bridge/forwarding-and-lifecycle.md#f08) |  | 2026-11-07生产已接入 / 验收待完成 |
| [P05 TC-1](../specs/features/05-bridge-lifecycle/requirements.md#tc-01) | [F06 可靠接管与请求信用](bridge/forwarding-and-lifecycle.md#f06) | [F01 启动、配置与固定池](system/startup-and-resources.md#f01) | 2026-11-07生产已接入 / 验收待完成 |
| [P05 TC-2](../specs/features/05-bridge-lifecycle/requirements.md#tc-02) | [F06 可靠接管与请求信用](bridge/forwarding-and-lifecycle.md#f06) | [F08 生命周期与单块导出](bridge/forwarding-and-lifecycle.md#f08) | 2026-11-07生产已接入 / 验收待完成 |
| [P05 TC-3](../specs/features/05-bridge-lifecycle/requirements.md#tc-03) | [F08 生命周期与单块导出](bridge/forwarding-and-lifecycle.md#f08) | [F03 独立可靠窗口与会话](protocol/framing-and-links.md#f03)；[F04 SPI事务与协议适配](transport/spi.md#f04) | 2026-11-07生产已接入 / 验收待完成 |

## Phase 06

| 需求/约束 | 主条目 | 协作 | 状态/限制 |
|---|---|---|---|
| [P06 FR-1](../specs/features/06-validation-delivery/requirements.md#fr-01) | [F10 工具、验证与交付](guides/validation.md#f10) |  | 最终负载/三端实机/完整交付未完成 |
| [P06 FR-2](../specs/features/06-validation-delivery/requirements.md#fr-02) | [F10 工具、验证与交付](guides/validation.md#f10) | [F01 启动、配置与固定池](system/startup-and-resources.md#f01) | 最终负载/三端实机/完整交付未完成 |
| [P06 FR-3](../specs/features/06-validation-delivery/requirements.md#fr-03) | [F04 SPI事务与协议适配](transport/spi.md#f04) | [F10 工具、验证与交付](guides/validation.md#f10) | 最终负载/三端实机/完整交付未完成 |
| [P06 FR-4](../specs/features/06-validation-delivery/requirements.md#fr-04) | [F07 遥测与源事件](bridge/forwarding-and-lifecycle.md#f07) | [F05 网络与单控制会话](transport/network.md#f05)；[F10 工具、验证与交付](guides/validation.md#f10) | 最终负载/三端实机/完整交付未完成 |
| [P06 FR-5](../specs/features/06-validation-delivery/requirements.md#fr-05) | [F08 生命周期与单块导出](bridge/forwarding-and-lifecycle.md#f08) | [F05 网络与单控制会话](transport/network.md#f05)；[F06 可靠接管与请求信用](bridge/forwarding-and-lifecycle.md#f06) | 最终负载/三端实机/完整交付未完成 |
| [P06 FR-6](../specs/features/06-validation-delivery/requirements.md#fr-06) | [F10 工具、验证与交付](guides/validation.md#f10) |  | 最终负载/三端实机/完整交付未完成 |
| [P06 FR-7](../specs/features/06-validation-delivery/requirements.md#fr-07) | [F10 工具、验证与交付](guides/validation.md#f10) |  | 最终负载/三端实机/完整交付未完成 |
| [P06 TC-1](../specs/features/06-validation-delivery/requirements.md#tc-01) | [F10 工具、验证与交付](guides/validation.md#f10) |  | 最终负载/三端实机/完整交付未完成 |
| [P06 TC-2](../specs/features/06-validation-delivery/requirements.md#tc-02) | [F10 工具、验证与交付](guides/validation.md#f10) |  | 历史首次建档范围，不是长期实现禁令 |

## 补充增量（不并入58条原FR/TC计数）

| 来源 | 主条目 | 状态 |
|---|---|---|
| [P05 R6-01](../specs/features/05-bridge-lifecycle/r6-log-counters.md#r6-01) | [F09 本地诊断与统计](system/diagnostics.md#f09) | 原2026-10-22增量；桥接模块2026-11-07已新增；原历史结果不重验 |
| [P05 R6-02](../specs/features/05-bridge-lifecycle/r6-log-counters.md#r6-02) | [F09 本地诊断与统计](system/diagnostics.md#f09) | 原2026-10-22增量；桥接模块2026-11-07已新增；原历史结果不重验 |
| [P05 R6-03](../specs/features/05-bridge-lifecycle/r6-log-counters.md#r6-03) | [F09 本地诊断与统计](system/diagnostics.md#f09) | 原2026-10-22增量；桥接模块2026-11-07已新增；原历史结果不重验 |

## 本轮存量整改追溯

2026-11-05：[Phase01～04格式与注释整改契约](../specs/research/2026-11-05-source-style-remediation-draft.md)；47文件静态整改与人工复核已完成，目标构建未验证，见[本轮报告](validation/2026-11-05-source-style-review.md)；现行功能、状态owner和模块映射不变。阶段验收及硬件待验不因本次整改自动完成。

## 2026-11-07 子主题主归属与反查

F01：[启动/配置](system/startup-and-resources.md#f01-startup)、[池/DMA](system/resource-pools.md#f01-pools)。F02：[帧/检查矩阵](protocol/frame-codec.md#f02-validation)、[JSON/u32](protocol/json-validation.md#f02-json)。F03：[窗口/重试/会话](protocol/reliable-and-sessions.md#f03)。F04/F05分别由SPI与网络子锚点说明。F09公开快照字段以[诊断](system/diagnostics.md#f09-spi)为主说明，传输页只解释事件机制。所有参数经[参数索引](parameters.md)反查。原FR/TC及F01～F10身份不变；Phase完整保留阶段设计映射，本页提供当前导航。
