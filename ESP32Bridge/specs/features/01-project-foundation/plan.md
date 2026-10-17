# Phase 01：工程基础 — 实现计划

## 目标与依据

交付可独立构建的v6.0.0工程基础。用户已授权 Phase 01；宪章及本阶段规约已在 `5c15c49` 纯文档提交。本组复用已有需求，不新增未决设计或共享协议。关联 [使命](../../mission.md)、[技术栈](../../techstack.md) 和 [需求](requirements.md)。

## 任务分组与顺序

### Group 1：工程和配置

- [x] 创建目标工程、显式组件依赖、配置示例和本工程忽略规则（FR-1、FR-3、TC-1、TC-3）；代码已实现，待用户审查提交。
- [x] 校验SDK声明、所选Python及实际工具版本，错误退出而非自动切换或安装工具；官方全量export与本目标构建分别记录，见 [证据](../../../docs/validation/phase-01-group-1.md)。

### Group 2：启动与固定资源

- [ ] 实现阶段化启动、独立服务错误状态、内部固定池初始化/销毁和失败注入（FR-2、FR-4、TC-2）。
- [ ] 为SPI和网络预留初始化入口，不在本阶段伪造其协议或硬件通过结果。

### Group 3：临时路径构建

- [ ] 实现复制、构建、产物回收、路径核验与错误退出，补中文说明（FR-5、FR-6）。
- [ ] 运行 [验证清单](validation.md)，实际报告与状态回填后交用户审查。

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

Group 1实际文件为工程/CMake配置、`main/Kconfig.projbuild`、`config/local-config.example.md`、`.gitignore`、`scripts/check_config.py`、两份主机测试及 `main/app_main.c`。标准入口 `void app_main(void)` 执行编译配置校验后明确报告启动与资源未实现；没有SPI/网络假成功入口。配置检查不返回密码至日志，最终sdkconfig与本地文件必须一致，避免缓存旧密码。其余模块仍为计划；Group 2具体池/启动接口在代码前固定，必要增量须先用户确认并独立提交。

## 风险与取舍

沿用用户指定v6.0.0，不将旧5.5.1示例当作兼容证明。SDK源码存在不代表工具组合可构建。无空格临时目录需求保留，即使当前仓库无空格；Windows清理前必须验证绝对路径且不作用于源目录。SDK/Flash容量/串口未知项通过后续证据填写，不猜测实板事实。
