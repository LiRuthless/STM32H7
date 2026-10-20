# SmartCarLink — 智能车 SCB/1 电脑端工具

电脑端接收、记录、运行调参和停车日志导出工具。协议为 SCB/1 v1
（三端共同协议，见任务书 `03-python-agent.md`）。
仅使用 Python 3.10+ 标准库，**无需安装任何第三方包**。

## 运行环境

- Python 3.10 或更高版本（Windows / Linux 均可，开发验证于 Windows + Python 3.11）。
- 不开发 GUI，全部为命令行。

## 连接设备

1. 给智能车上电，ESP32-S3 开启 WPA2 热点，SSID 形如 `SmartCar-XXXX`
   （XXXX 为 MAC 最后两字节的大写十六进制），密码见本地配置。
2. 电脑连接该热点。设备地址固定 `192.168.4.1`：TCP 5000（命令/日志），
   电脑 UDP 5001（实时遥测，用 SUBSCRIBE 注册）。

### Windows UDP 防火墙放行

Windows 首次运行可能弹出防火墙提示，请勾选"专用网络"允许。
若收不到遥测数据，手动放行（管理员 PowerShell）：

```powershell
New-NetFirewallRule -DisplayName "SmartCarLink UDP 5001" `
  -Direction Inbound -Protocol UDP -LocalPort 5001 -Action Allow
```

或在"控制面板 → Windows Defender 防火墙 → 允许应用通过防火墙"中允许 `python.exe`。

## 快速开始

```bash
cd Tools/SmartCarLink

# 常驻监控 + 实时记录（Ctrl+C 有序退出）
python smartcar_link.py monitor --output run01

# 运行调参
python smartcar_link.py tune enable
python smartcar_link.py tune set --kp 2 --k2p 0.008 --kd 15 --speed 180
python smartcar_link.py tune disable

# 参数查询（默认读设备当前 mode/gear）
python smartcar_link.py params get
python smartcar_link.py params get --mode 1 --gear 0

# 停车后完整导出 Flash 日志
python smartcar_link.py dump --output run01-full

# 离线转换：完整二进制日志 → 57 列 CSV
python smartcar_link.py convert run01-full.bin --events run01.events.jsonl
```

公共选项：`--host`（默认 192.168.4.1）、`--port`（默认 5000）、
`--udp-port`（默认 5001）、`--timeout`（请求超时秒数，默认 5）。
加 `--monitor` 可把单条命令转交给本机 monitor（127.0.0.1:15000），
不直接连设备；没有 monitor 时单次命令自行连接设备，不会偷偷启动第二个设备会话。

### monitor 的本地命令接口

monitor 持有唯一设备连接，并在 **127.0.0.1:15000**（只绑 loopback）
提供逐行 JSON 接口。每行一个请求：

```json
{"op":"GET_STATUS","args":{},"id":1}
```

响应一行：

```json
{"id":1,"status":"OK","result":{...}}
```

支持转发：`GET_STATUS`、`GET_PARAMS`、`SET_PARAMS`、`TUNE_ENABLE`、
`TUNE_DISABLE`、`GET_REQUEST_RESULT`、`SUBSCRIBE`、`GET_LOG_META`；
另有本地操作 `STATUS`（monitor 状态）、`SHUTDOWN`。
端口被占用时 monitor 明确报错退出，不抢占其他进程。
终端状态每秒刷新一次，不逐条打印记录。

## 命令说明

### tune set

先 `GET_STATUS` 取当前 mode/gear，再 `GET_PARAMS` 取完整 8 项 values 和
limits，**只替换命令行指定项**后整组提交 SET_PARAMS。
校验：五项增益（kp/k2p/kd/kp_v/ki_v）为有限数；base_speed/fan_run/fan_idle
为整数；全部按设备返回的 limits 验证范围，**越界拒绝、不静默钳位**。

- `--speed` 即 base_speed，单位 **脉冲/2ms**；`--fan_run/--fan_idle` 为 **万分比**。
- 运行时只允许当前 mode/gear，且 fan_idle 必须等于当前有效值。
- `RECEIVED` 只表示设备已收到，**不等于生效**；`APPLIED`（含实际完整
  values/revision/tick/control_sequence）才称已生效；`PARAM_SAVED`/`SAVED`
  才称已持久化。每次只允许一条待最终结果的调参请求。
- 断线后**绝不自动重放** SET_PARAMS；重连先 `GET_REQUEST_RESULT` 查询，
  `NOT_FOUND` 标记为 `UNKNOWN`。

### dump（完整导出）

只在**停车且日志收尾完成**后设备才允许 LOG_BEGIN。窗口 1 顺序读，
每块 length ≤ 960，末块按剩余字节。逐块验证 request_id/offset/length/边界；
可靠重发的重复块不重写、不重复累计整体 CRC32。
流式写 `<output>.partial`，LOG_END 返回的长度与 CRC32 均一致、
256B 头校验通过后才改名为 `<output>.bin` 并生成 57 列 CSV。
失败保留 `.partial` 及 `.partial.reason.txt` 原因文件；重连后新建导出从 0
开始，不自动续传旧会话。

## 输出文件格式

实时记录（`monitor --output run01`）产生五种文件；新 run（run_id 变化或索引
归零）新建文件段 `run01.seg2.raw/.csv` 等，旧运行数据不会被按索引归并或误删。

### `<base>.raw` 原始报文

按到达顺序追加，每条：

| 字段 | 类型 | 说明 |
|---|---|---|
| recv_time_ns | u64 小端 | 本机接收时间（纳秒），不代替设备采样 tick |
| length | u32 小端 | 报文字节数 |
| frame | length 字节 | 原始 SCB/1 帧（32B 头 + 负载） |

### `<base>.csv` 57 列实时 CSV

首行列头，共 57 列，与固件 Datalog_Dump 一致：

```
tick,adc0..adc3,spdL,spdR,err,tout,gx,gy,gz,dl1b,bat,kstate,rstate,
gray0..gray15,q0..q15,em_err,opt_err,mode,opt_state,gray_valid,protect,
reason,gray_seq,control_seq
```

- 只写首次有效记录，重复不再次写；乱序记录写入但计入统计并写事件，不插值。
- `dl1b`=8192 为无效哨兵；`q0..q15` 为按日志头黑白标定的归一化值
  （复现 Gray_Normalize：向零截断、q<50→0、q>1000→1000、标定无效→0）。
  没有有效日志头时 q 留空并记录 `missing_log_header` 事件，不猜标定。

### 导出 CSV（dump/convert 生成）

在上述 57 列前有三行前导：

```
#log_version=2,record_size=96,mode=<m>,gear=<g>
#black,<16 值>
#white,<16 值>
```

`convert` 未提供 events.jsonl 时额外加一行 `#note,无 events.jsonl：参数历史
不完整…`。

### `<base>.metadata.json`

```json
{"base":"run01","start_time_ns":...,"end_time_ns":...,
 "calibration":{"black":[...],"white":[...]} 或 null,
 "segments":[{"index":1,"session_id":...,"run_id":...,"raw_file":"...",
              "csv_file":"...","records_written":...}],
 "note":"实时上传可丢；参数历史见 events.jsonl…"}
