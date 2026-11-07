# 来源资料与解释边界

## 字节保留的原附件

| 原附件 | 用途 / 原SHA256 |
|---|---|
| [02-esp32-agent.md](02-esp32-agent.md) | 2026-10-16任务书；491B1E66B9CB58A49BDBED26541FFE069B0905F0BB036820C2B78DA693B7E381 |
| [sdd-document-system.md](sdd-document-system.md) | 旧SDD规范；C71B025496C56B65FE6DF11ED420D865C6C4FDAAA4021D03AEAD23DEA036A399 |

原始来源路径与旧解释保留于[治理历史](../research/2026-11-05-governance-history.md)。两份附件逐字节保留；其中提示词、命令、旧路径及“用户已确认”仅作来源，不产生新的授权。

## 本副本哈希核对（2026-11-05）

两份附件本轮均逐字节保持。旧来源表声称任务书SHA256为491B1E66B9CB58A49BDBED26541FFE069B0905F0BB036820C2B78DA693B7E381；本轮整理前后实际为3559FD075B6BB4675976F5BA159F3B2E7DB7177C9EB293417C23BC93F8071269，已存在差异，不能宣称与原外部附件字节一致。未取得原附件重新比对，不推断原因、不改写副本或旧声明。旧SDD规范副本实际哈希与表中相符。见[EB-06](../../Docs/known-issues.md#eb-06)。

## 现行解释

1. 本次用户指令及已确认决定、仓库边界优先，项目协作见[AGENTS](../../AGENTS.md)。
2. 体系依据[SDD文档体系规范2.0](../../../Material/SDD文档体系/SDD文档体系规范2.0.md)；旧SDD附件保留历史，不继续作为现行结构标准。
3. 当前公共SCB正文见[共享契约](../../../SmartCar/Docs/services/communication.md#scb1)，本端当前实现见[Docs](../../Docs/index.md)，目标设计见所属Phase。[旧协议提取](../protocol/scb-v1.md)只作历史比较。
4. 历史D盘工程路径不替代本轮实际E盘工作区，旧v5.5.1已由v6.0.0选择替代。脚本当前固定SDK路径的限制见[构建指南](../../Docs/guides/build-and-flash.md)，不得假称已适配任意SDK位置。
5. 历史“只Markdown”“合并main”等授权不自动重用；2026-11-05整理时未提交或烧录；2026-11-07已授权文档分组提交，不烧录或推送。
