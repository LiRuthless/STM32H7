# Phase02 G4 检查简化审查与验证

2026-11-07；规约基线90a60cb，适用Phase02 FR-1～6/TC-1～3及[本组设计](../../specs/features/02-scb-protocol/check-simplification.md)。来源为本工作区实际main源码；未关闭Phase02/05，未启动06。

## 范围与结果

- SPI/NET外部入口仍完整解码。生产握手、SPI随机候选重试和可靠接收复用已验证视图；原始安全入口保留。
- JSON object_get保留完整验证；内部object_get_validated仅定位已验证且不变的根/子对象，不重复扫描全文。重复键、UTF-8、有限数、完整消费和嵌套边界保留。
- NET一次提取op，供SUBSCRIBE、信用分类和日志请求元数据复用。SPI RESULT status一次提取，commit复用OK事实；仅当前BEGIN预留读取transfer/total。
- NET生产TCP/UDP重封装复用已验证负载；结构、容量、地址范围/别名及新CRC保留。SPI peek_tx为可配置回调，缺少验证来源保证，继续完整编码，不扩张回调信任。
- 本地EVENT_GAP生成后用NET原工作区首次验证再发布；没有未验证本地JSON进入内部编码。移除bridge_state不再需要的SPI全量JSON工作区，改为SPI owner的一个派生布尔事实；未新增池或动态分配。
- 公共SCB/1与Phase02 FR-2/3要求的遥测内容/保留区、LOG_DATA保留字段和有限数检查全部保留。第二批放宽不实施。

## 具体静态审查

| 维度 | 核对与结论 |
|---|---|
| 外部输入 | spi_protocol receive和NET dispatch先scb_decode；旧UDP offer亦完整decode。TCP头预检保留。validated接口没有外部字节直接入口 |
| 可靠接收 | raw和validated共用一份状态机；session/seq、可靠同序号有效帧字节一致、旧帧和真实reserved判定保持；原始字节仍来自当前DMA/socket帧 |
| 输入/输出别名 | receive_validated先复制视图再清输出，支持生产调用同一view作为输入输出；字节不改。编码保留输出与payload及link不可变图像的范围检查 |
| 顺序 | SPI方向/request拒绝仍在ACK处理前；reserve后receive，DELIVER后同步commit，非交付cancel；编码成功后才commit_prepared消费seq，完整发送后才启动计时 |
| JSON来源 | HELLO、SUBSCRIBE、COMMAND元数据、RESULT状态、EVENT锚点来自已完整验证负载；args和选定值均由合法子视图产生。对象查询不缓存跨轮状态 |
| 队列与修改 | 接管同步复制后借出只读，consumed结束借用；唯一遥测内容更新是偏移12丢弃计数，不影响已检查约束。EVENT_GAP格式生成后首验；其他本地响应完整编码 |
| 状态owner | session/link/workspace仍各任务独占。s_spi_result_ok仅SPI reserve/commit使用，下一reserve覆盖；无NET写入。commit增加短锁读取BEGIN操作，在锁外解析后原锁内仍复核epoch/revoked |
| 并发与资源 | 所有epoch/generation、借用撤销、池与DMA保护保留，锁序仍bridge→pool；没有锁内JSON/socket/等待，队列额度与超时不变 |
| 模块化 | encode_geometry/encode_image隔离共用检查与副作用，prepare_header/commit_prepared隔离窗口判断与提交；避免两份状态机。原字节包装入口承担验证边界和接口兼容，有保留价值 |
| 格式/注释 | 新接口有中文Doxygen与已验证前置条件，新增成员解释owner；新代码检查100字符行宽、空白及声明/定义/调用对应。删除失效field_equal与私有工作区，保留无改动模块 |
| 跨模块接口 | bridge_state_net_reserve传入op；net_peek传NET工作区验证本地GAP；全部生产调用同步。历史tests夹具不运行、不扩展，不代表其旧内部接口仍可编译 |

## 正式构建证据

首次受限启动Xtensa工具失败（OS error=623）；获准后使用同一既有SDK/工具及正式入口，不安装或换版。
第一轮64bddceae0fa4d1ab3077f439fecd8f4构建成功，但缺少ESP_ROM_ELF_DIR导致CMake告警，不记零警告。
核实SDK tools/tools.json推荐ROM符号版本20241011已在本机，最终构建仅为当前进程补齐ESP_ROM_ELF_DIR=C:/Espressif/tools/esp-rom-elfs/20241011/，不改工程配置或SDK。

最终run_id：b5bc560ef329489d8bc93ddf7f4e30ff；manifest status=success、stage=complete；build、配置/依赖检查、size、产物回收均完成。目标esp32s3、SDK6.0.0（662a3be354759d9487bf4b1a629fadb766cb1800，clean）、Python3.11.15、Xtensa esp-15.2.0_20251204、CMake4.0.3、Ninja1.12.1。本地密码使用既有忽略配置，不输出或提交。

主模块文件哈希与manifest逐项一致；脱敏build.log未发现error:/warning:/CMake Warning。证据保存在本地artifacts/b5bc560ef329489d8bc93ddf7f4e30ff及.local/build-diagnostics/同run_id，不提交含配置的BIN/ELF或产物。

## 未验与Git边界

未新增/执行宿主、单元、模拟socket/SPI/DMA测试。未烧录、复位或运行真实三端验证；坏帧、重复、背压、重连、导出及遥测功能仍待实机验收，无吞吐/延迟实测，不把静态审查与编译当成功能通过。
开始已有20个源码/头文件的700行注释增量、未跟踪注释报告及Tools/STM32Flash。保存本任务开始的main基线，提交只加入本任务差异；已有注释增量及其他目录不暂存、不提交、不删除。未push。
