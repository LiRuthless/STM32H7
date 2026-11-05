# 本地热点配置

在工程根目录创建 `.local/sdkconfig.local`，仅填写一条 Kconfig 字符串赋值：键名为 `CONFIG_BRIDGE_AP_PASSWORD`，等号后为双引号包围的本机密码。此示例不提供密码值，不要将密码发到聊天或放入命令行。

- 密码解码后为 8～63 个 ASCII 字节，不允许 NUL；双引号写成 `\"`，反斜杠写成 `\\`。采用 Kconfig 转义规则，不采用 JSON/C 的 `\n`、`\u` 转义。
- 本地文件只允许密码项、空行和注释，不用于覆盖目标或内存配置。
- `.local/`、生成的 `sdkconfig`、构建缓存和产物均被忽略。生成配置保留旧密码时，检查会明确拒绝；删除本工程被忽略的生成 `sdkconfig` 后重新配置。
- `scripts/check_config.py --local .local/sdkconfig.local` 可独立校验格式；增加 `--environment` 校验指定 IDF、Python、SDK 推荐编译器/CMake/Ninja及 Python 依赖。诊断不输出密码。

CMake 自动先校验本地输入，再加载 `sdkconfig.defaults` 与本地文件，并校验最终配置。缺失或非法输入即非零退出。当前Group 2使用生产C校验编译密码，非法时阻塞AP/socket但保留独立SPI启动路径；入口执行固定池及NVS/netif/event初始化，并明确报告尚未实现的服务。SoftAP、socket、SPI完整功能由对应后续阶段交付。

Group 3的正式入口现为 `scripts/activate.ps1` 和 `scripts/build.ps1`，步骤见 [中文说明](../Docs/guides/build-and-flash.md)。当前未授权烧录。密码会进入本地生成配置和固件，勿公开这些文件。
