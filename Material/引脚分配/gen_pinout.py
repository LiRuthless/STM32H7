# -*- coding: utf-8 -*-
# 从 STM32H743VIT6_引脚分配表.md 生成 LQFP100 引脚分布图
import re
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib import font_manager

for fp in [r'C:\Windows\Fonts\msyh.ttc', r'C:\Windows\Fonts\simhei.ttf']:
    try:
        font_manager.fontManager.addfont(fp)
    except Exception:
        pass
plt.rcParams['font.family'] = ['Microsoft YaHei', 'SimHei']
plt.rcParams['axes.unicode_minus'] = False

# ---------- 解析 markdown 引脚表 ----------
text = open('STM32H743VIT6_引脚分配表.md', encoding='utf-8').read()
rows = re.findall(r'^\|\s*(\d+)\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|$',
                  text, re.M)
pins = {}
for pos, name, alloc, desc in rows:
    pos = int(pos)
    if 1 <= pos <= 100:
        pins[pos] = (name.strip(), alloc.strip(), desc.strip())
assert len(pins) == 100, f'解析到 {len(pins)} 个引脚，应为 100'

# ---------- 分类着色 ----------
def category(alloc, desc):
    t = alloc + desc
    if '电源' in alloc:
        return 'power'
    if '🔒' in alloc:
        return 'board'
    if '未使用' in alloc:
        return 'free'
    if re.search(r'启动按键|LED|灰度 IO|XS|扩展', t):
        return 'gpio'
    if re.search(r'电感|按键|电池|灰度 ADC|备用 ADC|ADC', t):
        return 'adc'
    if re.search(r'PWM|DIR|风扇|驱动', t):
        return 'pwm'
    if '编码器' in t:
        return 'encoder'
    if re.search(r'LCD|屏|WiFi|串口|SCK|MOSI|MISO|DL1B|IMU|I2C|SPI|UART', t):
        return 'comm'
    return 'other'

COLORS = {
    'power':   '#f4b6b6', 'board': '#d9d9d9', 'free': '#f2f2f2',
    'adc':     '#ffe8a3', 'pwm':   '#ffc48f', 'encoder': '#b7e4a8',
    'comm':    '#a8d8f0', 'gpio':  '#d7c7f0', 'other': '#ffffff',
}
LEGEND = [
    ('power', '电源/晶振/复位'), ('board', '板载占用(Flash/TF/USB等)'),
    ('adc', 'ADC 模拟输入'), ('pwm', 'PWM / 电机驱动'),
    ('encoder', '编码器'), ('comm', '通信(屏/WiFi/IMU/串口/I2C)'),
    ('gpio', 'GPIO(按键/LED/控制)'), ('free', '未使用(备用)'),
]

def short_label(pos):
    name, alloc, desc = pins[pos]
    alloc = alloc.replace('**', '').replace('🔒', '')
    name = re.sub(r'\s*\(.*\)', '', name)
    return f'{pos}·{name} {alloc}'

# ---------- 布局参数 ----------
FS = 12.5          # 引脚字体
W = 13.0           # 芯片边长（与图例同宽）
stub = 0.7         # 引脚短线
B_SIDE = 6.2       # 左右文字带宽度
B_VERT = 5.2       # 上下文字带高度（60°旋转标签）
PAD = 0.6          # 内容区内边距

half_x = W/2 + stub + B_SIDE      # 左右文字带外沿
half_y = W/2 + stub + B_VERT      # 下文字带外沿

fig, ax = plt.subplots(figsize=(24, 22))
ax.set_aspect('equal')
ax.axis('off')

# 芯片本体
ax.add_patch(plt.Rectangle((-W/2, -W/2), W, W, facecolor='#333333',
                           edgecolor='black', zorder=3))
ax.text(0, 0.5, 'STM32H743VIT6', color='white', fontsize=26,
        ha='center', va='center', weight='bold', zorder=4)
