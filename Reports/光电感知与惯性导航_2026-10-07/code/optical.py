"""光电标定、两种偏差与 PID；Python 3.10+，仅依赖标准库。

阵列顺序为右到左，y 左为正，单位 m；q 越大越像目标线。
时间单位 s。100 Hz 只是示例，控制器使用实际采集时间戳。
LOST/形状阈值是本报告假设，需要依据实际光斑和赛道标定。
"""

from dataclasses import dataclass
import math

POSITIONS_M = (-0.02, -0.01, 0.0, 0.01, 0.02)
MIN_DT_S = 0.001
MAX_DT_S = 0.05
NOMINAL_DT_S = 0.01
LOST_TOTAL = 0.15
LOST_PEAK = 0.10
STRONG_THRESHOLD = 0.50
ALL_STRONG_THRESHOLD = 0.80


@dataclass
class OpticalResult:
    """e 无量纲；e_y_m 仅质心有物理位置，strength 是方法自身信号量。"""

    e: float
    e_y_m: float | None
    valid: bool
    reason: str
    method: str
    strength: float


def _finite(value, name):
    try:
        number = float(value)
    except (TypeError, ValueError, OverflowError) as exc:
        raise ValueError(f"{name} must be finite") from exc
    if not math.isfinite(number):
        raise ValueError(f"{name} must be finite")
    return number


def _vector(values, name):
    if isinstance(values, (str, bytes)):
        raise ValueError(f"{name} must be a numeric sequence")
    try:
        result = [_finite(value, name) for value in values]
    except TypeError as exc:
        raise ValueError(f"{name} must be a sequence") from exc
    if not result:
        raise ValueError(f"{name} must not be empty")
    return result


def _clip(value, low, high):
    return max(low, min(high, value))


def _dt_in_range(dt):
    # 相减时间戳可能在边界产生约 1e-16 s 舍入误差。
    return MIN_DT_S - 1e-12 <= dt <= MAX_DT_S + 1e-12


def _geometry(positions_m):
    positions = _vector(positions_m, "positions_m")
    if len(positions) < 2 or any(a >= b for a, b in zip(positions, positions[1:])):
        raise ValueError("positions_m must be strictly increasing, right to left")
    if max(abs(value) for value in positions) == 0:
        raise ValueError("positions_m must have nonzero span")
    return positions


def _input_q(q, positions_m):
    values = _vector(q, "q")
    positions = _geometry(positions_m)
    if len(values) != len(positions):
        raise ValueError("q and positions_m lengths differ")
    if any(value < 0 or value > 1 for value in values):
        raise ValueError("q must be in [0, 1]")
    return values, positions


def normalize(raw, background, target, raw_min=0.0, raw_max=1023.0, min_span=20.0):
    """逐通道 q=clip((raw-background)/(target-background),0,1)。

    默认 raw 为 ADC counts；正/负标定极性均支持。原始超量程、低对比
    或配置错误抛 ValueError。RC 计时可指定一致的单位与范围后复用此函数。
    """
    low = _finite(raw_min, "raw_min")
    high = _finite(raw_max, "raw_max")
    span = _finite(min_span, "min_span")
    if high <= low or span <= 0:
        raise ValueError("invalid raw range or min_span")
    values = _vector(raw, "raw")
    backgrounds = _vector(background, "background")
    targets = _vector(target, "target")
    if not len(values) == len(backgrounds) == len(targets):
        raise ValueError("raw and calibration lengths differ")
    if any(not low <= value <= high for value in values + backgrounds + targets):
        raise ValueError("raw or calibration is outside range")
    if any(abs(line - bg) < span for bg, line in zip(backgrounds, targets)):
        raise ValueError("insufficient calibration contrast")
    return [_clip((value - bg) / (line - bg), 0.0, 1.0)
            for value, bg, line in zip(values, backgrounds, targets)]


def digital(levels, target_level=0):
    """一位 DO 映射到 q；默认黑线为 LOW，仅允许 0/1，不实现 RC 驱动。"""
    target = _finite(target_level, "target_level")
    values = _vector(levels, "levels")
    if target not in (0.0, 1.0) or any(value not in (0.0, 1.0) for value in values):
        raise ValueError("digital levels and target_level must be 0 or 1")
    return [1.0 if value == target else 0.0 for value in values]


