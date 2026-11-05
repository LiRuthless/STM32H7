# 现有源码格式与注释整改人工复核

日期：2026-11-05。范围：main全部20个自维护C/H，tests/host的3个自有C夹具和24个SDK/运行库替身头，共47个文件。基线：35e56277f1842bc5509fa583c37103b4532f930d；纯规约：a0e0917。没有开发Phase05/06，没有改脚本、配置、协议或其他目录。本报告记录本轮执行，不覆盖历史报告和实机待验。

## 标准适用与实际结果

完整标准仍为[code-style](../../specs/code-style.md)，本批判据为[整改契约S1～S7](../../specs/research/2026-11-05-source-style-remediation-draft.md)。

- S1：逐文件核对四空格、独立控制/类型/跨行初始化括号、完整控制体、单行语句/声明、指针与运算符空格、case缩进及文件格式。保留CRLF、UTF-8无BOM和末尾一个换行；无Tab或尾空白。可读标量数组、for头沿用标准允许形式。未重排包含、声明或初始化。
- S2：main的110个函数定义均有C前置中文Doxygen，包含私有函数和固定名ISR/任务回调；9个生产头的接口为摘要与实际C实现定位。生产字段/枚举及共享状态补真实含义和访问上下文。未增加条件分支，现有#if配置拒绝路径保留；宿主编译配置的有效/非法密码分支由既有测试覆盖。
- S3：分别核对注释格式、参数顺序/方向、返回、空值、借用期、失败输出、单位和副作用。C的110个函数均保留details，无需为getter省略登记例外。头文件只写摘要及关键调用约束，不复制算法表。按标准第1节的适用范围，宿主夹具采用中文文件说明和151个定义的真实测试职责摘要；SDK固定接口不伪装为生产实现，3个头内联替身说明实际转换/转发。宿主SDK接口不重复套用生产模块前缀或完整SDK参数手册。
- S4：结合实现及调用方审查下述G1～G4具体路径，并用本批词法/宿主编译对照补充行为保持证据。发现的连接代际同步风险只登记，不修复。
- S5：目标构建未验证。现有activate入口实际失败，D盘不存在；没有切换SDK、安装工具或填写部署密码。不能宣称ESP-IDF v6.0.0目标构建零错误零警告通过。
- S6：本报告及逐文件表为交付前人工复核；不引入STM32 R08或JSON机制。全仓差异及暂存范围另在提交前核对。
- S7：Phase01～04、Docs入口/契约、roadmap、CHANGELOG和EB-03同步本轮状态；本轮静态整改与目标构建/实机验收分开，不把阶段整体勾为完成。

## G1：启动与固定池

app_main先调用startup_idf再读取报告及服务快照；原日志字符串和读取条件不变。startup_idf保持READY先锁存高再配置输出，随后密码、池、出口绑定、启动协调器的顺序。startup_run保留NVS→netif→event→AP→socket依赖，SPI仍独立于网络链；池失败和READY/密码失败各自阻塞的服务未改变。NVS错误仍直接返回，不擦除Flash、不复位。

池init只有四块全部检查并清零后才发布；失败按原顺序回滚。deinit的忙判断和锁外free未移动；跨deinit代际仍保留。DMA状态转换的源状态、非零generation、地址及容量校验保持同一短路顺序；只有回收证据允许DRIVER→APPLICATION。claim成功才写slot，失败保留输出；dma_acquire同样仅成功写pair。startup_run参数无效时不初始化report，不能写成所有失败路径清零。锁仅保护元数据，不保护借出数据内容，init/deinit仍要求停服无并发。

foundation_harness仅在宿主注入分配/能力、启动顺序和错误；原错误码、CHECK条件、2000次循环及代际耗尽用例未修改。SDK固定接口和现有g_测试状态命名保留。

## G2：帧、JSON、可靠窗口和会话

protocol保持头/长度→CRC→负载检查及成功后发布view的顺序。encode先检查容量/别名/负载，再写输出；失败used为零，decode失败有效view清零。头偏移、96字节布局见证、保留区、1024字节SPI尾零和TCP/UDP无填充保持。布局见证只表达公共记录字节，不添加车辆或硬件语义。

JSON仍使用调用者独占工作区，无堆、无递归；重复键比较的是解码标量，不做Unicode规范化。私有游标函数失败可部分推进，未误写为回滚。binary64溢出边界原字符串及比较顺序不变，u32只发布精确非负结果且失败保持已清零输出。

