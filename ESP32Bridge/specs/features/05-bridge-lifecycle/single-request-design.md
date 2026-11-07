# Phase02/05：第一、二组负载检查与单请求槽

2026-11-07用户明确“按照方案执行第一第二组”。基线32b53ac及既有700行注释增量；受ESP32Bridge前层治理约束。Phase02 FR-1～6、TC-1～3；Phase05 FR-1/2/5/6/7/8及资源契约；SmartCar Phase16公共协议文档与Tools/SmartCarLink客户端接入。第三组导出生命周期和EVENT_GAP职责迁移不实施。

## 第一组需求与三端边界

G12-FR1：ESP32继续校验帧魔数、版本、类型/flags、长度/记录数、CRC、JSON语法/UTF-8/重复键/完整消费与有界深度；取消遥测前缀非零、首tick一致及记录保留区逐字节检查；取消LOG_DATA reserved零检查，保留1～960长度、实际长度和offset加法溢出。
G12-FR2：JSON number只检查数字语法，删除binary64全局有限性比较；语法仍拒绝NaN/Infinity字面量，本地端口/日志/握手等u32转换继续范围与精确整数检查。错误枚举NONFINITE保留数值作旧接口兼容，不再由普通JSON校验产生。
G12-TC1：线格式/版本/生产端reserved零规范不改；明确桥接端不解释这些内容，STM32/电脑消费端可继续严格检查，不把非有限车辆参数变成合法业务。合法帧字节仍原样转发。SmartCar源码不修改，无车辆控制或存储格式变更。

## 第二组详细设计

G12-FR3：删除普通/查询/导出信用分类和遍历。一个COMMAND结构/1024B数据、两个RESULT结构/2×1024B数据（RECEIVED中间与最终），一个credit含request/tombstone/active/sent及既有导出元数据。保留EVENT/UDP独立FIFO。新增桥接帧数据从10240B降至4096B（含1024B GAP），减少6144B。
G12-FR4：net_reserve在单信用不active、无tombstone、COMMAND空且身份有效时锁定；不靠op分类，只识别LOG_BEGIN/READ/END的现有导出元数据。SPI结果匹配一个request；中间/最终仍有独立槽。最终TCP精确ACK后才释放信用；中间ACK不释放。统一预留/租借状态使用布尔表达单COMMAND占用，结果索引0/1和SPI丢弃/事件哨兵继续保留其语义。
G12-FR5：断线时未交SPI的请求撤销；已交SPI且结果未明的请求变为唯一墓碑，至SPI换会话才清。墓碑期间所有新转发请求（含查询）背压，不增加自动复位/会话切换。SUBSCRIBE仍本地处理，不占STM32请求槽，但电脑按统一命令串行。
G12-FR6：第三组未执行，保持导出10秒、BEGIN/READ/END和EVENT_GAP状态。expire_export只有当前活动信用为导出操作时才撤销COMMAND/结果并转墓碑，不能取消期间并行到来的普通命令。单请求槽下不保证等待APPLIED时查询前进，此项明确替代Phase05 FR-2旧独立查询通路。
G12-FR7：电脑客户端统一命令槽，不再仅保护调参；已有请求未获最终RESULT/LOG_DATA时其他命令本地拒绝，不无界排队。RECEIVED可以返回给request调用者但不释放槽；业务超时不证明未执行，保留槽到迟到最终或断线，禁止自动重放。断线清本地发送资格但不抹请求记录，ESP32墓碑仍可能阻塞重连查询，直到新SPI会话。
G12-FR8：客户端收到新/重复可靠帧主动发送IDLE确认；新帧ACK成功发出后才分派并唤醒等待者，使下一命令在TCP字节序上位于前一结果ACK之后。完整send保证，失败不交付成功。复用既有Session.send_frame(MSG_IDLE)和发送锁，不改变去重窗口或新增字段。分配编号、编码或发送前失败正确释放占位；仅对应request的最终响应能释放槽，未匹配结果不能释放新请求。

## 文件/符号与实施次序

A 纯规约提交：本设计，ESP32 Phase02/05三件套增量，SmartCar Phase16三件套公共职责增量。
B 第一组：先Docs frame-codec/json-validation和公共communication说明桥接与消费端边界，再改protocol::payload_check、json_guard::number及说明。保留u32 number语法调用和所有溢出检查。
C 第二组：先Docs forwarding-and-lifecycle、network、resource-pools、diagnostics和Tools README，再改bridge_state.c/.h的单credit/COMMAND/RESULT、reserve/commit/cancel/peek/consumed/acked/expire_export/snapshot；net_bridge删除分类op传入；Tools scb_client统一请求owner和先ACK后分派。信用/墓碑统计保持原字段但只使用bit0，避免诊断接口无意义改名。
D 交付：同步roadmap/CHANGELOG/traceability/validation，记录替代旧三类/查询资源规范；既有阶段不关闭、不进入06。

## 安全、失败与验证

SPI/NET各任务owner不变，JSON/编码/socket仍锁外，bridge→pool锁序不变。清理借用时只撤销，consumed归还；任何新连接不得收到旧响应。最终ACK与新命令之间保持先ACK后准入，普通背压仍由原可靠重试处理。无新堆、池或同步任务。
静态审查全调用、单槽初值/错误/取消、间接索引与导出条件；正式v6.0.0当前配置零错误零警告构建。电脑脚本仅做语法编译和逻辑审查，不运行宿主/模拟测试。SmartCar本轮只有文档，无源码/配置变更，不启动R08/固件构建。真实三端串行、迟答/断线墓碑、超时、导出、遥测与损坏输入仍待实机，不烧录/复位、不安装/换SDK、不push。资源节省为静态数据差额，不伪称总堆或吞吐实测。

## 实施细化

NET仍一次提取op传入reserve，供保留的导出元数据使用，不再分类。电脑编码先完成，再在请求锁内原子分配编号并占槽，无占位编号；编号失败不占槽。读取/重试/构帧对Session的修改共用短session锁，socket在锁外；负载解析成功才主动ACK，再发布结果。坏负载关闭而不发ACK，避免把桥接内容放行当作电脑消费成功。
