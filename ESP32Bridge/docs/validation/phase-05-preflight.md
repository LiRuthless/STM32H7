# Phase 05 开工核对（2026-10-20）

## 结论与证据范围

开工HEAD为`09feab00ae5bfbd0d3649004bfaf918d880c0dc8`，分支`codex/04-network-services`。
`git merge-base --is-ancestor 00331c9 HEAD`退出0；该修复当前仍在`main/net_bridge.c link_service`。
`git log -- ESP32Bridge`中00331c9之后无本目录实现提交，只有全仓合并。
全仓已有未跟踪`.vscode/settings.json`，不修改、不暂存。

本轮通过原入口重新执行221项主机回归，0失败、0跳过（`.local/host-tests.log`，12.631秒）。
先在沙箱遇到既有Xtensa启动OS error=623，再获自动审查允许，在沙箱外原样运行同一入口通过；未安装、替换或修改SDK。
真实网络、SPI、电气及STM32互斥释放均未因此验收。

## 已有构建产物复核

以下目录均位于`D:\STM32H7\ESP32Bridge\artifacts\`，本轮逐项复算manifest内所有产物SHA256，全部相符。
三个修复后构建的30个源码输入均与开工代码相符；BIN不同不能自行认定功能差异（构建时间等也会影响镜像）。

| run_id | 构建开始（北京时间） | 状态 | 应用BIN SHA256 |
|---|---|---|---|
| 39dbd159ff3041998c91a2c78a2df979 | 10-20 17:51:53 | build/size均0，success | 04836264639bc3770f536ea8d40ba59cf1d7b79d2f598fdd87059c59a81ccddf |
| 738d7bba757b449bba2492451835789b | 10-20 20:24:16 | build/size均0，success | b04e3aa62fa4284fc790aa97ad3215f03e46998146c801693ccc83534a4e23f1 |
| e861be93b9bc4471a8122e4a5fbb14ae | 10-20 20:40:14 | build/size均0，success | a26968fb519bf832c8eb0436332afc8f92ae95c2e10daaafc7deae9a3ca81dcd |

完整本地审计为`.local/phase05-baseline-audit.json`（审计脚本`.local/audit-phase05-baseline.py`），不包含密码。
最近产物manifest SHA256：`c23ae52bba73884a89df9b12a3d84cb9e6ae98dbe5cfd28fab766d7cdb17fa92`。
既有实机已烧录的`bf24b81807e842f8b66fded4d9186e17`与当前`main/net_bridge.c`不符，是修复前版本。
不能用其启动/DHCP成功证明00331c9已上机。未找到修复后串口、抓包或BUSY复测记录。

## 人工烧录与复测步骤（本轮未执行）

由用户选择时机执行；本轮代理不访问串口、不烧录、不复位。以下固定使用已核验的修复后产物，不能拿旧目录代替。

1. 关闭占用COM3的串口监视器；确认本次设备仍为ESP32-S3、串口仍为COM3。历史COM3仅是已有记录，不代表当前枚举已核验。
2. 在干净PowerShell激活既有环境并进入完整回收目录：

```powershell
Set-Location -LiteralPath 'D:\STM32H7\ESP32Bridge'
. .\scripts\activate.ps1
Set-Location -LiteralPath '.\artifacts\e861be93b9bc4471a8122e4a5fbb14ae'
Get-FileHash -Algorithm SHA256 -LiteralPath '.\esp32bridge.bin'
# 核对上表哈希后，由用户主动执行；该命令会烧录并触发设备复位。
python -m esptool --chip esp32s3 --port COM3 --baud 460800 write-flash '@flash_args'
```

`flash_args`原文使用dio/80m/2MB，偏移为0x0 bootloader、0x8000 partition table、0x10000 app；三者必须来自同一目录。
这是镜像元数据，不是对实物Flash总容量的新判定。esptool通过参数文件读取，无需抄写偏移。

3. 保存串口启动记录、固件镜像SHA256、串口与设备信息；连接SmartCar热点，确认电脑获192.168.4.x/24。
4. 在SPI尚无合法会话条件下，TCP连接192.168.4.1:5000，发SCB/1 HELLO：session/seq/ack/request均0，JSON为`{"role":"pc","protocol":1}`。
   核对HELLO_ACK的CRC、session_id=0、link_ready=false，保持第一连接至少2秒。超时无数据不等于关闭；必须区分recv超时与EOF。
5. 第一连接保持期间建立第二连接，不必先发HELLO；应立即收到HELLO_ACK，status=BUSY、同样session/ready，随后第二连接EOF，第一连接仍存活。
6. 主动关闭第一连接，重连并发HELLO，应正常取得HELLO_ACK；保持至少2秒后再验证第二连接BUSY。
7. 在无合法会话的第一连接发可靠SUBSCRIBE（request_id非零、seq=1、session=0），必须关闭该连接、不返回成功RESULT、不注册订阅；再重连HELLO仍可工作。
8. 分别保存原始收发字节、独立CRC结果、连接时间线和EOF/超时区分。未执行前，HELLO保持/BUSY/断线重连/业务拒绝均列为实机待验。

`.local/pc_smoke.py`是旧临时脚本，其“收到意外RESULT仍继续打印通过”等路径不构成严格断言；不能仅凭脚本末行“通过”验收。
正式Phase06工具仍未交付，本节不替代最终中文手册。

## 当前对端事实（只读）

当前SmartCar已经存在`App/scb_protocol.c`、`scb_link.c`、`tuning.c`和`datalog.c`导出接口，
`App_Init`/前台已调用SCB_Init/SCB_Service；旧报告“未发现SCB入口”只适用于当时基线。
本轮只读查看Phase16规约及实际代码，不改SmartCar，也不重复运行其阶段验收。

- `SCB_ReceiveFrame`接受HELLO_ACK的非零session后发IDLE确认，与ESP握手方案无已发现字段矛盾。
- `Datalog_ReadExport`只对新成功offset推进CRC/租期，重复相同offset/length返回缓存不续期；`Datalog_IsExporting`使用10000ms超时。
- `Tuning_LogCommand`成功LOG_READ生成SCB_LOG_DATA，LOG_BEGIN/END返回STM32自身元数据/CRC。
- STM32 EVENT_GAP额外携带dropped_events；ESP应原样转发源EVENT，不能删改该字段。ESP自生G-03报告仍只用已确认首源锚点，不增字段。

这些是静态兼容事实，三端固定向量、真实SPI、吞吐、断线后STM32实际超时释放仍待验；不据此更改公共契约。

## 后续边界

本轮用户明确恢复Phase05，Phase内不逐组等待，Phase末交审后才允许Phase06。
新增固定资源表R-05已形成单独草案并请求确认；确认前不实施依赖该决定的桥接状态。

## 连续流程回归回填

纯规约`bd1ee7b`之后新增`test_zero_session_hold_busy_disconnect_reconnect_and_business_refusal`：
生产网络循环在零会话HELLO后保持201轮（至少2秒），第二客户端获得精确BUSY且仅自身关闭；
随后主动断开、HELLO重连、无会话SUBSCRIBE拒绝、再次HELLO保持，完整流程检查精确帧字节和关闭FD顺序。
完整回归**222项通过、0跳过，13.164秒**；原`.local/host-tests.log`已被最新运行更新，
另保留不可混淆路径`.local/phase05-preflight-regression.log`，哈希见[证据索引](phase-05-preflight-evidence.json)。
本次仅新增主机测试，未改生产C；因此已有目标构建30项源码输入仍一致，不为测试变更重复构建镜像。

在`.local/zero-session-negative-*`独立副本中只撤回00331c9的非零会话保护：
原HELLO保持测试精确失败于`control_open == false`，新增连续流程因错误提前关闭导致脚本FD所有权断言失败；
共2项预期失败、0错误，负对照检查脚本退出0。实际生产C未被改写。
日志`.local/phase05-preflight-negative.log`及元数据见证据索引，不能把负对照“预期失败”算作回归失败或实机证据。

提交前另发现仓库外部并发新增未跟踪`SmartCar/MDK-ARM/flash-preflight-build.log`；
该文件不属于本次ESP32Bridge改动，未读取内容、修改、删除或暂存。连同原`.vscode/settings.json`保留。
