# Phase 正文恢复核对记录（2026-11-07）

调查基线：`8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e`；原文依据：`68cb5088846f2cb2b24026180d240dcdb07f762b`。本报告只记录纯文档恢复，不新增固件/实机通过结论。

## 实际枚举与恢复方法

实际枚举为 51 份三件套、Phase15 四份专项、Phase17 六份设计，共 61 份折叠正文，另恢复 Phase16 协议，共 62 文件。61 份折叠正文以 CRLF→LF 及首尾空白归一后逐字等于 Git 原文；恢复时保持原正文连续可读，不批量改写历史路径/符号。删除 10 月 31 日的分流独占声明、失效历史包装与折叠；现行导航移到文末。Phase16 原文从 Git 单独取回。

Phase02 三件套在原折叠区后新增的内容原样保留：FR-27～30、五组任务、板测验收及最终 R08 记录。`board-test-review.md`、全部 JSON/日志/源码/配置均未编辑；已发生提交顺序不变。阶段结束规则前瞻适用，不搬迁11月增量。

下表 SHA-256 是对归一文本计算，只用于这次正文与增量保持性核对，不替代原始证据摘要。每行“原文一致”均在修改前比较，修改后再次检查完整原文前缀及增量包含关系。

| 文件 | 10月31日处理及本次恢复位置 | 原文一致 / SHA-256 | 后续增量 SHA-256 | 保留稳定FR/TC锚点数 |
|---|---|---|---|---|
| [01-runtime-architecture/plan.md](../SmartCar/specs/features/01-runtime-architecture/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `e1096da9564f35e37d8dbd51a111088048740d2bc0b90e62f09b22db9e92f63b` | 无 | 0 |
| [01-runtime-architecture/requirements.md](../SmartCar/specs/features/01-runtime-architecture/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `2f6d3f1385a305af3e8b234298bfbb456efed744de437f866397041ca5f75903` | 无 | 21 |
| [01-runtime-architecture/validation.md](../SmartCar/specs/features/01-runtime-architecture/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `71d12ce0116f7a81d046f74d5576def001f652c155cbd9226fe91171f9ed78e5` | 无 | 0 |
| [02-bsp-platform/plan.md](../SmartCar/specs/features/02-bsp-platform/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `51b838951f4bc048d774b9d564eca68947648f3984bf5d15bab6555f1c88951e` | `7a5c16fd02da16d5e91c8b43dd7eec16819a380db8d9b579f033359f2ac17d3a` | 0 |
| [02-bsp-platform/requirements.md](../SmartCar/specs/features/02-bsp-platform/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `0eef944e5bd5ee681580c446faf90446a46f93f16d258030d77084c33887c0cb` | `e84d8a541bf36bd15f8f3d28d51407c93153f532039963d2cc5d5d70c55fd77c` | 33 |
| [02-bsp-platform/validation.md](../SmartCar/specs/features/02-bsp-platform/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `8954cbd1f5e5815047a2f92b3edefde941ba338845d2f88dcc86711080b690c5` | `d324403b17710842bfd1181a0573abe6369eb611992ff00b96d02e9b2a202e3d` | 0 |
| [03-track-following/plan.md](../SmartCar/specs/features/03-track-following/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `c18ccc87426507203356235b02dbaf9706be5d0eeb1f615e3009219dd0ee4dd6` | 无 | 0 |
| [03-track-following/requirements.md](../SmartCar/specs/features/03-track-following/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `b860dc585b1e5074803ef616862fd79b6d66fbf8144e13f63ebe132bbf00704b` | 无 | 17 |
| [03-track-following/validation.md](../SmartCar/specs/features/03-track-following/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `7684b7f5645b3c95fea1ede29c3936e520b2569b51f441e86c4ae0e719d384e8` | 无 | 0 |
| [04-element-fsm/plan.md](../SmartCar/specs/features/04-element-fsm/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `eab79a45eefb2656bac1a3ecd359017478d5b6c5111cc50454ade9a92e7a93d9` | 无 | 0 |
| [04-element-fsm/requirements.md](../SmartCar/specs/features/04-element-fsm/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `c7fdaf5976baa7e934b1f07f556a45541178babaf34007b57feb659fe481a6d3` | 无 | 16 |
| [04-element-fsm/validation.md](../SmartCar/specs/features/04-element-fsm/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `02ea0aba01b88ef56dc00eac63f227077047801cf6334ad0748d4fde98d3ec2e` | 无 | 0 |
| [05-imu-attitude/plan.md](../SmartCar/specs/features/05-imu-attitude/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `ac17b49efb87ace22387bd0cb3dad195ffec968e2ff4da12f4df75202dd76fbe` | 无 | 0 |
| [05-imu-attitude/requirements.md](../SmartCar/specs/features/05-imu-attitude/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `0860acb281dd5091e8d1c512ee87463661c5fc8a028793a9daa44b5a3c4dbcfb` | 无 | 13 |
| [05-imu-attitude/validation.md](../SmartCar/specs/features/05-imu-attitude/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `8a6ba7eafe899ade1847d6b25c2679ce68ed7ae051f4eac2f5460b05c2ee928a` | 无 | 0 |
| [06-hmi-tuning/plan.md](../SmartCar/specs/features/06-hmi-tuning/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `37c4b1c786849e061038376f9e7d8d7c8095df767a0816f4a3c7cbfee41583de` | 无 | 0 |
| [06-hmi-tuning/requirements.md](../SmartCar/specs/features/06-hmi-tuning/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `b4315c9b934faaf9b917797599bc9c6feb562924bc42bc78258158f9fa95a327` | 无 | 24 |
| [06-hmi-tuning/validation.md](../SmartCar/specs/features/06-hmi-tuning/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `841b897a863430e75b15aabfec8776ca7a1a73e469b229d7d69a8e5f35e43bfc` | 无 | 0 |
| [07-datalog/plan.md](../SmartCar/specs/features/07-datalog/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `5bcd5a4b284ac4944f73a4e3fa3fb3cc6c83e8da0494e84629fdbb336a4b61d2` | 无 | 0 |
| [07-datalog/requirements.md](../SmartCar/specs/features/07-datalog/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `b84fb9a3535fbdc5cdce327d985f6a2c62f1efd6693c5d7ed8afb89e34b80c89` | 无 | 14 |
| [07-datalog/validation.md](../SmartCar/specs/features/07-datalog/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `2dbb138620f6511ae154a629ebb1a84c24d96e5e25869c6c408769ce3d1e8bd7` | 无 | 0 |
| [08-storage-chain-fix/plan.md](../SmartCar/specs/features/08-storage-chain-fix/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `38533733fd5bcae4e069cb03ed32b008036c7b2d1a83117c14da173c67d15026` | 无 | 0 |
| [08-storage-chain-fix/requirements.md](../SmartCar/specs/features/08-storage-chain-fix/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `6cdbdd6b83e8e649b38a9c83fdb426c9653f4943a6f3936ab5d21725e92c0454` | 无 | 8 |
| [08-storage-chain-fix/validation.md](../SmartCar/specs/features/08-storage-chain-fix/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `db13ce314cf98409f3067aee5bb5c8240975d8e9f2051500333856b9cbdd9ed1` | 无 | 0 |
| [09-element-completion/plan.md](../SmartCar/specs/features/09-element-completion/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `0885ab42e16d50752b80ac49a44514aed260289ec3deab5d92d5f7cfffef2c38` | 无 | 0 |
| [09-element-completion/requirements.md](../SmartCar/specs/features/09-element-completion/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `53d89a2e7ad3e771bc4232b11aad5cdfe0552ae34c3db5c5e76b212ee84a44cb` | 无 | 10 |
| [09-element-completion/validation.md](../SmartCar/specs/features/09-element-completion/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `fbfb47ecebe281e1744cdfa5c09d32dcb8a12a871d3e3a8208c7af5570098bb9` | 无 | 0 |
| [10-control-consistency/plan.md](../SmartCar/specs/features/10-control-consistency/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `842df09bd4e992bf5d046d71e77a93dae98b6820b212a08358940808dd0f3398` | 无 | 0 |
| [10-control-consistency/requirements.md](../SmartCar/specs/features/10-control-consistency/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `2384bd1c704fdfbb2fe0b02e52ae5237e077a09a7827f8a36f9a704f5ecf9436` | 无 | 10 |
| [10-control-consistency/validation.md](../SmartCar/specs/features/10-control-consistency/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `e9bc7d84aa75662ffb65bb5fd2a23f8b6cfad15271ff39b9939ceef8700b8459` | 无 | 0 |
| [11-hardening-cleanup/plan.md](../SmartCar/specs/features/11-hardening-cleanup/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `4e53641b63e15e170d22b82b007ff03f5714c07d98179c5ce7a5bb76daef417f` | 无 | 0 |
| [11-hardening-cleanup/requirements.md](../SmartCar/specs/features/11-hardening-cleanup/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `97de54e1da255f7906dd4dfeaba961b3e08ba24134f9d3a5d651e2e66a288cae` | 无 | 15 |
| [11-hardening-cleanup/validation.md](../SmartCar/specs/features/11-hardening-cleanup/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `d1f8dbad6ba653909226933567695d0bd06b0c0c3fe1e0ee23c96e46d2e4433c` | 无 | 0 |
| [12-control-safety-closure/plan.md](../SmartCar/specs/features/12-control-safety-closure/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `2b124ff95e32f06828d2a2ac7a844b9eb48000e9930efded514c084798a456b2` | 无 | 0 |
| [12-control-safety-closure/requirements.md](../SmartCar/specs/features/12-control-safety-closure/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `57918d57aba1c190b4054e2c51904f64de665c91c05922bc71b5af903a02721a` | 无 | 12 |
| [12-control-safety-closure/validation.md](../SmartCar/specs/features/12-control-safety-closure/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `fa41ec2f2faa81f9c2f0c004f85c01fc8b99f4c40bf34d715e112a3bea333b68` | 无 | 0 |
| [13-control-structure/plan.md](../SmartCar/specs/features/13-control-structure/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `fa3096e9623207c1f5f2120aaddadf3d7e8ca35ae7066f4a83f59947d775c5a8` | 无 | 0 |
| [13-control-structure/requirements.md](../SmartCar/specs/features/13-control-structure/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `e5a1935d8da88a43b219ca6c6fdaefbdab518a01f3fdd3a1846e371eef191e7a` | 无 | 13 |
| [13-control-structure/validation.md](../SmartCar/specs/features/13-control-structure/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `318fb6ffb221081cecd8225ebda58ceac352fbe8c576492355c99e5a1c5d6a41` | 无 | 0 |
| [14-dual-mode-tracking/plan.md](../SmartCar/specs/features/14-dual-mode-tracking/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `991686351579c548ef884bcf88eabc239a674b5ed268017d723d1e66abe00c67` | 无 | 0 |
| [14-dual-mode-tracking/requirements.md](../SmartCar/specs/features/14-dual-mode-tracking/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `1c35d62f70942d15e43b543d9f99369d004f7cdb6793eb61a8c216906c4b7eb0` | 无 | 22 |
| [14-dual-mode-tracking/validation.md](../SmartCar/specs/features/14-dual-mode-tracking/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `e0bc0eaced5299b8b716e148d8523ddcd24c42f9136de52215d6eec21c2c7e99` | 无 | 0 |
| [15-code-style-consistency/comment-review-workflow.md](../SmartCar/specs/features/15-code-style-consistency/comment-review-workflow.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `6ff32484bc99a50de0b71bbc20a30232033bd0b32970f2465b325e6fe57d7079` | 无 | 0 |
| [15-code-style-consistency/comment-review.md](../SmartCar/specs/features/15-code-style-consistency/comment-review.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `33c44a7ac50e6c41d2f23d7c93116f6034333f00469dbeaeaf543e88cba4784c` | 无 | 0 |
| [15-code-style-consistency/plan.md](../SmartCar/specs/features/15-code-style-consistency/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `cf844f42ad3fb5bd5cf35743559168bb7f07ccc6622fae05dc8428b3a30fbd2e` | 无 | 0 |
| [15-code-style-consistency/r08-review-contract.md](../SmartCar/specs/features/15-code-style-consistency/r08-review-contract.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `d40d207255c82e8513dbe86458db2f84469fe4ce66602769131e08a67d2a21f3` | 无 | 0 |
| [15-code-style-consistency/requirements.md](../SmartCar/specs/features/15-code-style-consistency/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `524ed160b9e690155de8a7ca3a8689c1119442c0951ff138b9dcc3c58d8e4e96` | 无 | 25 |
| [15-code-style-consistency/symbol-map.md](../SmartCar/specs/features/15-code-style-consistency/symbol-map.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `b9a7510b91c486bb6fac413fc0277f97d9510aba14dfe0ff68c317463b0ed2e1` | 无 | 0 |
| [15-code-style-consistency/validation.md](../SmartCar/specs/features/15-code-style-consistency/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `07f9ff1db69e7d5b4974d00cc549be17622ab02580cfeed169c72ed5f752d177` | 无 | 0 |
| [16-spi-wifi-link/plan.md](../SmartCar/specs/features/16-spi-wifi-link/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `2429027b63a403323c55e694ad021555c0791bec3840085a0b1cba314d0a57c5` | 无 | 0 |
| [16-spi-wifi-link/requirements.md](../SmartCar/specs/features/16-spi-wifi-link/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `068b4be9651152d931dacb26ad818b59d4b1093b4123a994480e2327a310beda` | 无 | 24 |
| [16-spi-wifi-link/validation.md](../SmartCar/specs/features/16-spi-wifi-link/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `7d12f065292a8579ed39893d195c91269c798d4ea4332207cc7ed6408d04b83f` | 无 | 0 |
| [17-app-architecture/architecture.md](../SmartCar/specs/features/17-app-architecture/architecture.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `782de6a0ddddc2dd940859555020641c1b39009a5565d362356ce8a7946f2b1d` | 无 | 0 |
| [17-app-architecture/g2-system.md](../SmartCar/specs/features/17-app-architecture/g2-system.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `5b7f07d32440dc01c44d73efb3869278d9fa6bd7414944c94c13278bd860526a` | 无 | 0 |
| [17-app-architecture/g3-control.md](../SmartCar/specs/features/17-app-architecture/g3-control.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `94c8c20e3ddb80e70f1bf0d5aa48f83b93c42e6e0b832e0b350b3ba7adc1d990` | 无 | 0 |
| [17-app-architecture/g4-parameters-hmi.md](../SmartCar/specs/features/17-app-architecture/g4-parameters-hmi.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `c62d013aa585e893c93dc78062bbd66dc0d175a4cd9430fe05539455855ccce2` | 无 | 0 |
| [17-app-architecture/g5-recording-protocol.md](../SmartCar/specs/features/17-app-architecture/g5-recording-protocol.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `c5022473c20e6e358c3b38c171612b05460912167302d673ac14d8e9b8fd309b` | 无 | 0 |
| [17-app-architecture/g6-acceptance.md](../SmartCar/specs/features/17-app-architecture/g6-acceptance.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `6ea98c08faa1b368a5a351fe93c11d457335fd269bf4d7bbfa7ad81e15046aec` | 无 | 0 |
| [17-app-architecture/plan.md](../SmartCar/specs/features/17-app-architecture/plan.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `81a23c5f1de8f87d16cce6416c5151d796bc1edc009a3fc9b4e9ed5524583c52` | 无 | 0 |
| [17-app-architecture/requirements.md](../SmartCar/specs/features/17-app-architecture/requirements.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `288f6248316849ddc57f4773820e10ab636b8e64bf4f6839a73c89112f4e4485` | 无 | 13 |
| [17-app-architecture/validation.md](../SmartCar/specs/features/17-app-architecture/validation.md) | 原正文被折叠；恢复为文件主体；现行导航置后 | 是 / `669d060ccbd9e99c56e1b336dc851d41c592eaf278ec143e2d1e37a2c8b0578c` | 无 | 0 |
| [Phase16协议](../SmartCar/specs/features/16-spi-wifi-link/protocol.md) | 原文被跳转页替代；从Git恢复完整快照 | 是 / `30dfa2057662a9d9edea87025a976c5e9d5b43e764210cb4185f11cbe0328e82` | 无 | 13个章节锚点 |

## Phase16 兼容锚点映射

现存协议入口实际有 **13** 个显式锚点（调查口头初报14有误，以实际枚举为准），全部原位对应原章节，不回跳Docs替代原文。

| 旧锚点 | 恢复后具体章节 |
|---|---|
| [`#phase-16--scb1-公共协议快照`](../SmartCar/specs/features/16-spi-wifi-link/protocol.md#phase-16--scb1-公共协议快照) | Phase 16 — SCB/1 公共协议快照 |
| [`#公共通信协议scb1v1三端必须一致`](../SmartCar/specs/features/16-spi-wifi-link/protocol.md#公共通信协议scb1v1三端必须一致) | 公共通信协议：SCB/1（v1，三端必须一致） |
| [`#1-物理连接与网络`](../SmartCar/specs/features/16-spi-wifi-link/protocol.md#1-物理连接与网络) | 1. 物理连接与网络 |
| [`#2-帧头与编码`](../SmartCar/specs/features/16-spi-wifi-link/protocol.md#2-帧头与编码) | 2. 帧头与编码 |
| [`#3-消息确认与会话`](../SmartCar/specs/features/16-spi-wifi-link/protocol.md#3-消息确认与会话) | 3. 消息、确认与会话 |
| [`#4-json-命令与结果`](../SmartCar/specs/features/16-spi-wifi-link/protocol.md#4-json-命令与结果) | 4. JSON 命令与结果 |
| [`#5-telemetry-与-96b-记录`](../SmartCar/specs/features/16-spi-wifi-link/protocol.md#5-telemetry-与-96b-记录) | 5. TELEMETRY 与 96B 记录 |
| [`#6-log_data事件与错误边界`](../SmartCar/specs/features/16-spi-wifi-link/protocol.md#6-log_data事件与错误边界) | 6. LOG_DATA、事件与错误边界 |
| [`#7-共同测试向量与集成`](../SmartCar/specs/features/16-spi-wifi-link/protocol.md#7-共同测试向量与集成) | 7. 共同测试向量与集成 |
| [`#stm32-适配说明不属于上方公共原文`](../SmartCar/specs/features/16-spi-wifi-link/protocol.md#stm32-适配说明不属于上方公共原文) | STM32 适配说明（不属于上方公共原文） |
| [`#断开与导出释放`](../SmartCar/specs/features/16-spi-wifi-link/protocol.md#断开与导出释放) | 断开与导出释放 |
| [`#既有电脑字段与结果封装参考`](../SmartCar/specs/features/16-spi-wifi-link/protocol.md#既有电脑字段与结果封装参考) | 既有电脑字段与结果封装参考 |
| [`#固定向量`](../SmartCar/specs/features/16-spi-wifi-link/protocol.md#固定向量) | 固定向量 |

## 本组实际检查与限制

- 61份原文恢复前后均逐字核对通过（仅归一行尾和首尾空白）；没有改写原批准、错误记录、待验项或历史数字。
- Phase02三份后续增量按完整文本比较通过，FR-27～30及人工审查链接保留；人工表不变。
- Phase16去除新增锚点和末尾恢复说明后，与原协议正文一致；字段、测试向量、适配说明均在。
- 所有原显式需求锚点保留；旧需求导航仍指向现行功能入口，主题拆分及全局追溯由G3/G4统一更新。
- 未运行构建、软件行为回归、固件R08或板测；本组没有新的设计决定。全仓链接、当前Docs语义和最终范围核对由G5统一验收。
