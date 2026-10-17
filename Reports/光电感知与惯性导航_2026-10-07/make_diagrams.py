"""生成静态 SVG，报告可离线显示；无第三方依赖。"""
from pathlib import Path
from html import escape
ROOT=Path(__file__).resolve().parent
def box(x,y,w,h,label,color='#eaf3fa'):
    lines=label.split('|')
    txt=''.join(f'<tspan x="{x+w/2}" dy="{0 if i==0 else 23}">{escape(s)}</tspan>' for i,s in enumerate(lines))
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="9" fill="{color}" stroke="#6c96b0"/><text x="{x+w/2}" y="{y+h/2-10*(len(lines)-1)+5}" text-anchor="middle" font-size="17">{txt}</text>'
def arrow(x1,y1,x2,y2):
    return f'<path d="M{x1},{y1} L{x2},{y2}" stroke="#356780" stroke-width="2" fill="none" marker-end="url(#a)"/>'
def svg(body,h):
    return f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1080 {h}" role="img"><title>光电与惯性导航算法关系图</title><defs><marker id="a" markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto"><path d="M0,0 L8,4 L0,8" fill="#356780"/></marker></defs><rect width="1080" height="{h}" fill="#fff"/><g font-family="Microsoft YaHei, sans-serif" fill="#17394c">{body}</g></svg>'
b=box(30,25,1020,60,'同步采样及时间戳　ADC / GPIO / IMU / 编码器')
b+=arrow(280,85,280,125)+arrow(800,85,800,125)
b+=box(30,125,500,65,'光电极性标定 → 归一化 → 可选滤波')+box(550,125,500,65,'IMU 轴映射 / SI 单位 / 零偏标定')
b+=arrow(280,190,280,230)+arrow(800,190,800,230)
b+=box(30,230,500,65,'有效性检查 → 质心 或 左右差比和')+box(550,230,500,65,'姿态与重力补偿 → 纯 IMU 积分|或 编码器轮程＋陀螺角增量融合')
b+=arrow(280,295,280,335)+arrow(800,295,800,335)
b+=box(30,335,500,70,'正常偏差 → PID → 限幅 → 轮速指令|无效 / 丢线 → 复位 PID → 停止')+box(550,335,500,70,'有效状态 → 姿态 / 速度 / 位置|异常 → 状态标志 / 重新建立基线')
b+=arrow(280,405,280,445)+arrow(800,405,800,445)
b+=box(30,445,1020,60,'控制输出 + CSV 原始与结果记录 + JSON 上位机输出', '#e8f4eb')
b+=f'<text x="540" y="548" text-anchor="middle" font-size="16">下一周期继续；外部定位 / 可信静止观测按条件提供修正</text>'
(ROOT/'流程图.svg').write_text(svg(b,580),encoding='utf8')
b=''
branches=[(30,35,300,100,'光电算法|质心 / 差比和|单调区间与噪声'),(750,35,300,100,'惯性状态|姿态 / 相对航向|速度 / 位置 / 轨迹'),(30,285,300,100,'感知与闭环|极性 / 标定 / 滤波|PID / 饱和 / 丢线'),(750,285,300,100,'轮式融合与修正|轮径轮距 / 角增量|静止更新 / 外部观测'),(30,535,300,100,'实现与验证|明确接口 / 异常状态|算例 / 单元验证'),(750,535,300,100,'记录与交付|时间同步 / SI 单位|CSV / JSON / 参考链接')]
for x,y,w,h,label in branches:
    b+=arrow(540,330,x+w/2,y+h/2)+box(x,y,w,h,label)
b+=box(395,285,290,90,'光电感知与惯性导航|从测量到可靠输出','#dceef8')
(ROOT/'思维导图.svg').write_text(svg(b,675),encoding='utf8')
print('2 SVG diagrams created')