可靠层仍先检查完整身份，合法帧携带的精确ACK只释放pending且sent的TX；背压不允许新可靠入帧推进接管ACK。DELIVER后调用方先commit再发ACK，重复/旧帧不重复交付；可靠身份缓存不被非可靠高序号覆盖。sent只在全部字节/完整事务完成后记录毫秒起点，首次不计重发；poll保持199/200ms边界、三次重发后再等到期才重建，原缓存不改ACK字段。

会话保持WAIT_HELLO→WAIT_IDLE→READY；WAIT_IDLE重复HELLO复用会话，新的随机候选仍须非零且避开旧值。TCP生产路径使用显式SPI快照，未共享可写SPI对象；换会话撤许可和关闭请求的先后顺序不变，接管ACK不升级为APPLIED/SAVED。

## G3：SPI、READY与DMA

spi_task保持Core1初始化、固定任务配置、初始化结果发布后通知启动者。run_transactions仍按prepare/清RX和描述→pool submit→queue(0)推进，post_setup拉低READY、post_trans拉高；ISR只做IRAM GPIO写，不解析、日志、加等待锁或socket。原IRAM_ATTR及SDK名称保留。

等待超时只调用不接收DMA指针的protocol_poll并发布快照；软件超时、会话失效都不是回收依据。只有get成功且返回原描述才reclaim，之后读取trans_len；完整物理发送先通知sent再校验RX，坏RX不抹掉TX已完成事实。排队失败证明未接受时才撤回；get错误/陌生描述退出时保留可能仍被驱动持有的内存，不销毁池、不复位。长度0/8191/8192/可见8193以及硬件可能饱和的证据边界保持。

spi_protocol不在poll中复制重发图像；待回收后prepare才复制，期间精确ACK仍可取消待办。reserve/commit/cancel组合及同步复制借用视图的要求不变，缺Phase05消费者仍是真实背压/遥测丢弃，未补造生产队列。夹具setup中的虚拟DMA取消仅是用例隔离，不描述为生产能力。

## G4：TCP/UDP与连接身份

NET保持先核SPI会话、重试/IDLE、poll，再BUSY→控制TX/RX→accept→UDP→发布的循环顺序。TCP完整帧才分派，延后帧保留RX；满缓冲不做零长recv，recv零仍是EOF。TX只在cursor达到length后通知sent；EAGAIN、send零和短写保留原游标，停滞比较仍为大于阈值。BUSY用独立图像/游标及大于等于截止，不影响首连接。注释未将received误称为成功业务交付数。

SUBSCRIBE目的IP仍取accept对端，端口精确1～65535；合法订阅先更新状态再编码RESULT，编码失败走原关闭清理路径。close_control清订阅、RX/TX、TCP窗口及UDP pending，不重置SPI或重放设置。零会话dead链仍维持状态连接，非零可靠失败才按原路径关闭。UDP在锁内pending→in_use，锁外调用出口，再锁内释放；消费比较连接代际，旧身份不允许发送。s_generation的跨任务同步限制见EB-07，未借格式整改改变临界区。

AP仍按已取得步骤回滚；密码由启动协调器先校验，AP入口本身不新增校验。socket初始化失败不复位或永久停止SPI，UDP创建失败不回滚TCP。SDK栈余量接口的当前目标单位未在缺失v6.0.0环境下重新证实，字段说明保留“SDK原单位”，不伪造实测字节值。

## 行为保持证据及边界

1. 对全部47个C/H，以基线35e5627对照本轮代码词法序列：先按C规则消除反斜杠续行，忽略注释/空白和本次增加的大括号，去除声明类型/存储限定符的重复，并将逗号/分号统一作分隔符；47个序列一致。这不是完整等价证明，不能单独发现作用域、类型或分隔符语义变化；这些逐项结合差异和编译检查。
2. 对11个生产C，用同一MATLAB Clang 18.1.7、x86_64-pc-windows-msvc、C11、-O1、-Wall -Wextra -Werror及同一既有宿主配置/替身，分别编译基线和本轮至LLVM IR。只删除ModuleID/source_filename路径行后，11份IR全部一致；编译无错误/警告。此项临时选择用于广泛控制体/多声明改排的行为核对，不是历史固件二进制验收，也不能替代真实SDK布局、IRAM或目标构建。
3. 不新增宿主测试或模拟对端。复用9个既有生产相关模块的187项用例，最终全部通过，0失败/跳过；Clang使用-Werror。覆盖池、JSON/codec、窗口/会话、SPI任务与端口、TCP短写/连接/订阅以及冻结向量字节/CRC/SHA。Python/PowerShell构建工具未改，本轮不选其全量回归；不选资源/栈、多配置或架构迁移检查，因为任务/配置/资源/模块组织未改变。

