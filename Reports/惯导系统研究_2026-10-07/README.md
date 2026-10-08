# 惯导系统研究报告交付说明

主报告：[中文Markdown](report.md)；[离线HTML](报告.html)。

HTML的正文、公式、CSS、数学字体和SVG均已内嵌，复制单个HTML即可离线阅读；参考来源链接需要联网。页面提供目录与打印按钮，打印采用A4样式。2026-10-08更新：公式改为标准LaTeX源，由随附KaTeX 0.19.0在构建时排版，包含正确上下标、分式、矩阵、估计帽号和公式编号。阅读HTML不需要联网或运行客户端数学脚本。

## 配套文件

- `sources.md` / `sources.html`：文献与案例索引、全文访问状态、原文定位、代码固定版本。
- `coverage.md` / `coverage.html`：16项内容、对应章节、来源与证据缺口。
- `source_manifest.json`：实际公开PDF下载状态、SHA-256、页数、GitHub commit和文件哈希。失败下载不等于未阅读，另列网页全文读取状态。
- `research_notes.md`：本次检索/阅读/运行范围及基线记录。
- `figures/`：四幅原理图、两幅已运行数学/仿真曲线，均为离线SVG。
- `data/`：数值算例JSON和两次简化转弯仿真的完整CSV。
- `tools/`：本报告自编的资料获取、数学算例和HTML构建程序。
- `assets/katex/`：固定版本排版引擎、离线字体、许可证与下载哈希记录。
- `review.md`与`qa/`：交付检查、视觉核对与离线检查结果。

本报告不包含他人论文全文，不包含固件实现，也没有声称运行作者的导航代码或完成实车测试。自设算例仅验证相应简化模型。

## 重新运行

数值算例仅需Python 3标准库：运行`python tools/numerical_examples.py`，会重建data及两幅曲线。

HTML构建需Node.js及marked包：运行`node tools/build_report.cjs`。本机可用的Node/Python和包目录由桌面运行时提供，构建工具记录了本机回退路径；换机器需安装对应依赖或指定环境。

四幅原理图可由`python tools/diagrams.py`重建。修改独立索引后先运行`python tools/finalize_markdown.py`更新完整报告，再构建HTML；`REPORT_NODE_MODULES`环境变量可指定marked/playwright的包目录。`tools/audit_report.py`做文档检查，`tools/qa_html.cjs`使用本机Chrome进行离线浏览器截图和版面检查。

数学排版源码写在`report.md`的`$$...$$`独立公式与`$...$`行内公式中。构建程序还同步更新四幅SVG中的公式标签，并把字体嵌入独立SVG。`convert_inline_math.py`是本次一次性迁移工具，重复运行不会重复转换。最新排版截图与浏览器检查位于`qa/math_2026-10-08/`；旧`qa/`首层记录保留作为此前版本记录。

公开资料获取程序需要联网及`pypdf`：先运行`tools/fetch_sources.py`，再运行`tools/fetch_supplement.py`。它们只写临时阅读缓存和版本清单；网页读取成功但本地下载失败的资料需要从索引链接另行打开。再次下载当下分支可能更新commit，复查报告应优先使用已记录固定链接。