ax.text(0, -0.9, 'LQFP100', color='#cccccc', fontsize=16,
        ha='center', va='center', zorder=4)
ax.add_patch(plt.Circle((-W/2 + 0.7, W/2 - 0.7), 0.24, facecolor='white',
                        edgecolor='white', zorder=4))
ax.text(-W/2 + 1.25, W/2 - 0.7, 'Pin1', color='white', fontsize=11,
        ha='left', va='center', zorder=4)

def draw_pin(x1, y1, x2, y2, tx, ty, ha, pos, rot=None):
    cat = category(pins[pos][1], pins[pos][2])
    ax.plot([x1, x2], [y1, y2], color='#555555', lw=1.3, zorder=2)
    ax.add_patch(plt.Circle((x2, y2), 0.1, facecolor=COLORS[cat],
                            edgecolor='#555555', lw=0.6, zorder=3))
    kw = {}
    if rot:
        kw = dict(rotation=rot, rotation_mode='anchor')
    ax.text(tx, ty, short_label(pos), fontsize=FS, ha=ha, va='center',
            color='#222222' if cat != 'free' else '#999999',
            bbox=dict(boxstyle='round,pad=0.18', facecolor=COLORS[cat],
                      edgecolor='#bbbbbb', lw=0.5, alpha=0.95),
            zorder=3, **kw)

N = 25
step = W / (N + 1)
# 左边 1-25（上→下）
for i, pos in enumerate(range(1, 26)):
    y = W/2 - (i + 1) * step
    draw_pin(-W/2, y, -W/2 - stub, y, -W/2 - stub - 0.18, y, 'right', pos)
# 右边 51-75（下→上）
for i, pos in enumerate(range(51, 76)):
    y = -W/2 + (i + 1) * step
    draw_pin(W/2, y, W/2 + stub, y, W/2 + stub + 0.18, y, 'left', pos)
# 下边 26-50（左→右，文字 60°）
for i, pos in enumerate(range(26, 51)):
    x = -W/2 + (i + 1) * step
    draw_pin(x, -W/2, x, -W/2 - stub, x, -W/2 - stub - 0.18, 'right', pos,
             rot=60)
    ax.texts[-1].set_va('top')
# 上边 76-100（右→左，文字 60°）
for i, pos in enumerate(range(76, 101)):
    x = W/2 - (i + 1) * step
    draw_pin(x, W/2, x, W/2 + stub, x, W/2 + stub + 0.18, 'left', pos,
             rot=60)
    ax.texts[-1].set_va('bottom')

# 标题
lg_row1 = half_y + 3.1
title_y = half_y + 5.6
ax.text(0, title_y, 'STM32H743VIT6 (LQFP100) 智能车引脚分配图',
        fontsize=22, ha='center', va='center', weight='bold')
ax.text(0, title_y - 1.0, '俯视图 · 引脚1在左上角 · 逆时针编号',
        fontsize=14, ha='center', va='center', color='#555555')

# 图例：两排四列，总宽与芯片组件同宽
col_w = (2 * half_x) / 4
for i, (cat, label) in enumerate(LEGEND):
    x = -half_x + (i % 4) * col_w + 0.2
    y = lg_row1 - (i // 4) * 0.85
    ax.add_patch(plt.Rectangle((x, y - 0.24), 0.6, 0.48,
                               facecolor=COLORS[cat], edgecolor='#999999'))
    ax.text(x + 0.8, y, label, fontsize=13, ha='left', va='center')

# 视图范围：四边留白相等
ax.set_xlim(-half_x - PAD, half_x + PAD)
ax.set_ylim(-half_y - PAD, title_y + 0.9)

plt.savefig('STM32H743VIT6_引脚分配图.png', dpi=160,
            bbox_inches='tight', pad_inches=0.35, facecolor='white')
print('saved: STM32H743VIT6_引脚分配图.png')
