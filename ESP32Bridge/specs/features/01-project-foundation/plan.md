# Phase 01：工程基础 — 实现计划

## 目标与依据

交付可独立构建的v6.0.0工程基础。用户已授权 Phase 01；宪章及本阶段规约已在 `5c15c49` 纯文档提交。本组复用已有需求，不新增未决设计或共享协议。关联 [使命](../../mission.md)、[技术栈](../../techstack.md) 和 [需求](requirements.md)。

## 任务分组与顺序

### Group 1：工程和配置

- [x] 创建目标工程、显式组件依赖、配置示例和本工程忽略规则（FR-1、FR-3、TC-1、TC-3）；已随用户继续指令核对并提交为`8750d6a`。
- [x] 校验SDK声明、所选Python及实际工具版本，错误退出而非自动切换或安装工具；官方全量export与本目标构建分别记录，见 [证据](../../../docs/validation/phase-01-group-1.md)。

### Group 2：启动与固定资源

- [x] 用户确认 [启动与固定池增量](group-2-design.md)，已先独立提交为`73a2a2f`。
- [x] 实现阶段化启动、独立服务错误状态、内部固定池初始化/销毁和失败注入（FR-2、FR-4、TC-2）；实现已复核提交为`a2947a8`。
- [x] SPI/AP/socket保持显式未实现回调，报告UNIMPLEMENTED/BLOCKED；59项主机检查及目标构建通过，见 [Group 2证据](../../../docs/validation/phase-01-group-2.md)。

### Group 3：临时路径构建

- [x] 实现正式build/activate入口、白名单复制、受控产物回收、安全清理与中文说明（FR-5、FR-6）；代码待用户审查提交。
- [x] 运行 [验证清单](validation.md) 本组软件项：77项主机、普通/含空格两次真实构建、3个真实预期失败场景；报告回填后待用户审查。实机项不打勾。

Group 1→2→3，每组独立验证。SPI/网络的最终功能分别由阶段03/04交付。

## 计划模块与追溯

| 需求 | 计划位置 / 内部职责 | 输入→输出 | 验证 |
|---|---|---|---|
| FR-1、TC-1 | `CMakeLists.txt`、`main/CMakeLists.txt`、`sdkconfig.defaults`；目标/版本约束 | SDK与目标→匹配或失败 | V-1、V-7 |
| FR-2 | `main/app_main.c` 的应用入口 | 分阶段初始化结果→独立服务状态/串口错误 | V-2 |
| FR-3 | `main/Kconfig.projbuild`、`config/local-config.example.md`、`.local/sdkconfig.local`（忽略） | 本地密码→校验结果 | V-3 |
| FR-4、TC-2 | `main/buffer_pool.c/.h`；固定槽位及所有权接口 | 启动分配→池句柄或失败 | V-4、V-8 |
| FR-5 | `scripts/build.ps1`；构建入口 | 工程路径/环境→临时构建/产物/退出码 | V-5 |
| FR-6 | `README.md`、`docs/build-and-flash.md` | 验证过的步骤→中文说明 | V-6 |
| TC-3 | `.gitignore`与本阶段规约/审查记录 | 明确路径→受控交付清单 | V-9 |

Group 1交付工程配置与输入检查；Group 2现有 [buffer_pool](../../../main/buffer_pool.c) 的 `bridge_pool_*`、[startup](../../../main/startup.c) 的 `bridge_startup_run`/`bridge_password_validate`、[IDF适配](../../../main/startup_idf.c) 的 `bridge_startup_idf_run`、app_main及生产C主机测试。池容量/所有权按已提交增量实现，NVS/netif/event实际初始化，AP/socket/SPI尚未实现。其余模块仍为计划，Group 2实现已提交a2947a8。Group 3实际为 [build.ps1](../../../scripts/build.ps1)、[build_support.py](../../../scripts/build_support.py) 的 `build_project`/`copy_project`/`safe_cleanup`、[activate.ps1](../../../scripts/activate.ps1) 和 [中文说明](../../../docs/build-and-flash.md)；实际验证与待审清单见 [Group 3报告](../../../docs/validation/phase-01-group-3.md)。

## 风险与取舍

沿用用户指定v6.0.0，不将旧5.5.1示例当作兼容证明。SDK源码存在不代表工具组合可构建。无空格临时目录需求保留，即使当前仓库无空格；Windows清理前必须验证绝对路径且不作用于源目录。SDK/Flash容量/串口未知项通过后续证据填写，不猜测实板事实。
