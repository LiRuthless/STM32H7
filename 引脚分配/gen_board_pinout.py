# -*- coding: utf-8 -*-
# 生成 WeAct MiniSTM32H7xx 核心板两侧排针引脚图（数据来自原理图 SchDoc V12）
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

# ---------- 解析引脚分配表 ----------
text = open('STM32H743VIT6_引脚分配表.md', encoding='utf-8').read()
rows = re.findall(r'^\|\s*(\d+)\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|$',
                  text, re.M)

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

FUNC, CATS = {}, {}
for pos, name, alloc, desc in rows:
    base = name.strip().split(' ')[0].split('-')[0].replace('_C', '')
    FUNC[base] = alloc.strip().replace('**', '').replace('🔒', '')
    CATS[base] = category(alloc.strip(), desc.strip())

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

PIP10 = {
    1: 'GND', 3: 'PE1', 5: 'PB9', 7: 'PB7', 9: 'PB5', 11: 'PB3',
    13: 'PD6', 15: 'PD4', 17: 'PD2', 19: 'PD0', 21: 'PC11', 23: 'PA15',
    25: 'PA11', 27: 'PA9', 29: 'PC9', 31: 'PC7', 33: 'PD15', 35: 'PD13',
    37: 'PD11', 39: 'PD9', 41: 'PB15', 43: 'PB13',
    2: '5V', 4: 'PE0', 6: 'PB8', 8: 'PB6', 10: 'PB4', 12: 'PD7',
    14: 'PD5', 16: 'PD3', 18: 'PD1', 20: 'PC12', 22: 'PC10', 24: 'PA12',
    26: 'PA10', 28: 'PA8', 30: 'PC8', 32: 'PC6', 34: 'PD14', 36: 'PD12',
    38: 'PD10', 40: 'PD8', 42: 'PB14', 44: 'PB12',
}
PIP20 = {
    1: 'GND', 3: 'PE2', 5: 'PE4', 7: 'PE6', 9: 'PC13', 11: 'PC0',
    13: 'PC2', 15: 'GND', 17: 'PA0', 19: 'PA2', 21: 'PA4', 23: 'PA6',
    25: 'PC4', 27: 'PB0', 29: 'PB2', 31: 'PE8', 33: 'PE10', 35: 'PE12',
    37: 'PE14', 39: 'PB10', 41: '3V3', 43: '3V3',
    2: '3V3', 4: 'PE3', 6: 'PE5', 8: 'VBAT', 10: 'NRST', 12: 'PC1',
    14: 'PC3', 16: 'VREF+', 18: 'PA1', 20: 'PA3', 22: 'PA5', 24: 'PA7',
    26: 'PC5', 28: 'PB1', 30: 'PE7', 32: 'PE9', 34: 'PE11', 36: 'PE13',
    38: 'PE15', 40: 'PB11', 42: '5V', 44: 'GND',
}
POWER_NETS = {'5V', '3V3', 'GND', 'VBAT', 'NRST', 'VREF+'}

# ---------- 布局参数 ----------
FS = 12.0          # 字体
rh = 0.56          # 行距
ROWS = 22
BH = ROWS * rh + 1.0     # 排针条高度
SW = 1.7                 # 排针条宽度
GAP = 6.6                # 两条排针之间的间距（放内侧标签）
B_SIDE = 5.6             # 外侧文字带宽度

L0 = -GAP/2 - SW         # 左排针条左沿
R0 = GAP/2               # 右排针条左沿

fig, ax = plt.subplots(figsize=(22, 17))
ax.set_aspect('equal')
ax.axis('off')

# 两条排针的黑色条
for x0 in (L0, R0):
    ax.add_patch(plt.Rectangle((x0, -BH/2), SW, BH, facecolor='#2b2b2b',
                               edgecolor='black', zorder=2))

def draw_label(x, y, s, cat, ha):
    ax.text(x, y, s, fontsize=FS, ha=ha, va='center',
            color='#222222' if cat != 'free' else '#999999',
            bbox=dict(boxstyle='round,pad=0.15', facecolor=COLORS[cat],
                      edgecolor='#bbbbbb', lw=0.5), zorder=4)

def draw_header(pmap, side):
    for pin, net in pmap.items():
        row = (pin + 1) // 2
        col = 0 if pin % 2 == 1 else 1      # 0=外侧列(奇)，1=内侧列(偶)
        y = BH/2 - 0.55 - (row - 1) * rh
        if net in POWER_NETS:
            label, cat = f'{pin}·{net}', 'power'
        else:
            label = f'{pin}·{net} {FUNC.get(net, "?")}'
            cat = CATS.get(net, 'other')
        if side == 'left':
            hx = L0 + 0.45 + col * 0.85
            if col == 0:   # 外侧列 → 标签在左外侧
                draw_label(L0 - 0.2, y, label, cat, 'right')
                ax.plot([L0 - 0.15, hx - 0.16], [y, y], color='#aaaaaa',
                        lw=0.8, zorder=2)
            else:          # 内侧列 → 标签在两排针之间
                draw_label(L0 + SW + 0.2, y, label, cat, 'left')
        else:
            hx = R0 + SW - 0.45 - col * 0.85
            if col == 0:   # 外侧列 → 标签在右外侧
                draw_label(R0 + SW + 0.2, y, label, cat, 'left')
                ax.plot([hx + 0.16, R0 + SW + 0.15], [y, y], color='#aaaaaa',
                        lw=0.8, zorder=2)
            else:          # 内侧列 → 标签在两排针之间
                draw_label(R0 - 0.2, y, label, cat, 'right')
        ax.add_patch(plt.Circle((hx, y), 0.14, facecolor='#d4af37',
                                edgecolor='#8a6d1f', lw=0.8, zorder=3))
        ax.add_patch(plt.Circle((hx, y), 0.06, facecolor='#2b2b2b',
                                edgecolor='none', zorder=4))

draw_header(PIP10, 'left')
draw_header(PIP20, 'right')

# 排针名称
ax.text(L0 - 0.2, BH/2 + 0.55, '左排针（PIP10）', fontsize=14,
        ha='right', va='center', weight='bold')
ax.text(R0 + SW + 0.2, BH/2 + 0.55, '右排针（PIP20）', fontsize=14,
        ha='left', va='center', weight='bold')

# 标题
title_y = BH/2 + 3.2
ax.text(0, title_y, 'WeAct MiniSTM32H7xx 核心板排针引脚图',
        fontsize=20, ha='center', va='center', weight='bold')
ax.text(0, title_y - 0.9, '智能车项目功能分配 · 标签格式：排针脚位·MCU引脚 功能',
        fontsize=12.5, ha='center', va='center', color='#555555')

# 图例
lg1 = BH/2 + 1.8
full_half = GAP/2 + SW + B_SIDE
col_w = (2 * full_half) / 4
for i, (cat, label) in enumerate(LEGEND):
    x = -full_half + (i % 4) * col_w + 0.2
    y = lg1 - (i // 4) * 0.8
    ax.add_patch(plt.Rectangle((x, y - 0.22), 0.55, 0.44,
                               facecolor=COLORS[cat], edgecolor='#999999'))
    ax.text(x + 0.72, y, label, fontsize=12, ha='left', va='center')

ax.set_xlim(-full_half - 0.5, full_half + 0.5)
ax.set_ylim(-BH/2 - 0.8, title_y + 0.8)

plt.savefig('STM32H743VIT6_核心板排针图.png', dpi=160,
            bbox_inches='tight', pad_inches=0.35, facecolor='white')
print('saved: STM32H743VIT6_核心板排针图.png')
