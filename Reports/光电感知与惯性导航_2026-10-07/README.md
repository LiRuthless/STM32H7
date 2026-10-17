# 光电感知与惯性导航 技术报告交付

- `光电感知与惯性导航_技术报告.html`：离线报告，含公式、比较表、静态流程图/思维导图、引用、完整代码。浏览器打开即可阅读，可打印为 PDF。
- `report_full.md`：相同内容的 Markdown 版；配套 SVG 在同目录。
- 数学排版于 2026-10-09 更新：HTML 中全部公式已静态排版，并嵌入数学字体，可离线阅读；Markdown 使用 LaTeX 数学语法，需要支持数学公式的阅读器。
- `code/optical.py`：光电预处理、质心、差比和、PID 与差速指令。
- `code/inertial.py`：六轴姿态、重力补偿/双积分、轮式相对里程计与条件修正。
- `code/demo.py`：101 帧人工输入、CSV/JSON 输出示例。
- `code/test_algorithms.py`：公式算例、物理不变量与关键异常验证。
- `code/output/`：已经运行生成的示例记录及元数据。
- `review.md`、`validation.txt`：独立复核与实际运行验证记录。

Python 3.10 及以上，标准库即可。在解压后的 code 目录运行：

```powershell
python -m unittest -v test_algorithms.py
python demo.py
```

本交付为通用算法示例，不是已经接入具体 STM32 工程的固件。硬件接入、主控最坏执行时间、真实车速、PID 稳定裕度、IMU 温漂与轨迹精度尚须实测。原始人工记录不表示物理仿真闭环或真实车辆采集数据。

HTML 数学排版使用 KaTeX 0.16.22，附 MIT 许可证于 `vendor/katex/LICENSE`。阅读最终 HTML 无需安装 KaTeX 或加载网络字体。
