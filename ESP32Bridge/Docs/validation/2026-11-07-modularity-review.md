# 2026-11-07 生产代码适度模块化审查

基线23520ddf9cea3a41870eca6765da9487fff1c1c4；规约先行提交8392bb9。范围仅ESP32Bridge；不关闭Phase01～05、不进入06。完整设计见[本批规约](../../specs/features/05-bridge-lifecycle/modularity-review-design.md)。

## 覆盖与未覆盖

已阅读main全部24个自维护C/H（13 C、11 H），核对main/CMakeLists.txt全部13个C均接入，无测试源混入。根CMake/配置及构建入口按依赖核对；未修改配置。下表每行均含源文件及对应头文件（如有），不是抽样结论。

| 已审文件 | 实际调用/owner及结论 |
|---|---|
| app_main.c | SDK app_main→startup_idf→startup；随后只读池/SPI/NET快照，保留薄入口和服务成功条件；修正过时诊断 |
| startup.c、startup.h | startup_idf→bridge_startup_run；报告无运行期共享状态，六步依赖/首错/池全阻塞/网络错不阻塞SPI逐分支核对；保留表驱动依赖协调 |
| startup_idf.c | app_main调用；READY→密码→pool→端口绑定→协调启动。pool_alloc核验内部及DMA首尾地址；锁包装绑定实际portMUX；init_nvs/netif/event_loop适配void*回调签名，非空洞透传 |
| buffer_pool.c、buffer_pool.h | startup初始化，SPI DMA转换，bridge FIFO claim/release，app/stats快照；代际和使用计数属于池；初始化回滚、重复地址去重、借出时拒绝销毁、无回绕代际均保留 |
| protocol.c、protocol.h | session/reliable/SPI/NET调用codec；无共享状态；header/payload/CRC、别名、显式字节序、96B见证各有不同校验责任，不合并为泛化模板 |
| json_guard.c、json_guard.h | codec/session/NET/bridge调用；工作区归任务，非递归状态机；scalar处理UTF-8/代理对，duplicate重扫直接键，number精确溢出，u32精确整数；短谓词隔离词法语义，保留 |
| reliable_link.c、reliable_link.h | SPI protocol和NET独立实例；receive完整身份去重、下游预留与反向精确ACK；prepare缓存，sent完整发送后起时，poll200ms/三重发后重建；不提升业务ACK |
| link_session.c、link_session.h | SPI握手与NET显式快照HELLO/BUSY；reply/hello共享角色及编码校验，保留；旧TCP状态API无生产调用，见可选项 |
| spi_bridge.c、spi_bridge.h | startup配置/启动；SPI Core1优先级12、6144B静态栈。ISR只写READY；run_transactions在queue失败或成功取回原描述后才reclaim；超时仅poll不读DMA；random_session适配随机回调签名，保留 |
| spi_protocol.c、spi_protocol.h | spi_bridge→prepare/poll/sent/receive/lost；协议实例属SPI，端口同步调用bridge_state；握手、可靠窗口与事务完成分离，端口reserve/commit/cancel和peek/consumed组合校验必要 |
| net_bridge.c、net_bridge.h | startup绑定/AP/任务；NET Core0优先级5、8192B栈。dispatch预留→receive→commit/cancel；rx_consume保留完整帧，TX短写游标、BUSY独立截止、poll有界；udp_service旧交接和生产FIFO入口互斥；清理无效参数已删除 |
| bridge_state.c、bridge_state.h | NET发布身份/预留COMMAND/借上行/通知精确ACK；SPI通过端口预留上行/借COMMAND/发布session；信用、墓碑、FIFO、租借及导出共用s_mux，统一生命周期是实际职责；只复用重复清理，不按文件长度拆状态 |
| bridge_stats.c、bridge_stats.h | NET每轮调用poll，5秒读取各owner快照后锁外串口；跨owner非同刻快照；保留诊断与数据路径分离 |

未覆盖SDK/第三方内部实现、生成文件、硬件电气、电脑与STM32实现、真实跨核调度和端到端故障注入；未审查/运行tests夹具实现，仅检索接口调用定位历史消费者。未执行软件测试，未创建测试脚手架。工具脚本只核对构建准入，不做全量工具审计。

## 明确问题及实施证据

| 文件与符号 | 调用证据、具体问题及维护影响 | 实际修改 |
|---|---|---|
| net_bridge.c::close_control、rx_recv | session_check/link_service/handle_subscribe/dispatch/rx_recv/rx_consume/tx_service/run_service/net_task共18处关闭调用原来均传health；close_control只(void)health，rx_recv仅向关闭函数透传。让清理接口看似依赖统计，实际归因在调用点 | close_control(void)、rx_recv(void)，18处关闭及1处recv调用同步；不移动计数点或清理顺序 |
| bridge_state.c::clear_queues、bridge_state_identity | clear_connection→clear_queues和NET订阅身份更新分别复制同一UDP释放/计包循环，未来易漏改一处；两者都已持s_mux | 提取私有clear_udp_queue，两处复用。FIFO顺序、dropped/cleared计数点及池锁顺序不变，不接触独占借出的s_udp_borrowed |
| bridge_state.c::expire_export | NET poll及SPI commit→expire_export；已无条件revoke COMMAND，active分支再次撤销同一帧，掩盖真正生命周期动作 | 删第二次幂等撤销，保留首次无条件撤销和结果槽处理；同锁内borrowed/revoked无并发变化 |
| app_main.c::app_main | startup_idf已绑定生产bridge端口，末尾仍写消费者/导出等待Phase05；注释也误导当前实现状态 | 日志提示上方生产启动结果及端到端待验，不无条件声称绑定成功；同步纠正注释 |

