"""SI-unit inertial demonstration; Python 3.10+, standard library only.

Body: x forward, y left, z up. Navigation: startup-local, z up.
q_nb = (w,x,y,z), Hamilton, rotates body vectors into navigation.
An accelerometer measures specific force: at rest/level f_b=(0,0,+G).
The independent planar odometry assumes a level differential-drive vehicle.
Hardware must supply calibrated axis mapping, scale factors, synchronized
acquisition timestamps/cumulative encoder counts, and trusted stationary flags.
This is a teaching estimator, not a complete Mahony filter or an EKF.
"""

from dataclasses import dataclass
import math
from numbers import Real
from typing import Iterable

G = 9.80665
Vec3 = tuple[float, float, float]
Quat = tuple[float, float, float, float]


@dataclass(frozen=True)
class IMUSample:
    t_s: float
    accel_m_s2: Vec3
    gyro_rad_s: Vec3
    count_left: int | None = None
    count_right: int | None = None
    stationary: bool = False


def _finite(value: object) -> bool:
    if not isinstance(value, Real) or isinstance(value, bool):
        return False
    try:
        return math.isfinite(value)
    except OverflowError:
        return False


def _vector(value: object, size: int) -> tuple[float, ...]:
    try:
        result = tuple(value)  # type: ignore[arg-type]
    except TypeError as exc:
        raise ValueError("vector must be an iterable") from exc
    if len(result) != size or not all(_finite(v) for v in result):
        raise ValueError(f"expected {size} finite real values")
    return tuple(float(v) for v in result)


def _norm(v: tuple[float, ...]) -> float:
    return math.hypot(*v)


def _cross(a: Vec3, b: Vec3) -> Vec3:
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def _multiply(a: Quat, b: Quat) -> Quat:
    w,x,y,z = a
    v,i,j,k = b
    return (w*v-x*i-y*j-z*k, w*i+x*v+y*k-z*j,
            w*j-x*k+y*v+z*i, w*k+x*j-y*i+z*v)


def _unit(q: Quat) -> Quat:
    n = _norm(q)
    if not math.isfinite(n) or n < 1e-12:
        raise ValueError("quaternion norm is zero")
    return tuple(v/n for v in q)  # type: ignore[return-value]


def rotate(q_nb: Quat, v_b: Vec3) -> Vec3:
    """Rotate with a unit q_nb: R_nb v_b; no units are changed."""
    conjugate = (q_nb[0], -q_nb[1], -q_nb[2], -q_nb[3])
    result = _multiply(_multiply(q_nb, (0.0, *v_b)), conjugate)
    return result[1:]


def _euler(q: Quat) -> Vec3:
    w,x,y,z = q
    roll = math.atan2(2*(w*x+y*z), 1-2*(x*x+y*y))
    pitch = math.asin(max(-1.0, min(1.0, 2*(w*y-z*x))))
    yaw = math.atan2(2*(w*z+x*y), 1-2*(y*y+z*z))
    return roll, pitch, _wrap(yaw)


def _wrap(angle: float) -> float:
    return (angle+math.pi) % (2*math.pi)-math.pi


def _tilt_quaternion(f: Vec3) -> Quat:
    roll = math.atan2(f[1], f[2])
    pitch = math.atan2(-f[0], math.hypot(f[1], f[2]))
    cr,sr = math.cos(roll/2), math.sin(roll/2)
    cp,sp = math.cos(pitch/2), math.sin(pitch/2)
    # Rz(0) Ry(pitch) Rx(roll), yaw deliberately initialized to zero.
    return (cr*cp, sr*cp, cr*sp, -sr*sp)


