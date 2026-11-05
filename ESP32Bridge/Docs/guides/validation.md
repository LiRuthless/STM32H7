# 验证、证据与交付

> 维护：2026-11-05。执行政策唯一见 [AGENTS](../../AGENTS.md)，代码/注释标准见 [code-style](../../specs/code-style.md)。

<a id="f10"></a>
## F10 构建工具、验证方法与交付边界

### 状态与接口

构建环境检查、复制构建、产物回收及本端协议/驱动/网络测试已存在。Phase06完整60秒负载故障工具、三端实机及最终交付手册未完成。测试存在不代表本轮执行；最终交付能力与历史脚本能力分开。

输入为明确SDK、目标、配置、版本及测试刺激；输出为非零失败或脱敏产物清单/日志/状态。构建部署步骤见 [build-and-flash](build-and-flash.md)，不在Phase复制工具内部表。版本锁定ESP-IDF v6.0.0，目标esp32s3；本地密码配置必须被忽略。构建复制白名单至无空格临时目录，不移动仓库、不遍历其他工程；清理前验证实际目标范围。SDK/工具启动失败应显示工具名及系统错误，不泄漏配置，不自动切换/安装SDK。

### 方法选择与通过标准

| 类别 | 方法与判据 | 限制 |
|---|---|---|
| 文档 | 检查功能唯一性、58条FR/TC和R6增量、符号/owner、链接及范围 | 不触发固件构建或行为回归 |
| 代码及注释 | 按code-style检查格式、准确性、调用顺序、状态/并发、接口与错误路径 | 不以注释数量或模板证明内容正确 |
| 当前配置构建 | 复用scripts/build.ps1，核对SDK/工具/配置及真实产物，零错误零警告 | 无环境则记未验证；不代表吞吐/硬件通过 |
| 协议与故障 | 复用既有tests及候选向量，按变更选择边界、错误、去重、重试/背压 | 假SDK/socket/DMA均为软件测试 |
| 实机SPI/网络 | 对应功能页方法，记录板型/镜像/串口/接线、抓包/波形 | 编译、宿主和单项上机不能代替整链路 |
| Phase06目标 | 至少60秒1kHz、原始96kB/s；拥堵/断线/导出字节及独立CRC对照 | 当前未执行；计输入/接管/发送/丢弃、峰值及内存，不强求UDP绝对零丢包 |

每次执行记录日期、工作区或提交、SDK版本、配置、输入、方法、实际结果与证据/缺失。状态区分通过、失败、未执行、阻塞、硬件待验、历史已通过。传输ACK不代表APPLIED/SAVED；本端候选向量不代表三端一致；未做不能填通过。

### 工具归属与实现定位

| 文件或辅助目录 | 关键入口 / 责任 |
|---|---|
| [activate.ps1](../../scripts/activate.ps1)、[build.ps1](../../scripts/build.ps1)、[build_support.py](../../scripts/build_support.py) | 环境选择、build_project/copy_project/safe_cleanup/validate_flash_metadata、失败返回与脱敏回收 |
| [check_config.py](../../scripts/check_config.py) | read_password/read_idf_version/check_environment/check_generated；不输出密码 |
| [generate_vectors.py](../../scripts/generate_vectors.py)、[check_vectors.py](../../scripts/check_vectors.py)、[向量说明](../../tests/vectors/scb-v1.md) | 生成/独立核验固定字节、CRC与哈希；生产编解码验证归F02 |
| [tests](../../tests/) | 协议/可靠/会话归F02/F03；SPI归F04；网络归F05；foundation归F01；构建/配置归F10 |
| tests/*_host.py、tests/host/*.c、tests/host/include/ | 编译和调用真实生产C的边界夹具/SDK替身，不是产品或实机能力 |
| 未来test_throughput与桥接生命周期工具 | 计划入口，当前不存在，不给可运行链接 |

### 历史证据及复现

阶段validation选择本阶段标准并记录结果，[validation目录](../validation/)保存各次报告和原JSON。旧D盘路径、SDK和提交号属于当时环境；本机缺artifacts/.local日志时标“本副本缺原产物，未重验”，不得改哈希/计数补齐。当前工程未跟踪，文中历史提交不视为已在本仓验证。

2026-10-20记录中的222项是历史主机结果；旧镜像启动/AP/DHCP成功与修复后网络待复测分开。[开工记录](../validation/phase-05-preflight.md)给出原环境操作步骤；当前指南需从实际项目根定位并核验设备，不自动使用历史COM3。
本轮迁移记录及文档验收见 [2026-11-05整理记录](../../specs/research/2026-11-05-document-system-alignment.md)。
