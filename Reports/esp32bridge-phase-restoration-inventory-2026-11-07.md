# ESP32Bridge Phase恢复清单

基线：441a77843ba765cf58dbcdda4d0d55ad2e4f5557。恢复来源为当前文件内五波浪线Markdown围栏正文；不是检出35e5627或SmartCar历史。哈希为移出包装前正文UTF-8/LF的SHA256（不含围栏首尾换行）。原正文提升为主体，旧FR/TC显式锚点保留；正文原链接的docs大小写修正为实际Docs。后续11月5日增量独立保留。

|文件|正文行数|原正文SHA256|后续整改增量|
|---|---:|---|---|
|`ESP32Bridge/specs/features/01-project-foundation/group-2-design.md`|82|`c4c1a2c59361fd1e6c74806243ddf6bf4814aceb0d0a3d7bff274e80ea40959b`|无独立整改段|
|`ESP32Bridge/specs/features/01-project-foundation/plan.md`|45|`bfa45a1ae7bc0558ec379d3b68069ff033310cd6ffdd027cc38a0a9c2c4a1e8e`|保留|
|`ESP32Bridge/specs/features/01-project-foundation/requirements.md`|36|`86ac90c5945d47f477709e8daee96b5094fb7a6bddacace1da19578b67d82af9`|保留|
|`ESP32Bridge/specs/features/02-scb-protocol/g-02-session-change.md`|21|`587f5357bac0e8ef0361ecbd8784b9fa24505b8e1d901b392e7fb6b779d0e2d1`|无独立整改段|
|`ESP32Bridge/specs/features/02-scb-protocol/group-2-design.md`|20|`c0a8a4d54b652e898a23f079b2478cc9b9b3d772841bc2379dd68b13fac0e08d`|无独立整改段|
|`ESP32Bridge/specs/features/02-scb-protocol/plan.md`|58|`84ffb1f59fdd105f650c5f806c76529aa430f56213c55d2d2506271c81e193e7`|保留|
|`ESP32Bridge/specs/features/02-scb-protocol/requirements.md`|33|`832e5a24515a79b21044c532988027d61724d282f2e51f8be811298415939d29`|保留|
|`ESP32Bridge/specs/features/03-spi-slave/g-06-task-json.md`|16|`3c2d7221b5c1c4a84e7d9c1ac1b1f8160c03e222089020e4422c4bd826572a6e`|无独立整改段|
|`ESP32Bridge/specs/features/03-spi-slave/group-1-design.md`|23|`18db1f664258400e5980ff09afe6da36cacd746e52ee2146c40ddf76bf11c088`|无独立整改段|
|`ESP32Bridge/specs/features/03-spi-slave/lifecycle-design.md`|35|`806a74c894073181edcb7dcf2462689c05f830bce8fce076b4b20706c83f4373`|无独立整改段|
|`ESP32Bridge/specs/features/03-spi-slave/plan.md`|49|`13084d4a1d3e2667b4748a1d8a06893ec1000648897ddd6375eed6d364bf6f4b`|保留|
|`ESP32Bridge/specs/features/03-spi-slave/requirements.md`|24|`4c891d8f03709f1836be82eb464a73d19ca721a695e037b1a848ba733eb2d128`|保留|
|`ESP32Bridge/specs/features/04-network-services/g-01-busy.md`|17|`9094bb6a98e222661fd05d2bf6d85f3ffbe9f429598c6990b749cbec42e7012e`|无独立整改段|
|`ESP32Bridge/specs/features/04-network-services/network-design.md`|41|`3546d165f344edfadc45fec440db040913b796e90a562e779bb85708b17645d0`|无独立整改段|
|`ESP32Bridge/specs/features/04-network-services/plan.md`|53|`cdc83be473c1fc2148eebe57f5db76053f16d2a7b449f3cf050e492d34976f12`|保留|
|`ESP32Bridge/specs/features/04-network-services/requirements.md`|26|`7d761383a51845c24190ee829736b3b01862cda6eab5826adfc8d8d33567ff43`|保留|
|`ESP32Bridge/specs/features/05-bridge-lifecycle/g-03-event-gap.md`|17|`7ac378b1459c7acd6cf23d8e153412ace1c60d57841938f9c8aa0c294660d4cb`|无独立整改段|
|`ESP32Bridge/specs/features/05-bridge-lifecycle/lifecycle-design.md`|48|`21c00c9e92b5349007b8fdf841cc24b3454d4f774e5aaa372603004ed1caa3ea`|无独立整改段|
|`ESP32Bridge/specs/features/05-bridge-lifecycle/plan.md`|58|`d57b798ac11198c2f4968c75533798ec1382fed372ff0b25b04040c9771a1ffa`|无独立整改段|
|`ESP32Bridge/specs/features/05-bridge-lifecycle/r6-log-counters.md`|30|`5240a1ec7662384030a6b37bbf0a7d5bb3270e7141622805e2acf35b69d9dbd6`|无独立整改段|
|`ESP32Bridge/specs/features/05-bridge-lifecycle/requirements.md`|27|`8104837ca0ba56764d365c9db5226654127eb3bb0ff2292aee96d7a2d49c5cb3`|无独立整改段|
|`ESP32Bridge/specs/features/06-validation-delivery/plan.md`|44|`dd8ea505dd996a43ce1fe2bc10206a792e1448b8f36dbadb54cf986313ab21d4`|无独立整改段|
|`ESP32Bridge/specs/features/06-validation-delivery/requirements.md`|22|`9e2ecf7a2121fcb5fd641ffd5e3684d5ba20278b45175aff092d821ee6573ff4`|无独立整改段|

## 实际核对与保留

- 6份requirements、6份plan及11份附属设计，共23份解包；6份validation按真实内容核对，未机械解包。18份三件套全部保留。
- 对照441a778各文件内原冻结正文逐非空行核对：23份原文零缺行；仅将原docs路径大小写修正为Docs，并修复Phase01旧构建指南路径到Docs/guides/build-and-flash.md。显式FR/TC锚点插入原需求处；无编号重排。原代码围栏及原任务勾选保留。
- Phase01～04 requirements/plan内8段11月5日后续源码整改增量保留。validation四个整改准备段改注明实施前时点；最终187项结果和目标S5未验证不改。Phase05/06导入前“未跟踪工程”时点改注为导入前历史，保留原产物缺失与未验声明。
- 29份Markdown跳过代码围栏后核对本地文件路径，修复上述两处旧路径后无缺失；章节锚点交主执行者全局核对。
- Phase05未来设计由本地完整原文恢复，原Docs目标的独有run边界、事件锚点字段、LOG块范围和独立无时钟计时边界归入lifecycle-design；批准冲突、计时分歧、缺失模块及跨任务风险明确并列。
- 六份validation均新增2026-11-07适用说明，不覆盖原日期、计数、失败、待审、构建及实机结果；本次未运行测试或硬件。