class Navigator:
    def __init__(self, k_a: float = 1.0, gyro_bias: Vec3 = (0,0,0),
                 accel_bias: Vec3 = (0,0,0), gyro_weight: float = .7,
                 meters_per_count_left: float = .001,
                 meters_per_count_right: float = .001, wheelbase_m: float = .2,
                 initial_q_nb: Quat = (1,0,0,0),
                 dt_min_s: float = .001, dt_max_s: float = .05,
                 max_accel_m_s2: float = 16*G, max_gyro_rad_s: float = 35.0,
                 max_wheel_speed_m_s: float = 5.0,
                 angular_disagreement_rad_s: float = 2.0,
                 accel_gate_fraction: float = .15,
                 innovation_limit_rad: float = math.pi/4,
                 stationary_accel_fraction: float = .05,
                 stationary_gyro_limit_rad_s: float = .05,
                 gyro_bias_tau_s: float = 30.0):
        """All gains/limits are configurable examples, not sensor specifications.

        k_a: s^-1; biases: SI body coordinates; wheelbase: metres;
        counts are final decoded counts after gear ratio/quadrature decoding.
        Initial position/velocity are zero. initial_q_nb must be known, or call
        initialize_tilt on a trusted stationary sample before the first update.
        """
        params = (k_a, gyro_weight, meters_per_count_left, meters_per_count_right,
                  wheelbase_m, dt_min_s, dt_max_s, max_accel_m_s2, max_gyro_rad_s,
                  max_wheel_speed_m_s, angular_disagreement_rad_s,
                  accel_gate_fraction, innovation_limit_rad,
                  stationary_accel_fraction, stationary_gyro_limit_rad_s,
                  gyro_bias_tau_s)
        if not all(_finite(v) for v in params):
            raise ValueError("configuration must contain finite real values")
        if (k_a < 0 or not 0 <= gyro_weight <= 1 or
            min(meters_per_count_left,meters_per_count_right,wheelbase_m,
                dt_min_s,max_accel_m_s2,max_gyro_rad_s,max_wheel_speed_m_s,
                angular_disagreement_rad_s,stationary_gyro_limit_rad_s,
                gyro_bias_tau_s) <= 0 or dt_max_s < dt_min_s or
            not 0 < accel_gate_fraction < 1 or
            not 0 < stationary_accel_fraction < 1 or
            not 0 < innovation_limit_rad <= math.pi):
            raise ValueError("invalid navigation configuration")
        self.k_a, self.gyro_weight = k_a, gyro_weight
        self.gyro_bias = _vector(gyro_bias,3)
        self.accel_bias = _vector(accel_bias,3)
        if _norm(self.gyro_bias) > max_gyro_rad_s or _norm(self.accel_bias) > max_accel_m_s2:
            raise ValueError("bias magnitude exceeds configured sensor range")
        self.q_nb = _unit(_vector(initial_q_nb,4))
        self.c_left, self.c_right, self.wheelbase_m = (
            meters_per_count_left, meters_per_count_right, wheelbase_m)
        self.dt_min_s, self.dt_max_s = dt_min_s, dt_max_s
        self.max_accel, self.max_gyro = max_accel_m_s2, max_gyro_rad_s
        self.max_wheel_speed = max_wheel_speed_m_s
        self.angular_disagreement = angular_disagreement_rad_s
        self.accel_gate_fraction = accel_gate_fraction
        self.innovation_limit_rad = innovation_limit_rad
        self.stationary_accel_fraction = stationary_accel_fraction
        self.stationary_gyro_limit = stationary_gyro_limit_rad_s
        self.gyro_bias_tau_s = gyro_bias_tau_s
        self.v_n = (0.0,0.0,0.0)
        self.p_n = (0.0,0.0,0.0)
        self.a_n = (0.0,0.0,0.0)
        self._t = None
        self._counts = None
        self._pending_rebaseline = False
        self._incomplete = False
        self._odom_incomplete = False
        self._odom = {"x_m":0.0,"y_m":0.0,"yaw_rad":0.0,"speed_m_s":0.0,
                      "quality":"relative_dead_reckoning"}

    def initialize_tilt(self, sample: IMUSample) -> None:
        """Before first update only: use trusted static gravity; yaw remains zero.

        One static pose does not calibrate all three accelerometer biases.
        The caller supplies the external stationary truth; gates are additional
        rejection checks, not a proof that the vehicle is stationary.
        """
        if self._t is not None:
            raise ValueError("tilt initialization is allowed only before updates")
        f = _vector(sample.accel_m_s2,3)
        w = _vector(sample.gyro_rad_s,3)
        fc = tuple(v-b for v,b in zip(f,self.accel_bias))
        wc = tuple(v-b for v,b in zip(w,self.gyro_bias))
        if (sample.stationary is not True or
            abs(_norm(fc)-G) > self.stationary_accel_fraction*G or
            _norm(wc) > self.stationary_gyro_limit):
            raise ValueError("trusted stationary tilt conditions not met")
        self.q_nb = _tilt_quaternion(fc)

    def _baseline(self, t: float, sample: IMUSample) -> None:
        self._t = t
        self._counts = ((sample.count_left,sample.count_right)
                        if type(sample.count_left) is int and type(sample.count_right) is int
                        else None)

    def _packet(self, t: float | None, valid: bool, reason: str,
                quality: str, show_odom: bool = True) -> dict:
        roll,pitch,yaw = _euler(self.q_nb)
        odom = dict(self._odom) if show_odom and self._counts is not None else None
        if odom is not None and self._odom_incomplete:
            odom["quality"] += ";incomplete_dead_reckoning"
        if self._incomplete:
            quality += ";incomplete_dead_reckoning"
        return {"t_s":t,"valid":valid,"reason":reason,"quality":quality,
                "q_nb":self.q_nb,"a_n_m_s2":self.a_n,"v_n_m_s":self.v_n,
                "p_n_m":self.p_n,"pure_yaw_rad":yaw,"roll_rad":roll,
                "pitch_rad":pitch,"odom":odom}

    def _reject(self, sample: IMUSample, reason: str) -> dict:
        t = float(sample.t_s) if _finite(sample.t_s) else None
        if t is None:
            # 无法定位故障发生区间，下一有效帧只能重新建立基线。
            self._pending_rebaseline = True
            self._counts = None
            self._incomplete = self._odom_incomplete = True
        if t is not None and (self._t is None or t > self._t):
            self._baseline(t,sample)
            self._incomplete = self._odom_incomplete = True
        return self._packet(t,False,reason,"rejected_no_integration")

    def update(self, sample: IMUSample) -> dict:
        """First sample establishes baselines. dt comes from acquisition time.

        Invalid samples never update the integrated state. Positive time gaps or
        bad samples reset baselines, preventing later integration across the lost
        interval. Result quality then permanently flags an incomplete trajectory.
        Missing encoder samples only suspend odometry; restoration re-baselines.
        """
        if not isinstance(sample, IMUSample):
            raise ValueError("update requires IMUSample")
        if not _finite(sample.t_s) or type(sample.stationary) is not bool:
            return self._reject(sample,"INVALID_INPUT")
        try:
            f_m = _vector(sample.accel_m_s2,3)
            w_m = _vector(sample.gyro_rad_s,3)
        except ValueError:
            return self._reject(sample,"INVALID_INPUT")
        if any(v is not None and type(v) is not int
               for v in (sample.count_left,sample.count_right)):
            return self._reject(sample,"INVALID_ENCODER")
        if _norm(f_m) > self.max_accel or _norm(w_m) > self.max_gyro:
            return self._reject(sample,"IMU_OVERRANGE")
        t = float(sample.t_s)
        f = tuple(v-b for v,b in zip(f_m,self.accel_bias))
        w = tuple(v-b for v,b in zip(w_m,self.gyro_bias))
        if self._pending_rebaseline:
            # 已知旧时间仍不可跨未知时间故障补积分。
            if self._t is not None and t <= self._t:
                return self._packet(t,False,"NON_MONOTONIC_TIME","rejected_no_integration")
            self._baseline(t,sample)
            self._pending_rebaseline = False
            return self._packet(t,True,"REBASELINED_AFTER_INVALID_TIME","no_motion_update")
        if self._t is None:
            self._baseline(t,sample)
            rf = rotate(self.q_nb,f)
            self.a_n = (rf[0],rf[1],rf[2]-G)
            quality = "relative_dead_reckoning" if self._counts is not None else "pure_imu_drift_demo"
            return self._packet(t,True,"INITIALIZED",quality)
        dt = t-self._t
        if dt <= 0:
            return self._packet(t,False,"NON_MONOTONIC_TIME","rejected_no_integration")
        # Allow arithmetic roundoff at configured inclusive time boundaries.
        if dt < self.dt_min_s-1e-12 or dt > self.dt_max_s+1e-12:
            return self._reject(sample,"DT_OUT_OF_RANGE")
        have_counts = type(sample.count_left) is int and type(sample.count_right) is int
        wheel_delta = None
        if have_counts and self._counts is not None:
            wheel_delta = (sample.count_left-self._counts[0],
                           sample.count_right-self._counts[1])
            try:
                dl,dr = wheel_delta[0]*self.c_left, wheel_delta[1]*self.c_right
            except OverflowError:
                return self._reject(sample,"ENCODER_JUMP")
            if max(abs(dl),abs(dr))/dt > self.max_wheel_speed:
                return self._reject(sample,"ENCODER_JUMP")
        # External flag is mandatory. IMU/available wheel observations must agree.
        stationary_ok = (sample.stationary and
                         abs(_norm(f)-G) <= self.stationary_accel_fraction*G and
                         _norm(w) <= self.stationary_gyro_limit and
                         (not have_counts or wheel_delta == (0,0)))
        # Direction correction sign: measured up cross estimated up.
        feedback_ok = False
        wc = w
        nf = _norm(f)
        if self.k_a > 0 and nf > 1e-12 and abs(nf-G) <= self.accel_gate_fraction*G:
            conjugate = (self.q_nb[0],-self.q_nb[1],-self.q_nb[2],-self.q_nb[3])
            up_b = rotate(conjugate,(0.0,0.0,1.0))
            fh = tuple(v/nf for v in f)
            dot = max(-1.0,min(1.0,sum(a*b for a,b in zip(fh,up_b))))
            if math.acos(dot) <= self.innovation_limit_rad:
                correction = _cross(fh,up_b)
                wc = tuple(a+self.k_a*b for a,b in zip(w,correction))
                feedback_ok = True
        # First-order quaternion integration plus normalization.
        dq = _multiply(self.q_nb,(0.0,*wc))
        self.q_nb = _unit(tuple(a+.5*dt*b for a,b in zip(self.q_nb,dq)))
        rf = rotate(self.q_nb,f)
        self.a_n = (rf[0],rf[1],rf[2]-G)
        old_v = self.v_n
        self.p_n = tuple(p+v*dt+.5*a*dt*dt
                         for p,v,a in zip(self.p_n,old_v,self.a_n))
        self.v_n = tuple(v+a*dt for v,a in zip(old_v,self.a_n))
        reason = "OK"
        if self.k_a > 0 and not feedback_ok:
            reason = "ACCEL_FEEDBACK_REJECTED"
        if stationary_ok:
            # ZUPT cannot undo the position error accumulated before this update.
            self.v_n = (0.0,0.0,0.0)
            alpha = dt/(self.gyro_bias_tau_s+dt)
            self.gyro_bias = tuple(b+alpha*(raw-b)
                                   for b,raw in zip(self.gyro_bias,w_m))
            reason = "ZUPT"
        elif sample.stationary:
            reason = "STATIONARY_REJECTED"
        if wheel_delta is not None:
            ds = (dl+dr)/2
            dg = w[2]*dt  # Level-only approximation, independent of pure q_nb.
            de = (dr-dl)/self.wheelbase_m
            disagreement = abs((dg-de)/dt) > self.angular_disagreement
            weight = 1.0 if disagreement else self.gyro_weight
            dpsi = weight*dg+(1-weight)*de
            mid = self._odom["yaw_rad"]+dpsi/2
            self._odom["x_m"] += ds*math.cos(mid)
            self._odom["y_m"] += ds*math.sin(mid)
            self._odom["yaw_rad"] = _wrap(self._odom["yaw_rad"]+dpsi)
            self._odom["speed_m_s"] = ds/dt
            self._odom["quality"] = ("angular_disagreement" if disagreement
                                     else "relative_dead_reckoning")
        elif have_counts:
            self._odom["speed_m_s"] = 0.0  # Baseline placeholder, not a speed observation.
            self._odom["quality"] = "encoder_rebaselined_no_motion_update"
        else:
            self._odom_incomplete = True
        self._baseline(t,sample)
        quality = "relative_dead_reckoning" if have_counts else "pure_imu_drift_demo"
        return self._packet(t,True,reason,quality)


