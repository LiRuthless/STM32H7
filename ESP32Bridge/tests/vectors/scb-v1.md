# SCB/1 本端候选向量

**状态：本端候选，尚未与 STM32/电脑端核对。** 不作为三端已一致的证明。

固定文件：[scb-v1.json](scb-v1.json)，SHA256：`3477fdc53254f24b2fdcd719a8131ac8e344dc197c26fb12107a2b22f38668ee`。

JSON包含每帧完整hex、SHA256、全部字段值、负载hex、存储及独立计算CRC、预期拒绝原因；所有普通会话为0x12345678，握手为0。TCP帧无填充；SPI正向验证补零到1024B，长度错误向量保持截短。

| ID | TCP字节数 | 期望codec结果 | SHA256 |
|---|---:|---|---|
| hello-stm32 | 61 | OK | `97b6760a36bd1584281668bb697d4db6a1af3cbbe8e8969c59f94153f7667060` |
| hello-pc | 58 | OK | `a954f25addd7d90eafece3414d3e18bfb20cf157237a56707522c75326f8f125` |
| hello-ack | 88 | OK | `1c578021e1343bea817762dd68c9af43251c0d9e3f4e4cd1d17262a0442ddd34` |
| idle | 32 | OK | `fea601d37607cf22f4a90bd56e2a96b1ded87b5231f98e3db7923e5da568c1e9` |
| get-status | 61 | OK | `f8a6cc2f84dafe5fefb1366cd1886855765c4a765294875301110cc70d67d3fb` |
| set-params | 192 | OK | `d94881c94bf2af98d14cdbd3bff830ddc14eea197a4aea1f6f094964a5a7e65d` |
| telemetry-1 | 152 | OK | `dc37eb8fbe3e9833d62a4661a71ffb699157011952c9130e2048e49a2de95e06` |
| telemetry-10 | 1016 | OK | `a0146fd7d770e43ed89ae62add50a84ce91045c140fd5dfdb3d8543b15440c20` |
| log-data | 56 | OK | `89e3d2249a60de97ef6168554df3d58a878103fd464a745b3487cf3ef42108f5` |
| bad-crc | 61 | CRC | `863458eef7467a1713b1835487767fdb12106b120d31c32e0ddd732300571bd8` |
| bad-length | 61 | LENGTH | `5e2b91da80aed1c77d6501b4840ace2035bb444d4e713acaa6a9e16c170344e9` |
| reliable-repeat | 61 | OK | `f8a6cc2f84dafe5fefb1366cd1886855765c4a765294875301110cc70d67d3fb` |
| request-id-repeat | 192 | OK | `f541c8fa794ace4b9cdf5e3be9e9af4e3bffddd27e124d78e21de46ea3eb9884` |
| request-id-conflict | 192 | OK | `711e2f0dd1528a6960ce1604e8eb5283b1947433720bd68ca13205cbfb9f7f95` |

## 操作（仓库根目录，已激活v6.0.0环境）

```powershell
python ESP32Bridge/scripts/generate_vectors.py --check
python ESP32Bridge/scripts/check_vectors.py --report ESP32Bridge/.local/vector-audit.json
python -m unittest discover -s ESP32Bridge/tests -p test_vectors.py -v
```

生成器使用struct/zlib；独立校验器只用显式字节偏移与逐位CRC；测试还直接调用生产C codec及可靠接口。`--check`不覆盖文件；修改向量必须重新审查，不能自行换一组输入使测试通过。

可靠重复帧与GET_STATUS字节完全相同，只分发一次。相同request_id的新seq以及冲突内容均由ESP32继续转交，STM32负责业务缓存/INVALID_ARG；没有用ESP32伪造结果。遥测1/10条为152/1016B，记录reserved全0。LOG_DATA含16B已知数据，不代表真实Flash内容。

## 三端核对表

| 端 | 源码提交 | 固定文件SHA256 | codec/可靠测试 | 实机 |
|---|---|---|---|---|
| ESP32Bridge | 当前Group 3工作区；先行规约0154d9c | 上述固定值 | 本端通过，见交审报告 | 未执行 |
| STM32 | 待填写 | 待核对 | 未执行 | 未执行 |
| 电脑端 | 待填写 | 待核对 | 未执行 | 未执行 |

三端各自保留同一固定文件并独立复核后，由人工记录具体提交、命令与证据；在此之前G-04保持待验。软件独立验证不等于电气SPI/网络/车辆联调。
