# Phase 01：工程基础 — 需求

> 2026-11-05整理；状态：软件基础已存在；阶段实机项未全部验收。本轮仅文档，不改变原阶段身份、编号或硬件验收。原交付记录见末尾冻结区。

## 动机与范围

工程配置、输入与密码检查；启动依赖、固定池与错误回收；环境选择、复制构建及中文说明。当前/目标详细契约在Docs唯一维护，来源决定与历史依据保留在冻结区及关联决定页。

## 功能需求

<a id="fr-01"></a>
### FR-1 独立esp32s3工程与明确SDK依赖

- 主契约：[F10 工具、验证与交付](../../../Docs/guides/validation.md#f10)。
- 状态：当前实现及待验边界见主契约。

<a id="fr-02"></a>
### FR-2 按依赖启动并记录真实失败，不因网络不可用永久停用SPI

- 主契约：[F01 启动、配置与固定池](../../../Docs/system/startup-and-resources.md#f01)。
- 状态：当前实现及待验边界见主契约。

<a id="fr-03"></a>
### FR-3 本地密码输入校验与保密

- 主契约：[F01 启动、配置与固定池](../../../Docs/system/startup-and-resources.md#f01)。
- 协作：[F05 网络与单控制会话](../../../Docs/transport/network.md#f05)。
- 状态：当前实现及待验边界见主契约。

<a id="fr-04"></a>
### FR-4 内部固定池、一次分配和明确所有权

- 主契约：[F01 启动、配置与固定池](../../../Docs/system/startup-and-resources.md#f01)。
- 状态：当前实现及待验边界见主契约。

<a id="fr-05"></a>
### FR-5 无空格临时目录构建、脱敏回收及有界清理

- 主契约：[F10 工具、验证与交付](../../../Docs/guides/validation.md#f10)。
- 状态：当前实现及待验边界见主契约。

<a id="fr-06"></a>
### FR-6 中文构建配置说明与无密码示例

- 主契约：[F10 工具、验证与交付](../../../Docs/guides/validation.md#f10)。
- 状态：当前实现及待验边界见主契约。

## 技术约束

<a id="tc-01"></a>
### TC-1 锁定ESP-IDF v6.0.0及真实环境取证

- 主契约：[F10 工具、验证与交付](../../../Docs/guides/validation.md#f10)。
- 状态：当前实现及待验边界见主契约。

<a id="tc-02"></a>
### TC-2 固定容量、DMA对齐与资源测量边界

- 主契约：[F01 启动、配置与固定池](../../../Docs/system/startup-and-resources.md#f01)。
- 协作：[F04 SPI事务与协议适配](../../../Docs/transport/spi.md#f04)；[F07 遥测与源事件](../../../Docs/bridge/forwarding-and-lifecycle.md#f07)。
- 状态：当前实现及待验边界见主契约。

<a id="tc-03"></a>
### TC-3 规约先行、范围及提交纪律

- 主契约：[F10 工具、验证与交付](../../../Docs/guides/validation.md#f10)。
- 状态：当前实现及待验边界见主契约。

## 决定与非目标

用户决定、原日期及提交声明保留在下方原记录；现行冲突统一见[known-issues](../../../Docs/known-issues.md)。不修改SCB/1、不替代STM32业务、不以软件通过代替硬件完成。
## 2026-11-05 存量格式与注释整改

复用FR-2/3/4、TC-2/3的功能身份，落实[本轮整改契约](../../research/2026-11-05-source-style-remediation-draft.md)的G1 启动与池；仅排版和真实注释，不改变功能契约、类型布局、表达式/初始化/写入顺序、协议、任务或错误行为，不新增阶段，不开发05/06。原实机与未决状态保留。

<details>
<summary>整理前阶段需求与决定（冻结原文，不作为现行规则）</summary>

~~~~~markdown
# Phase 01：工程基础 — 需求

2026-10-18诊断增量：环境版本与依赖检查保持不变；工具启动失败须给出不含配置内容的工具名、系统错误号；不得吞没为无法定位的Environment check failed或因失败自动切换SDK。

> 已提交规约基线 `5c15c49`，用户已授权本阶段分组实施；Group 1已提交为`8750d6a`，Group 2规约73a2a2f、实现a2947a8；Group 3实现及验证待审查，阶段整体验收未完成。依据：[技术栈](../../techstack.md)、来源任务书“任务与授权边界 / A / 验收2、7”。

## 功能需求

- **FR-1**：在ESP32Bridge创建独立C工程，目标esp32s3，仅引用本工程及指定SDK，不依赖PSRAM，不使用参考资料目录作为源码位置。
- **FR-2**：按NVS→网络接口/事件循环→SoftAP→socket服务→SPI任务的顺序尝试启动。初始化失败明确记录阶段和错误，停止受影响服务；网络服务不可用不作为SPI永久停用条件，SPI驱动失败不发布READY。普通运行期socket错误不触发整机重启。
- **FR-3**：热点密码由被本工程忽略的本地配置提供；示例只解释填写方法，无真实密码。8～63个ASCII字符，缺失、含NUL/非ASCII或长度非法均报错且不启动开放AP。不在日志、版本清单或交付文件泄漏密码。
- **FR-4**：启动一次分配并检查内部DMA缓冲与固定队列池，失败则相关服务不接管业务；数据路径不反复malloc，不共享驱动仍持有的内存。容量、所有权和峰值可供后续测试核对。
- **FR-5**：构建入口验证v6.0.0环境后，将本工程复制到无空格临时目录构建，保留本地必需配置但不发布密码；不复制.git、不移动原仓库、不遍历复制其他目录。仅回收本工程产物与脱敏版本/内存报告，失败保留诊断并返回非零；清理仅作用于本次已核实的临时目录。
- **FR-6**：交付中文环境/配置/编译说明和无密码示例，明确首次目标设置、版本核验、构建产物位置及失败排查。实际构建前不能把示意命令写成已执行记录。

## 技术约束

- **TC-1**：ESP-IDF精确锁定v6.0.0，环境基线`D:\esp\v6.0\esp-idf`；构建记录SDK实际提交及工具版本，不接受PATH中其他SDK静默替代。
- **TC-2**：SPI双缓冲每个至少1024B且DMA对齐、内部可达；UDP16包、EVENT32条；SPI窗口1。最终总内存须由构建与启动测量给出，不能将SDK驱动动态分配误写为全系统无动态内存。
- **TC-3**：实现前纯规约提交，文档/代码逐批审查；只暂存明确本工程路径。产物、构建缓存和密码配置必须排除，默认不push；用户本次已解除当前获准任务组的工程文件创建限制。

## Group 1 实施追溯

本组复用已提交 FR-1、FR-3、TC-1、TC-3，不新增需求或共享接口。实际工程入口为 [CMakeLists](../../../CMakeLists.txt)，配置检查为 [check_config.py](../../../scripts/check_config.py) 的 `read_password`、`check_environment`、`check_generated`；Group 1交付时app_main仅配置自检；当前启动与池已由Group 2扩展。历史[验证证据](../../../docs/validation/phase-01-group-1.md)与 [validation](validation.md) 保留此边界。

## 接口约定与非目标

输入为已激活的SDK环境和本地密码配置；输出为明确的服务启动状态及构建产物/诊断。应用入口和池接口是后续内部接口，见 [计划](plan.md)；不新增SCB/1消息，不修改对端，不实现车辆存储。

## Group 2细化与实施（规约73a2a2f，实现a2947a8）

FR-2、FR-3、FR-4、TC-2的内部接口、资源表、所有权与失败验证见 [增量规约](group-2-design.md)。推荐启动一次分配两块1024B DMA、16×1016B UDP和32×1024B EVENT；51072B仅为数据区容量推导。阶段05可靠/查询保留槽另行审查，AP/socket/SPI当前未实现须如实报告，不以缺失回调返回成功。实际符号为 `bridge_pool_*`、`bridge_startup_run`、`bridge_password_validate`、`bridge_startup_idf_run`；本次59项主机及目标构建结果见 [Group 2证据](../../../docs/validation/phase-01-group-2.md)。实机验收未执行。

## Group 3实施追溯

FR-5由 [build.ps1](../../../scripts/build.ps1)、[build_support.py](../../../scripts/build_support.py) 的 `build_project`、`copy_project`、`safe_cleanup`、`validate_flash_metadata`实现；TC-1复用配置检查并由 [activate.ps1](../../../scripts/activate.ps1)显式选择已有推荐工具。FR-6见 [中文说明](../../../docs/build-and-flash.md)。本组不新增外部接口/共享协议；本次77项主机检查、正式入口目标/失败场景与限制见 [Group 3报告](../../../docs/validation/phase-01-group-3.md)，源码部署密码仍缺失，实机部分未执行。
~~~~~

</details>