def stationary_gyro_bias(samples: Iterable[IMUSample], min_samples: int = 20,
                         max_accel_fraction: float = .05,
                         max_gyro_norm_rad_s: float = .2,
                         max_std_rad_s: float = .01) -> Vec3:
    """Estimate gyro bias in a caller-confirmed stationary window.

    Requires stationary=True on every sample, monotonic timestamps, unchanged
    available encoder counts, gravity-like acceleration, and low gyro dispersion.
    This calibrates gyro bias only; never subtract the static acceleration mean
    from all three accelerometer axes, because that would subtract gravity.
    """
    if (type(min_samples) is not int or min_samples < 2 or
        not all(_finite(v) and v > 0 for v in
                (max_accel_fraction,max_gyro_norm_rad_s,max_std_rad_s))):
        raise ValueError("invalid static calibration settings")
    data = list(samples)
    if len(data) < min_samples:
        raise ValueError("insufficient stationary samples")
    gyros = []
    previous_time = None
    previous_counts = None
    for s in data:
        if not isinstance(s,IMUSample) or not _finite(s.t_s) or s.stationary is not True:
            raise ValueError("stationary samples with finite times are required")
        if previous_time is not None and s.t_s <= previous_time:
            raise ValueError("calibration timestamps must increase")
        f,w = _vector(s.accel_m_s2,3),_vector(s.gyro_rad_s,3)
        if abs(_norm(f)-G) > max_accel_fraction*G or _norm(w) > max_gyro_norm_rad_s:
            raise ValueError("stationary IMU gate failed")
        if any(c is not None and type(c) is not int for c in (s.count_left,s.count_right)):
            raise ValueError("invalid calibration encoder counts")
        counts = (s.count_left,s.count_right)
        if previous_counts is not None and counts != previous_counts:
            raise ValueError("encoder counts changed in static window")
        previous_time,previous_counts = s.t_s,counts
        gyros.append(w)
    mean = tuple(sum(w[i] for w in gyros)/len(gyros) for i in range(3))
    std = tuple(math.sqrt(sum((w[i]-mean[i])**2 for w in gyros)/(len(gyros)-1))
                for i in range(3))
    if max(std) > max_std_rad_s:
        raise ValueError("gyro dispersion too large for static calibration")
    return mean