def centroid(q, positions_m=POSITIONS_M, min_strength=0.15):
    """质心 e_y=Σyq/Σq [m]，e=e_y/max|y|；strength=Σq。"""
    values, positions = _input_q(q, positions_m)
    threshold = _finite(min_strength, "min_strength")
    if threshold <= 0:
        raise ValueError("min_strength must be positive")
    strength = sum(values)
    if strength < threshold:
        return OpticalResult(0.0, None, False, "LOW_SIGNAL", "centroid", strength)
    e_y = sum(y * value for y, value in zip(positions, values)) / strength
    e = e_y / max(abs(y) for y in positions)
    return OpticalResult(e, e_y, True, "OK", "centroid", strength)


def difference_over_sum(q, positions_m=POSITIONS_M, min_denominator=0.05):
    """L/R 为 y>0/y<0 通道 q 均值；e=(L-R)/(L+R)。

    中心排除；strength=L+R。分母小且中心 q>=0.5 为 CENTER_ONLY，
    否则 LOW_DENOMINATOR；无效 e=0 只占位。
    """
    values, positions = _input_q(q, positions_m)
    threshold = _finite(min_denominator, "min_denominator")
    if threshold <= 0:
        raise ValueError("min_denominator must be positive")
    left = [value for value, y in zip(values, positions) if y > 0]
    right = [value for value, y in zip(values, positions) if y < 0]
    if not left or not right:
        raise ValueError("difference_over_sum requires left and right channels")
    l_mean, r_mean = sum(left) / len(left), sum(right) / len(right)
    denominator = l_mean + r_mean
    if denominator < threshold:
        center_strong = any(y == 0 and value >= STRONG_THRESHOLD
                            for value, y in zip(values, positions))
        reason = "CENTER_ONLY" if center_strong else "LOW_DENOMINATOR"
        return OpticalResult(0.0, None, False, reason, "dos", denominator)
    return OpticalResult((l_mean - r_mean) / denominator, None, True,
                         "OK", "dos", denominator)


class PID:
    """e/u 无量纲；kp 无量纲，ki [1/s]、kd [s]、tau_d [s]。

    正偏差给左转正输出。integral 已乘 ki，derivative 单位 1/s。
    实际 dt 必须在 [0.001, 0.05] s；首帧导数零，条件积分抗饱和。
    """

    def __init__(self, kp=0.8, ki=0.1, kd=0.02, u_max=0.5, tau_d=0.02):
        self.kp = _finite(kp, "kp")
        self.ki = _finite(ki, "ki")
        self.kd = _finite(kd, "kd")
        self.u_max = _finite(u_max, "u_max")
        self.tau_d = _finite(tau_d, "tau_d")
        if min(self.kp, self.ki, self.kd, self.tau_d) < 0 or self.u_max <= 0:
            raise ValueError("PID gains/tau_d must be nonnegative, u_max positive")
        self.reset()

    def reset(self):
        """清除历史；新有效首帧不会产生差分导数脉冲。"""
        self.integral = 0.0
        self.derivative = 0.0
        self._previous_e = None

    def step(self, e, dt_s):
        error = _finite(e, "e")
        dt = _finite(dt_s, "dt_s")
        if not _dt_in_range(dt):
            raise ValueError("dt_s outside [0.001, 0.05]")
        if self._previous_e is None:
            derivative = 0.0
        else:
            d_raw = (error - self._previous_e) / dt
            derivative = self.derivative + dt / (self.tau_d + dt) * (d_raw - self.derivative)
        increment = self.ki * error * dt
        i_try = self.integral + increment
        u_try = self.kp * error + i_try + self.kd * derivative
        if not all(math.isfinite(value) for value in (derivative, i_try, u_try)):
            raise ValueError("PID arithmetic overflow")
        blocks_growth = ((u_try > self.u_max and increment > 0) or
                         (u_try < -self.u_max and increment < 0))
        integral = self.integral if blocks_growth else i_try
        u_raw = self.kp * error + integral + self.kd * derivative
        self.integral, self.derivative, self._previous_e = integral, derivative, error
        return _clip(u_raw, -self.u_max, self.u_max)