## 必要边界、可选项与决定

- startup回调表保留：依赖索引同时驱动步骤状态和错误来源；缺回调UNIMPLEMENTED与依赖BLOCKED有实际区别。删除表会复制六步的报告/失败分支。IDF三个短适配满足不同SDK函数与统一void*签名，pool_lock/unlock绑定元数据锁副作用，均不按短函数机械内联。
- net_receive保留：先保存pending/tx_seq，调用可靠层后只在真实确认时通知bridge；直接透传会丢失信用释放桥接。bridge_tx_service隔离借用→编码→consumed及NET游标发布。SPI适配层在DMA持有时只poll逻辑状态，其prepare/receive不能折入驱动循环而模糊所有权。
- bridge_state保留统一owner：拆成COMMAND/EVENT/导出各模块会跨模块协调epoch、断线、墓碑和在途借用，当前无收益。field_equal/object_u32复用真实字段检查，不是未来框架。snapshot/udp_valid/take_close分别封装一致副本、发送前资格和粘性待办消费，不能因类似getter而删除。
- DMA submit/reclaim/release虽短，却表达三种不同调用证据及状态转换；dma_transition共享凭据校验。协议get16/put16与NET头预检load16只在私有边界使用，为两字节运算新增跨模块公用接口没有收益。
- 可选/需决定：link_session.c/.h的scb_session_tcp_open/close/hello/permitted在main无调用；生产NET使用hello_explicit并独占socket状态。旧接口及scb_session_t内tcp_*字段保留了另一套逻辑状态模型，增加理解成本。建议另批确认兼容范围后一起退役，不能只删函数却留失效文档/状态；本批已询问用户，未得到决定前保留。不是证明生产双owner竞争。
- 可选保留：bridge_net_telemetry_offer仅历史net_harness调用，生产绑定后明确拒绝；UDP端口表仍实际用于真实socket出口。不为移除历史入口扩展到tests在途文件，也不恢复测试。
- 注释存量差距：spi_protocol.c文件块“当前无Phase05生产消费者时”表述有历史歧义（实际已绑定），net_bridge.h::backpressure注释只提无消费者但计数也含生产满额；本批未把它们当行为证据。后续可在所属Phase补充纯注释整改，不据此宣称24文件全部格式/注释合规。

## 十条设计原则逐项结论

1. 沿用现有启动/驱动/协议/bridge边界，无新模块框架。
2. 只把共同生命周期清理归到私有助手，不分散状态owner。
3. 新助手具有两处真实复用和同一锁前置条件；保留函数均有上表语义。
4. 未为缩短函数拆分dispatch或解析状态机。
5. 删除无效health参数链和重复撤销；保留有状态动作的ACK适配。
6. app_main、run_transactions、poll_wait等单调用入口保留副作用/复杂性隔离。
7. 未新增配置模式、未来队列框架或通用分派层。
8. 继续复用既有池、codec、可靠状态机和端口，不新增依赖。
9. 不全量改名、排版、删历史兼容接口，不触及其他目录。
10. 复核新增助手不增加锁、不暴露可写状态、不改变错误分支；没有用代码行数代替正确性及必要边界判断。

## 静态复核与构建实际结果

- 修改的3个C保持UTF-8无BOM及原CRLF；新增函数中文Doxygen、4空格、独占大括号，无新普通超长行。既有日志字符串超长按标准例外；存量长注释不声称全量整改。
- 已复核18处close_control调用、1处rx_recv调用、新助手2个调用及expire_export首次撤销；所有实参原为局部指针、无求值副作用。公开C/H接口、CMake、固定数组/栈/优先级/核号未改变。Docs在代码前更新、完成撤去同步中标记，语义双向核对。
- 锁/并发：NET socket和游标仍独占；bridge→pool无反向调用；snapshot锁独立释放后读取其他owner；SPI ISR不取任务锁。clear_udp_queue只处理排队槽，旧借用在consumed归还。静态推理不证明真实调度或临界区时长。
- 现有scripts/activate.ps1执行失败：固定D:\esp\v6.0\esp-idf所在D盘不存在。scripts/build.ps1以TEMP下无空格工作根调用，退出1：Activate the specified ESP-IDF v6.0.0 environment first; no tools will be installed。尚未进入编译，无新镜像，零错误零警告未验证；未安装、换SDK或跳过准入。
- 编辑过程中系统WindowsApps python.exe别名不可执行，未执行编辑脚本；改用PowerShell完成文件编辑。未运行任何宿主/单元/模拟测试。
- 实机待验：真实电脑↔ESP32↔STM32可靠ACK/背压、FIFO丢旧、断线/重连/会话切换、导出10秒取消、DMA/READY波形和负载水位。无烧录、复位或设备操作，不补写通过。

## 交付边界与剩余工作

本批静态审查与列明优化完成；目标编译及实机验证待环境/设备与授权。旧TCP接口退役为可选决定，不阻塞本批等价优化。规约独立提交后，Docs与实现及本报告同批提交；提交号由最终交付消息提供。初始8个在途文件原样保留：AGENTS.md、CHANGELOG.md、Docs/guides/validation.md、specs/code-style.md、specs/document-maintenance.md、specs/mission.md、specs/roadmap.md、tests/host/net_harness.c。未暂存它们，未push/合并。