验证过程中发现一次嵌套初始化排版重叠，词法对照发现夹具中多出的header赋值，已在提交前纠正，并重新执行187项验证通过。没有修改断言或生产行为来适配验证。CHECK仍返回实际失败行号；排版导致的__LINE__行号变化是既有诊断机制的预期位置更新，宏条件及求值次数未改。

## 环境、命令和本轮证据

宿主：Windows，Python 3.11.2（E:/APP/Espressif/tools/idf-python/3.11.2/python.exe），Clang 18.1.7（E:/APP/MATLAB/R2025a/toolbox/sldrt/clang/win64/clang.exe），既有LLD位于E:/APP/Espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/ld.lld.exe。只通过既有BRIDGE_HOST_CLANG/BRIDGE_HOST_LLD参数选择宿主工具，不激活或替换项目SDK。排版复用VS Code扩展已有clang-format 23.1.1，不新增项目格式化器或配置。

在ESP32Bridge/tests执行：

```powershell
$env:BRIDGE_HOST_CLANG = 'E:/APP/MATLAB/R2025a/toolbox/sldrt/clang/win64/clang.exe'
$env:BRIDGE_HOST_LLD = 'E:/APP/Espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/ld.lld.exe'
& 'E:/APP/Espressif/tools/idf-python/3.11.2/python.exe' -m unittest test_foundation test_protocol test_reliable test_sessions test_spi_driver test_spi_lifecycle test_network_stream test_subscribe test_vectors -v
```

最终结果：187项，9.746秒，OK。首轮选到只支持嵌入式目标的Espressif Clang 19.1.2，x86_64创建失败，0项执行、12个setUpClass错误；随后定位已有宿主编译器重新执行通过，不掩盖首轮环境失败。原始本轮日志保留在被忽略的.local/style-work，不提交密码、DLL/IR、原始日志或临时辅助命令文件。新报告的摘要与哈希见后表；历史222/258项及原镜像结果不计入本轮。

目标入口：在仓库根实际运行`. ./ESP32Bridge/scripts/activate.ps1`失败，提示D盘不存在。固定SDK tools/tools.json、默认工具和Python环境不可用，部署.local/sdkconfig.local缺失。仅发现的ESP-IDF v5.5.5不符合锁定版本，未切换使用；build.ps1实际目标编译未启动。无目标BIN/ELF或零警告构建证据，S5及全部硬件项保持未验证。

## 例外与剩余项