```

### `<base>.events.jsonl`

逐行 JSON 事件：设备事件（RUN_STARTED/RUN_STOPPED/TUNE_APPLIED/
TUNE_SUSPENDED/TUNE_RESUMED/PARAM_SAVED/PARAM_SAVE_FAILED/EVENT_GAP 等）、
调参结果（含 request_id、session_id、run_id、tick、control_sequence、
revision、实际完整 values、本机时间、确认状态）、本地事件
（record_gap/record_out_of_order/tick_wrap/udp_queue_overflow/
udp_bad_packet/disconnected/export_failed 等）。EVENT_GAP 与断线期间缺失的
历史明确保留，不伪造完整历史。

### `<base>.link_stats.json`

```json
{"packets_received":0,"records_received":0,"records_written":0,
 "records_duplicate":0,"records_out_of_order":0,"records_gap_missing":0,
 "gap_count":0,"run_count":0,"tick_wraps":0,"failed":null}
```

## 故障排查

| 现象 | 排查 |
|---|---|
| 连接失败 / 超时 | 确认已连上 `SmartCar-XXXX` 热点；ping 192.168.4.1；确认无其他电脑占用控制会话（第二个连接会被 BUSY 拒绝） |
| 收不到遥测 | Windows 防火墙放行 UDP 5001（见上文）；确认 monitor 已 SUBSCRIBE；确认热点隔离设置 |
| monitor 启动报"本地端口被占用" | 已有 monitor 在运行：先用它（`--monitor`）或停止它；工具不抢占其他进程 |
| 调参返回 BUSY | 设备正忙（如导出中）；稍候重试 |
| 调参返回 SPECIAL_ELEMENT | 处于特殊元素，普通赛道覆盖被拒；回到普通循迹再调 |
| 调参返回 OUT_OF_RANGE / INVALID_ARG | 参数超出设备 limits 或整组不完整；用 `params get` 查 limits |
| 请求超时后结果未知 | 工具标 UNKNOWN 并在重连后 GET_REQUEST_RESULT 查询；NOT_FOUND 即 UNKNOWN，绝不自动重放 |
| dump 返回 BUSY/NOT_FOUND | 未停车、日志未收尾或无有效日志；停车后稍候再试 |
| dump 中断 | 保留 `.partial` 与原因文件；重新执行 dump 从 0 开始（不续传旧会话） |
| link_ready=false | STM32↔ESP32 SPI 链路未建立；只能查询连接状态，等待链路恢复 |

## 测试与验证状态

```bash
cd Tools/SmartCarLink
python -m unittest discover -s tests -v
```

当前 146 个测试全部通过，覆盖：协议帧/记录/CRC/JSON/共同测试向量、
模拟 TCP 拆包粘包/部分发送/可靠重发/结果延迟/会话变化/NOT_FOUND→UNKNOWN、
UDP 缺口/乱序/重复/队列溢出/新运行归零/tick 回绕、调参整组拼装与校验、
导出重复块/错偏移/整体 CRC/.partial 保留/CSV 57 列/灰度归一化对照/convert。

**待验（未上机）**：实机 60 秒 1kHz 遥测与 Flash 导出逐条对照、调参生效拍、
元素暂停恢复事件、停车保存流程、真实热点下 UDP 丢包率与防火墙行为。
以上项目仅通过模拟对端验证，未在真实车辆上测试。

## 目录结构

```
Tools/SmartCarLink/
├── smartcar_link.py   # 命令行入口
├── scb_cli.py         # argparse 子命令
├── scb_protocol.py    # SCB/1 帧头/CRC32/JSON/增量解析（不依赖 socket）
├── scb_records.py     # 96B 记录/256B 日志头/遥测负载/灰度归一化/57 列 CSV
├── scb_vectors.py     # 三端共同测试向量（固定十六进制）
├── scb_session.py     # 可靠层与会话状态机（可注入时钟）
├── scb_client.py      # TCP 客户端 + UDP 接收
├── scb_output.py      # 实时输出文件组（raw/CSV/metadata/events/stats）
├── scb_export.py      # 完整导出与离线转换
├── scb_tune.py        # 调参业务逻辑
├── scb_monitor.py     # monitor 与本地命令接口
└── tests/             # unittest（含 mock_esp32.py 模拟对端）
```
