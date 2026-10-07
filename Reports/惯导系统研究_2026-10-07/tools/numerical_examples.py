"""Deterministic illustrative calculations, NOT paper reproduction or vehicle tests."""
from pathlib import Path
import math, csv, json, platform, sys
ROOT=Path(__file__).resolve().parents[1]
G=9.80665
def wrap(a):return (a+math.pi)%(2*math.pi)-math.pi
def turn(delay):
 dt=.01; tau=.15; kp=3.; kd=.6; max_rate=1.2
 psi=rate=0.; target=math.pi/2; history=[0.]*delay; rows=[]
 for k in range(1001):
  t=k*dt; error=target-psi
  cmd=max(-max_rate,min(max_rate,kp*error-kd*rate))
  history.append(cmd); applied=history.pop(0)
  rows.append([t,psi,rate,error,cmd,applied])
  rate+=(applied-rate)*dt/tau
  psi+=rate*dt
 overshoot=max(0.,max(row[1] for row in rows)-target)*180/math.pi
 stable=None
 for i,row in enumerate(rows):
  if all(abs(r[3])<=math.radians(1) and abs(r[2])<=.02 for r in rows[i:]):
   stable=row[0]; break
 return rows,{'delay_s':delay*dt,'overshoot_deg':overshoot,'settling_1deg_and_0_02rad_s_s':stable,'final_error_deg':rows[-1][3]*180/math.pi,'dt_s':dt,'tau_s':tau,'kp_s_inverse':kp,'kd_dimensionless':kd,'max_rate_rad_s':max_rate}
def chart(series,path,title,xlabel,ylabel,xmax,ymax):
 import html
 colors=['#176b91','#b84a28','#39804c']; parts=[f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 880 390"><rect width="880" height="390" fill="white"/><style>text{{font-family:Microsoft YaHei,Arial,sans-serif;font-size:15px}}</style><text x="440" y="26" text-anchor="middle">{html.escape(title)}</text>']
 for i in range(6):
  x=85+i*720/5;y=320-i*260/5
  parts.append(f'<path d="M {x} 60 V 320 M 85 {y} H 805" stroke="#dce4eb" fill="none"/><text x="{x}" y="344" text-anchor="middle">{xmax*i/5:g}</text><text x="75" y="{y+5}" text-anchor="end">{ymax*i/5:g}</text>')
 parts.append(f'<path d="M85 60V320H805" fill="none" stroke="#345"/><text x="445" y="379" text-anchor="middle">{html.escape(xlabel)}</text><text x="15" y="44">{html.escape(ylabel)}</text>')
 for idx,(label,points) in enumerate(series):
  p=' '.join(f'{85+x/xmax*720:.2f},{320-y/ymax*260:.2f}' for x,y in points)
  parts.append(f'<polyline points="{p}" stroke="{colors[idx]}" stroke-width="2.5" fill="none"/><text x="{110+idx*240}" y="52" fill="{colors[idx]}">{html.escape(label)}</text>')
 parts.append('</svg>');path.write_text(''.join(parts),encoding='utf-8')
if __name__=='__main__':
 (ROOT/'figures').mkdir(exist_ok=True);(ROOT/'data').mkdir(exist_ok=True)
 runs=[]
 for delay in [0,10]:
  rows,result=turn(delay);runs.append((rows,result))
  with (ROOT/'data'/f'turn_delay_{delay:02d}.csv').open('w',newline='',encoding='utf-8') as f:
   writer=csv.writer(f);writer.writerow(['t_s','psi_rad','yaw_rate_rad_s','error_rad','requested_rad_s','applied_rad_s']);writer.writerows(rows)
 results={'kind':'本文自设参数数学算例与简化仿真','python':sys.version.split()[0],'platform':platform.platform(),
  'acc_bias_10s':{'residual_m_s2':.02,'dv_m_s':.2,'dp_m':1.},
  'tilt_1deg_10s':{'horizontal_leakage_m_s2':G*math.sin(math.pi/180),'dp_m':.5*G*math.sin(math.pi/180)*100},
  'tilt_gyro_bias_0_1deg_s_10s':{'dp_small_angle_m':G*math.radians(.1)*1000/6},
  'yaw_bias_0_1deg_s_speed_1_10s':{'dy_small_angle_m':.5*math.radians(.1)*100,'dy_exact_m':(1-math.cos(math.radians(.1)*10))/math.radians(.1)},
  'heading_wrap':{'target_deg':-179,'measured_deg':179,'error_deg':wrap(math.radians(-358))*180/math.pi},
  'wheel_map':{'v_m_s':.8,'omega_rad_s':.3,'track_m':.3,'left_m_s':.8-.3*.3/2,'right_m_s':.8+.3*.3/2,'ackermann_wheelbase_m':.5,'steer_deg':math.degrees(math.atan(.5*.3/.8))},
  'pure_pursuit':{'x_goal_body_m':.8,'y_goal_body_m':.2,'curvature_m_inverse':.4/.68,'omega_at_0_6m_s_rad_s':.6*.4/.68},
  'braking':{'v_m_s':1.,'deceleration_m_s2':.5,'delay_s':.1,'distance_m':1.1},
  'false_arrival':{'encoder_scale':1.05,'target_m':10.,'true_when_estimated_arrival_m':10/1.05,'shortfall_m':10-10/1.05},
  'scalar_speed_update':{'prior_m_s':1.,'prior_variance':.04,'measurement_m_s':.8,'measurement_variance':.01,'gain':.8,'posterior_m_s':.84,'posterior_variance':.008},
  'turn_simulations':[r for _,r in runs]}
 assert abs(results['heading_wrap']['error_deg']-2)<1e-10
 assert abs(results['yaw_bias_0_1deg_s_speed_1_10s']['dy_exact_m']-.0872642474)<1e-8
 (ROOT/'data'/'results.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
 chart([(f"延迟 {r['delay_s']:.2f} s",[(x[0],x[1]*180/math.pi) for x in rows]) for rows,r in runs],ROOT/'figures'/'turn_response.svg','定角转弯：简化一阶执行器仿真','时间 / s','航向 / °',10,100)
 times=[i*.1 for i in range(101)]
 chart([('加速度偏置 0.02 m/s²',[(t,.5*.02*t*t) for t in times]),('初始倾角误差 1°',[(t,.5*G*math.sin(math.pi/180)*t*t) for t in times]),('倾角陀螺残差 0.1°/s',[(t,G*math.radians(.1)*t**3/6) for t in times])],ROOT/'figures'/'error_growth.svg','误差传播量级：彼此独立的自设条件','时间 / s','位置误差 / m',10,9)
 print(json.dumps(results,ensure_ascii=False,indent=2))
