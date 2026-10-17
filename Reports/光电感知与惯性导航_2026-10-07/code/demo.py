"""离线示例：原始 SI 输入 -> 光电控制/惯导 -> CSV + JSON。

运行 python demo.py，输出位于 code/output；输入为人工构造的接口数据，非实测。
实际硬件请以 DRDY 采样时刻提供同步编码器计数，并使用后台日志队列。
"""
from dataclasses import asdict
from pathlib import Path
import csv
import json
import math
from optical import OpticalController
from inertial import G, IMUSample, Navigator

def main():
    out = Path(__file__).resolve().parent / "output"
    out.mkdir(exist_ok=True)
    opt = OpticalController(method="dos")
    nav = Navigator(k_a=0.0)  # 已知姿态的代数演示，非实车建议参数。
    metadata = {
        "format_version": 1, "data_origin": "synthetic_interface_demo",
        "sample_period_nominal_s": 0.01, "rate_hz": 100,
        "clock": "startup_monotonic_seconds", "timestamp": "sampling time",
        "frame_b": "x_forward_y_left_z_up", "frame_n": "startup_local_x_forward_y_left_z_up",
        "quaternion": "Hamilton scalar_first body_to_navigation",
        "units": {"accel": "m/s^2 specific_force", "gyro": "rad/s", "position": "m", "velocity": "m/s", "yaw": "rad CCW [-pi,pi)", "encoder": "decoded cumulative count", "adc": "count"},
        "optical": {"positions_m": [-0.02,-0.01,0,0.01,0.02], "background": [100]*5, "target": [900]*5, "raw_range": [0,1023], "method": "dos with explicit centroid fallback", "tau_q_s": 0, "kp": .8, "ki_s_inverse": .1, "kd_s": .02, "u_max": .5, "v_base_m_s": .3, "k_u_m_s": .3, "v_max_m_s": .6},
        "inertial": {"k_a_s_inverse": 0, "gyro_weight": .7, "meters_per_count_left": .001, "meters_per_count_right": .001, "wheelbase_m": .2, "gyro_bias_rad_s": [0,0,0], "accel_bias_m_s2": [0,0,0]},
        "hardware": {"sensor_model": "unspecified", "calibration_id": "synthetic_100_900", "temperature_C": None},
        "assumptions": "synchronized planar differential drive, no slip; turn sample omits physical centripetal acceleration",
    }
    (out / "metadata.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2, allow_nan=False), encoding="utf-8")
    fields = ["seq","t_sample_ns","t_receive_ns","dt_s","adc_0","adc_1","adc_2","adc_3","adc_4","accel_m_x","accel_m_y","accel_m_z","gyro_m_x","gyro_m_y","gyro_m_z","count_left","count_right","stationary","optical_q_json","optical_e","optical_valid","optical_reason","optical_method","u","v_left_m_s","v_right_m_s","q_nb_json","a_n_json","v_n_json","p_n_json","gyro_bias_before_json","gyro_bias_after_json","accel_bias_json","pure_yaw_rad","odom_json","nav_valid","nav_reason","nav_quality"]
    left = right = 0
    frames = []
    with (out / "samples.csv").open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        for k in range(101):
            t = k * .01
            if k == 1:
                left, right = 8, 12
            elif k > 1:
                left += 10
                right += 10
            raw = [140,180,260,740,820]  # q=[.05,.10,.20,.80,.90]，报告算例。
            if k == 50:
                raw = [100]*5  # 丢线验证：停止，而不是把无测量当居中。
            if k == 60:
                raw = [100,100,900,100,100]  # DOS CENTER_ONLY -> 质心显式回退。
            sample = IMUSample(t, (0.,0.,G), (0.,0.,1.8 if k == 1 else 0.), left, right, k == 0)
            control = opt.step(t, raw)
            bias_before = nav.gyro_bias
            state = nav.update(sample)
            frame = {"seq": k, "t_sample_ns": round(t*1e9), "input": asdict(sample), "adc": raw, "gyro_bias_before_rad_s": bias_before, "gyro_bias_after_rad_s": nav.gyro_bias, "accel_bias_m_s2": nav.accel_bias, "control": control, "navigation": state}
            frames.append(frame)
            def j(value):
                return json.dumps(value, ensure_ascii=False, separators=(",",":"), allow_nan=False)
            row = dict(seq=k, t_sample_ns=round(t*1e9), t_receive_ns=round(t*1e9), dt_s=.01 if k else 0., count_left=left, count_right=right, stationary=int(sample.stationary))
            row.update({f"adc_{i}": v for i,v in enumerate(raw)})
            for prefix,values in [("accel_m",sample.accel_m_s2),("gyro_m",sample.gyro_rad_s)]:
                row.update({f"{prefix}_{axis}": value for axis,value in zip("xyz",values)})
            row.update(optical_q_json=j(control["q"]), optical_e=control["e"], optical_valid=control["valid"], optical_reason=control["reason"], optical_method=control["method"],u=control["u"],v_left_m_s=control["v_left_m_s"],v_right_m_s=control["v_right_m_s"],q_nb_json=j(state["q_nb"]),a_n_json=j(state["a_n_m_s2"]),v_n_json=j(state["v_n_m_s"]),p_n_json=j(state["p_n_m"]),pure_yaw_rad=state["pure_yaw_rad"],odom_json=j(state["odom"]),nav_valid=state["valid"],nav_reason=state["reason"],nav_quality=state["quality"])
            row.update(gyro_bias_before_json=j(bias_before), gyro_bias_after_json=j(nav.gyro_bias), accel_bias_json=j(nav.accel_bias))
            writer.writerow(row)
    with (out / "frames.jsonl").open("w", encoding="utf-8") as f:
        for frame in frames:
            f.write(json.dumps(frame, ensure_ascii=False, allow_nan=False)+"\n")
    summary = {"samples":len(frames),"optical_example":frames[1]["control"],"odometry_example":frames[1]["navigation"]["odom"],"lost_line":frames[50]["control"],"center_only":frames[60]["control"],"final_navigation":frames[-1]["navigation"]}
    (out / "summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2, allow_nan=False), encoding="utf-8")
    print(json.dumps(summary, ensure_ascii=False, indent=2, allow_nan=False))

if __name__ == "__main__":
    main()