- 100字符例外：app_main.c的三条既有诊断字符串，spi_bridge.c的两条既有日志字符串；字符串原字节不拆改。json_guard.c的309位binary64精确边界字符串保持单一原token，作为有明确数学意义且须保留常量字节的本批例外登记。普通声明、参数及注释未借此豁免。
- 命名例外：保留SDK/CRT固定名、app_main、IRAM_ATTR、SDK字段和唯一保护宏/#pragma once；保留既有私有pool_state_t/record_layout_witness_t、MAGIC/ROOT_*及OFFSET，夹具g_状态、EV_*、CHECK/TRACE_MAX等，理由是本次只做格式/注释、不全量改名，不改变接口/测试职责。
- R-05、导出计时歧义、缺失bridge_state/bridge_stats、三端向量和全部硬件待验保持。新增[EB-07](../known-issues.md#eb-07)登记既有同步风险，无功能修复。
- EB-03范围内47文件的静态整改已完成；目标构建S5未验证，本报告不宣称Phase01～04或整机/全量验收完成。

## 逐文件人工复核清单

下表“格式/命名”包含第1～3节适用条款与上述例外；“注释”分别记录覆盖和格式，内容准确性/行为说明按G1～G4结合定义及调用方核对。摘要是本轮磁盘原始字节SHA256（CRLF），不等于Git规范化LF blob哈希；未整改目录不在表内。

| 文件 | 格式/命名 | 注释覆盖/格式 | 内容/行为复核定位 | 原始字节SHA256 |
|---|---|---|---|---|
| [main/app_main.c](../../main/app_main.c) | 已查，行宽例外见上文 | C完整文件/定义/状态说明已查 | G1 | f5cbc1b2a17334fd9bb8720adafe37709211aec81c077021ff0a011cea7b46ac |
| [main/buffer_pool.c](../../main/buffer_pool.c) | 已查，固定命名例外见上文 | C完整文件/定义/状态说明已查 | G1 | 9a154d17e9396edcad94450219cbfc605fc22ff84de66a502debae393f67ec26 |
| [main/buffer_pool.h](../../main/buffer_pool.h) | 已查，固定命名例外见上文 | H摘要/字段/枚举/实现定位已查 | G1 | bba483e915796d33fe0f88db2c3fc1c04b49c9b85b28d73a9432b843bd10484d |
| [main/json_guard.c](../../main/json_guard.c) | 已查，行宽例外见上文 | C完整文件/定义/状态说明已查 | G2 | 072376bd26e81bfb64f8623696e06b3fea6edb20b53035164902fa5030337ccf |
| [main/json_guard.h](../../main/json_guard.h) | 已查，固定命名例外见上文 | H摘要/字段/枚举/实现定位已查 | G2 | bc1400fe000f2e268baca0b12f2379cd8f3c45ca09a51d0d82c1b4dc295d2ddf |
| [main/link_session.c](../../main/link_session.c) | 已查，固定命名例外见上文 | C完整文件/定义/状态说明已查 | G2 | cc527c997414c3f302a7a5b410fcfef4d1f1624b40ea89c5021c1e082a3c3d66 |
| [main/link_session.h](../../main/link_session.h) | 已查，固定命名例外见上文 | H摘要/字段/枚举/实现定位已查 | G2 | ec81abcac0fb158f0a70086d7dc4e79a6719aba178cb46828978721634265bbc |
| [main/net_bridge.c](../../main/net_bridge.c) | 已查，固定命名例外见上文 | C完整文件/定义/状态说明已查 | G4 | a891f77c822d99fef22183f0d4af188c833c32bb4c726d902480f20d96bc6f5a |
| [main/net_bridge.h](../../main/net_bridge.h) | 已查，固定命名例外见上文 | H摘要/字段/枚举/实现定位已查 | G4 | d621700762bbe67baca52e9e781b0ace7f1d7d28837bb5d55734f12c594d38c7 |
| [main/protocol.c](../../main/protocol.c) | 已查，固定命名例外见上文 | C完整文件/定义/状态说明已查 | G2 | 3ac3496bfb33ddcef3902c5432a7df949fa1872eafcceab2207140dcf185b079 |
| [main/protocol.h](../../main/protocol.h) | 已查，固定命名例外见上文 | H摘要/字段/枚举/实现定位已查 | G2 | a7c52d61c3f71e58640ed46719a90367ef7056137712b8c9be02ae527394d0f3 |
| [main/reliable_link.c](../../main/reliable_link.c) | 已查，固定命名例外见上文 | C完整文件/定义/状态说明已查 | G2 | a2b768d5b39510a452455cf78bbb5b624ffbe3f4e89f187ca4db1c2a1969a85c |
| [main/reliable_link.h](../../main/reliable_link.h) | 已查，固定命名例外见上文 | H摘要/字段/枚举/实现定位已查 | G2 | e849240e30c954c7b84c5177552fbce87da04bb8151972810160519d7bfdbf05 |
| [main/spi_bridge.c](../../main/spi_bridge.c) | 已查，行宽例外见上文 | C完整文件/定义/状态说明已查 | G3 | 7b42fceeb18adbc7b8ca58637c295d9aaaac70c2f1a2f3567ee4611b5423347d |
| [main/spi_bridge.h](../../main/spi_bridge.h) | 已查，固定命名例外见上文 | H摘要/字段/枚举/实现定位已查 | G3 | f614aae75cc7dacb5d92b0449503329f32ebf24cbedc9286506a74b9d4ab9c81 |
| [main/spi_protocol.c](../../main/spi_protocol.c) | 已查，固定命名例外见上文 | C完整文件/定义/状态说明已查 | G3 | 5b33e595435f330df3d0d38f040ad0dadd2a87f796f223834f618eaa54a83b09 |
| [main/spi_protocol.h](../../main/spi_protocol.h) | 已查，固定命名例外见上文 | H摘要/字段/枚举/实现定位已查 | G3 | 6037d3dee563e5086cbadf2dc6144dadb4919ea5c453efbe868657ecefe60c67 |
| [main/startup.c](../../main/startup.c) | 已查，固定命名例外见上文 | C完整文件/定义/状态说明已查 | G1 | 1f2bcf3a4cb8689a1af051ee3b39e313ea4fc4279597f0d7123be75942777122 |
| [main/startup.h](../../main/startup.h) | 已查，固定命名例外见上文 | H摘要/字段/枚举/实现定位已查 | G1 | 51485265c6515107241aa1e85fb031898a3aeddc5bf1328f250a24d496e4c1d4 |
| [main/startup_idf.c](../../main/startup_idf.c) | 已查，固定命名例外见上文 | C完整文件/定义/状态说明已查 | G1 | 08bc18eda6a9978dee72f61fb56e32f47b3d470ea538c05a07df60294b395a79 |
| [tests/host/foundation_harness.c](../../tests/host/foundation_harness.c) | 已查，固定命名例外见上文 | 文件与测试定义职责摘要已查 | G1 | 7c44d95a02cf7a747f3d113cd9043b34abcc9e0e94537a58a1308c9e0e5f1a22 |
| [tests/host/include/driver/gpio.h](../../tests/host/include/driver/gpio.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | 7df6826c35dd5700e8a2f4cf56957029db353512482f0a6c338bb0c18a65fd4c |
| [tests/host/include/driver/spi_slave.h](../../tests/host/include/driver/spi_slave.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | a3e4543c310e178379336a173c59e319408e87b35c0f838efb06bf72eab36ba2 |
| [tests/host/include/esp_attr.h](../../tests/host/include/esp_attr.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | c49696c12f8f3e62d7f997f8f1847e3bfdcb60540748a63c2d481dc45b05142d |
| [tests/host/include/esp_err.h](../../tests/host/include/esp_err.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | 4ad5ac9c2b5cf7b23e92cc8cf115e9e7e1a7d82dfc6251bbecd0d491e87ecbd9 |
| [tests/host/include/esp_event.h](../../tests/host/include/esp_event.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | dc25e03c38ae424d51f5ccabbf82199e27e585dfc962bbab09f09d9217f81982 |
| [tests/host/include/esp_heap_caps.h](../../tests/host/include/esp_heap_caps.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | 3cdbf963ca70e406b02c30317badacf5eb3e3a569b4fdaf50b568ff7af9621f7 |
| [tests/host/include/esp_intr_alloc.h](../../tests/host/include/esp_intr_alloc.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | be2cf8199d3b125f93759ec9fb69e379213a16192f52efebfb83be49758ee87c |
| [tests/host/include/esp_log.h](../../tests/host/include/esp_log.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | fdf476a11602c64cb464cbff73f7d6a9551e7b01694ce633acaeeff21c1d4633 |
| [tests/host/include/esp_mac.h](../../tests/host/include/esp_mac.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | 8001c96ab3248b3bbe9b8c71c1bb25db948bc189f4c8b8493336b45f55e6d822 |
| [tests/host/include/esp_memory_utils.h](../../tests/host/include/esp_memory_utils.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | fc4a8b6207f83e60630f6f67c3045f8ad7dfe0c4307fcc12119d2858161f63f2 |
| [tests/host/include/esp_netif.h](../../tests/host/include/esp_netif.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | 7b2015c321ea46180a68d70acec40e54bf5ed8ee15e8a8ff530291e8bafc60f1 |
| [tests/host/include/esp_random.h](../../tests/host/include/esp_random.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | 0e3627504edd536470b99fa6e0604c4a6f5e7ccaf90870da1b366e47d4a041c0 |
| [tests/host/include/esp_timer.h](../../tests/host/include/esp_timer.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | 3204edfcaa9af09ab448b1ea8cfc6dd303d90cab834d4e05abe98afff59fe975 |
| [tests/host/include/esp_wifi.h](../../tests/host/include/esp_wifi.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | f21754e54adc74711dc9aa7516ba889387b6f9a56778b29063cc6544c7aedc9f |
| [tests/host/include/esp_wifi_default.h](../../tests/host/include/esp_wifi_default.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | 8a3b20a677876bbd82f12c5ceb9c947a25ddce323209eafb1ef8cc882d14cb42 |
| [tests/host/include/freertos/FreeRTOS.h](../../tests/host/include/freertos/FreeRTOS.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | 6a33ba5b18e809490147920334adffb2c0a2813d6d597fed3ec00868e21b5668 |
| [tests/host/include/freertos/semphr.h](../../tests/host/include/freertos/semphr.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | 94e63333a28fedd8bfb70ad4b2b6c8b9ae3f35610f638954b93a51f6f59dbab6 |
| [tests/host/include/freertos/task.h](../../tests/host/include/freertos/task.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | e2e6001a11cc83d8b26b50f03f05aeb7727697af4e866af2198f1d2632747c49 |
| [tests/host/include/hal/gpio_ll.h](../../tests/host/include/hal/gpio_ll.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | c53eb3e139593ab65c2c3737990c795edf889a2a805a4ce38f0889f0414f6f79 |
| [tests/host/include/lwip/sockets.h](../../tests/host/include/lwip/sockets.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | 74b36ebe87086d96b864dda58f3ebd9e43ca1078bdf5252c012aea3844d3b97d |
| [tests/host/include/nvs_flash.h](../../tests/host/include/nvs_flash.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | 256a19071c7ffe8ccc92223f6ca2f97daac43fc811156c633590627f586f803f |
| [tests/host/include/soc/gpio_struct.h](../../tests/host/include/soc/gpio_struct.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | d2ed7c3a9823214ea7b4417933cdde9715f46929b0ef9c36bfde1b919795fa7f |
| [tests/host/include/string.h](../../tests/host/include/string.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | 91f72773d7fbb4383a89c70253f8155e6b1b43991dc3257514164d035ad03a09 |
| [tests/host/include/test_sdk.h](../../tests/host/include/test_sdk.h) | 已查，固定命名例外见上文 | 替身职责/固定接口，内联及实际字段已查 | 共用替身，G1/G3/G4 | eb0dbb5e5e9df7ae74ef992de1dc04b7b2bd108e0fa0af230db47c267ad4f345 |
| [tests/host/net_harness.c](../../tests/host/net_harness.c) | 已查，固定命名例外见上文 | 文件与测试定义职责摘要已查 | G4 | 84dc56b5fbeb65b656819bf717ad09a9db623e3417672e9f4e88ca0f62c70810 |
| [tests/host/spi_driver_harness.c](../../tests/host/spi_driver_harness.c) | 已查，固定命名例外见上文 | 文件与测试定义职责摘要已查 | G3 | 65be11afa8b29e98d0c61b74823963fc0a85ac877e6f6ca53cc467b880eb3276 |

## 本轮本地证据摘要

| 本地文件（忽略，不提交） | SHA256 |
|---|---|
| .local/style-work/selected-tests.log | 72bc771376c492343be7b91ab0b533f25abfb1f78cfaf23addd9613af58d4d98 |
| .local/style-work/selected-tests-native.log | bf2390c86e64085be5907973b74d20db14a18000a98557d218c011911bad8b9f |
| .local/style-work/selected-tests-final.log | 5963bdd23f7991344e0da33684c5ab586dfc7e7f7eabb0c1c9880284d211effa |
| .local/style-work/production-ir-comparison.txt | d2578c0f7b967f7e3acd5b75423e4b34a312f76698307c10d2d399c3b75c91cd |
| .local/style-work/token-comparison.txt | cb0a3eab1145470302d2aa992911dc8c9473a711638bb113f43ea807c6cc60a9 |
| .local/style-work/target-environment.log | f819a339a963fffad345a8cc5646f1a7168351be916509b2b8810297d29d9536 |

### 行宽例外的最终定位

- [main/app_main.c:60](../../main/app_main.c#L60)：保留既有日志/诊断字符串token，110字符。
- [main/app_main.c:72](../../main/app_main.c#L72)：保留既有日志/诊断字符串token，221字符。
- [main/app_main.c:83](../../main/app_main.c#L83)：保留既有日志/诊断字符串token，139字符。
- [main/json_guard.c:414](../../main/json_guard.c#L414)：保留既有binary64精确边界常量字符串token，322字符。
- [main/spi_bridge.c:209](../../main/spi_bridge.c#L209)：保留既有日志/诊断字符串token，223字符。
- [main/spi_bridge.c:318](../../main/spi_bridge.c#L318)：保留既有日志/诊断字符串token，134字符。

## 文档与提交范围核对

对本批修改文档的673个相对文件链接作存在性核对，唯一缺失链接位于P01 plan的冻结原文（旧docs/build-and-flash.md）；保留历史原文，现行入口已指向Docs/guides/build-and-flash.md。本批新增链接、功能锚点和Phase身份已交叉核对；没有更改冻结报告、来源附件、旧计数或原始证据。代码以外只回填本批Phase/Docs/标准存量状态/roadmap/CHANGELOG，原工程基线导入与本次实现分开。