class OpticalController:
    """采样→标定→可选低通→偏差→PID→差速车轮速示例 [m/s]。

    analog 原始量默认 10 位 ADC counts；digital 为一位 DO。不包含硬件
    ADC/GPIO/RC/PWM 驱动。任何异常或丢线立即停止并清除 PID/滤波。
    DOS 回退质心是教材示例；生产应给两算法分别标定 PID 增益。
    """

    def __init__(self, method="centroid", background=(100,) * 5,
                 target=(900,) * 5, positions_m=POSITIONS_M, tau_q=0,
                 pid=None, v_base=0.3, k_u=0.3, v_max=0.6):
        if method not in ("centroid", "dos"):
            raise ValueError("method must be centroid or dos")
        self.method = method
        self.positions_m = tuple(_geometry(positions_m))
        self.background = tuple(_vector(background, "background"))
        self.target = tuple(_vector(target, "target"))
        if len(self.background) != len(self.positions_m):
            raise ValueError("calibration and geometry lengths differ")
        normalize(self.background, self.background, self.target)
        if method == "dos":
            difference_over_sum([0.0] * len(self.positions_m), self.positions_m)
        self.tau_q = _finite(tau_q, "tau_q")
        self.v_base = _finite(v_base, "v_base")
        self.k_u = _finite(k_u, "k_u")
        self.v_max = _finite(v_max, "v_max")
        if self.tau_q < 0 or self.k_u < 0 or not 0 <= self.v_base <= self.v_max:
            raise ValueError("invalid filter or wheel speed parameters")
        if pid is not None and not isinstance(pid, PID):
            raise ValueError("pid must be PID")
        self.pid = pid if pid is not None else PID()
        if self.k_u * self.pid.u_max > min(self.v_base, self.v_max - self.v_base) + 1e-12:
            raise ValueError("wheel limits require smaller k_u*u_max")
        self._time = None
        self._filtered = None
        self._active_method = None
        self.pid.reset()

    def _reset_control(self):
        self.pid.reset()
        self._filtered = None
        self._active_method = None

    def _stopped(self, t_s, reason, q=None, strength=0.0):
        self._reset_control()
        return {"t_s": t_s, "q": [] if q is None else q, "e": 0.0,
                "e_y_m": None, "valid": False, "reason": reason,
                "method": self.method, "strength": strength, "u": 0.0,
                "v_left_m_s": 0.0, "v_right_m_s": 0.0}

    def step(self, t_s, raw, mode="analog", target_level=0):
        """t_s 为单调采集时间 [s]，返回含 valid/reason 的可 JSON 化字典。

        非单调时间不回退基线；正向大缺口重建基线。异常 e=0 不能当居中。
        """
        try:
            timestamp = _finite(t_s, "t_s")
        except ValueError:
            return self._stopped(None, "TIME_INVALID")
        if self._time is None:
            dt = NOMINAL_DT_S
        else:
            dt = timestamp - self._time
            if dt <= 0:
                return self._stopped(timestamp, "TIME_NONMONOTONIC")
            if not _dt_in_range(dt):
                self._time = timestamp
                return self._stopped(timestamp, "TIME_GAP")
        self._time = timestamp
        try:
            if mode == "analog":
                q = normalize(raw, self.background, self.target)
            elif mode == "digital":
                q = digital(raw, target_level)
            else:
                raise ValueError("mode must be analog or digital")
            if len(q) != len(self.positions_m):
                raise ValueError("raw and geometry lengths differ")
        except ValueError:
            return self._stopped(timestamp, "INPUT_INVALID")

        # 形状与低信号检查先于低通，防止滤波历史掩盖当前丢线/路口。
        total = sum(q)
        if total < LOST_TOTAL or max(q) < LOST_PEAK:
            return self._stopped(timestamp, "LOST", q, total)
        blocks = 0
        previous_strong = False
        for value in q:
            strong = value >= STRONG_THRESHOLD
            if strong and not previous_strong:
                blocks += 1
            previous_strong = strong
        if all(value >= ALL_STRONG_THRESHOLD for value in q) or blocks >= 2:
            return self._stopped(timestamp, "AMBIGUOUS", q, total)

        if self._filtered is None or self.tau_q == 0:
            filtered = q.copy()
        else:
            alpha = dt / (self.tau_q + dt)
            filtered = [old + alpha * (new - old) for old, new in zip(self._filtered, q)]
        self._filtered = filtered
        if self.method == "centroid":
            result = centroid(filtered, self.positions_m)
        else:
            result = difference_over_sum(filtered, self.positions_m)
            if not result.valid:
                fallback = centroid(filtered, self.positions_m)
                if fallback.valid:
                    fallback.method = "centroid_fallback"
                    fallback.reason = "FALLBACK_" + result.reason
                    result = fallback
        if not result.valid:
            return self._stopped(timestamp, result.reason, filtered, result.strength)
        if self._active_method != result.method:
            self.pid.reset()
        self._active_method = result.method
        try:
            u = self.pid.step(result.e, dt)
        except ValueError:
            return self._stopped(timestamp, "PID_INVALID", filtered, result.strength)
        return {"t_s": timestamp, "q": filtered, "e": result.e,
                "e_y_m": result.e_y_m, "valid": True, "reason": result.reason,
                "method": result.method, "strength": result.strength, "u": u,
                "v_left_m_s": self.v_base - self.k_u * u,
                "v_right_m_s": self.v_base + self.k_u * u}
