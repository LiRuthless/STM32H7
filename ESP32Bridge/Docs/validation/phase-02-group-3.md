# Phase 02 Group 3：固定本端候选向量与阶段交审

> 历史证据说明（2026-11-05）：本文按原日期/镜像/环境保存；旧“本轮/当前”不作为现行进度。现行状态见[roadmap](../../specs/roadmap.md)，方法见[验证指南](../guides/validation.md)。本次未重跑本文检查；本副本缺原artifacts/.local时仅保留历史报告与JSON，不声称重新核验产物。

审查更新：用户明确确认“确认提交并推进 Phase 03 独立实现，保留三端和实机待验”。最终检查后提交本组，下文待审为原交付记录。

2026-10-18，分支`codex/02-scb-protocol`。Group 2经用户明确确认提交`a0973a0`；Group 3工具/清单细化先纯规约提交`0154d9c`再实施。本组代码待审查、未提交。

## 交付与验证

- [固定文件](../../tests/vectors/scb-v1.json)包含14条完整hex、全部字段期望、负载、CRC、预期错误及逐帧SHA256；[索引与三端表](../../tests/vectors/scb-v1.md)列操作与核对状态。
- [生成器](../../scripts/generate_vectors.py)使用struct/zlib确定性生成，`--check`只比较，不替换基线。
- [独立核验器](../../scripts/check_vectors.py)按字节偏移解析、逐位CRC及SHA256核对，不导入生成器/生产codec，不使用struct/zlib。
- [生产C测试](../../tests/test_vectors.py)调用真实codec与可靠接口，核对14条SPI/TCP向量（含错CRC/长度）、可靠重复只分发一次，相同request_id新seq及冲突内容继续透传给STM32。

固定JSON的SHA256为`3477fdc53254f24b2fdcd719a8131ac8e344dc197c26fb12107a2b22f38668ee`。独立校验结果12条OK、1条CRC错误、1条长度错误，均符合冻结期望。**这是本端候选向量，STM32/电脑端尚未核对，G-04仍待验。** 不把STM32负责的请求缓存/INVALID_ARG当作ESP32已执行业务。

本次仓库根目录实际命令：

```powershell
. .\ESP32Bridge\scripts\activate.ps1
python ESP32Bridge/scripts/generate_vectors.py --check
python ESP32Bridge/scripts/check_vectors.py --report ESP32Bridge/.local/p2-g3-vector-audit.json
python -m unittest discover -s ESP32Bridge/tests -p test_vectors.py -v
python -m unittest discover -s ESP32Bridge/tests -v
```

独立校验通过，新增6项通过，**全量145项通过，0失败/0跳过**，日志`.local/p2-g3-regression.log`。使用同一核验后的v6.0.0/Python3.11.15环境，生产C仍由现有Clang/LLD编译执行；不安装替代工具。

本组没有修改生产C或目标CMake，目标构建证据引用上一组本次实际成功的run_id `8939b9db003c483d86c48fb07a99d0cb`，见[Group 2证据](phase-02-group-2-evidence.json)。**本组未重新运行目标构建，不把引用写成新构建。** 最终app仍只有基础服务，未接入真实SPI/socket，不能用候选向量通过替代生产路径完成。

## 文件、拟提交与后续

新增：scripts/generate_vectors.py、scripts/check_vectors.py、tests/test_vectors.py、tests/vectors/scb-v1.json、tests/vectors/scb-v1.md、本报告及[证据JSON](phase-02-group-3-evidence.json)；同步README、AGENTS、Phase 02三件套、roadmap、techstack、mission、CHANGELOG与traceability。拟提交：`test(esp32bridge): 建立 SCB/1 固定候选向量与独立核验`。

Phase 02本端codec/JSON/可靠/会话/候选向量均已实现并完成对应主机验证；真实SPI/socket适配属于Phase 03～05，三端一致和实机验收未完成，阶段总完成不打勾。G-01/G-02/G-03方案已确认，不再因旧门禁停工；G-04三端对齐与G-05实机能力继续保留。

按逐组/逐阶段审查要求，本组及Phase 02本端软件交付交用户审查。确认后提交本组，并推进Phase 03可独立实现的真实SPI驱动任务/适配；真实三端验收继续待验。当前尚无Phase 03～06实现、60秒负载、客户端、电气夹具或最终中文实机手册，不将本组当成用户完整目标已完成。
