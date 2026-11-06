<!-- ppt-master-schema: spec-lock/v1 -->
# Execution Lock

## canvas
- viewBox: 0 0 1280 720
- format: ppt169

## communication
- primary_language: zh-CN
- audience: 熟悉C与STM32的固件技术评审人员
- objective: 深入讲解日志上传机制，使评审者追踪数据、理解故障并识别待实测项。
- core_message: 同一96B记录进入独立Flash与上传队列，Flash及安全优先，导出仅成功前缀。

## mode
- mode: custom
- mode_behavior: 沿同一记录的数据流逐层讲解，从整体图到并发与错误边界。

## visual_style
- visual_style: custom
- visual_style_behavior: 浅底深蓝青色，关系用布局与连接表达，大字拆页，必要限制保留为正文。

## colors
- background: #FFFFFF
- secondary_bg: #F5F8FB
- primary: #123252
- accent: #007E87
- secondary_accent: #AD551D
- body_text: #233647
- secondary_text: #52697B
- divider: #CBD9E2

## typography
- font_family: Microsoft YaHei
- title_family: Microsoft YaHei
- body_family: Microsoft YaHei
- body: 30
- title: 42
- subtitle: 30
- annotation: 24
- display: 64

## icons
- library: none
- inventory: none

## page_rhythm
- P01: anchor
- P02: anchor
- P03: dense
- P04: dense
- P05: dense
- P06: dense
- P07: dense
- P08: dense
- P09: dense
- P10: anchor
- P11: dense
- P12: dense
- P13: dense
- P14: dense
- P15: dense
- P16: anchor
- P17: dense
- P18: dense
- P19: dense
- P20: anchor
- P21: dense
- P22: dense
- P23: dense
- P24: anchor

## pptx_structure
- mode: flat

## forbidden
- `mask`, `<style>`, `class`, external CSS, `<foreignObject>`, `textPath`, `@font-face`, `<animate*>`, `<set>`, `<script>` / event attributes, `<iframe>`
- HTML named entities in text; write typography as raw Unicode and escape XML reserved characters
