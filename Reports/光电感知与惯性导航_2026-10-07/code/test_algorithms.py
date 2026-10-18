"""以外部算例、物理不变量和异常恢复验证；不证明实车性能。"""
import json
import math
import unittest
from optical import normalize, digital, centroid, difference_over_sum, PID, OpticalController
from inertial import G, IMUSample, Navigator, stationary_gyro_bias

Q = [.05,.10,.20,.80,.90]
RAW = [140,180,260,740,820]
def sample(t, ax=0., gz=0., left=None, right=None, stationary=False):
    return IMUSample(t, (ax,0.,G), (0.,0.,gz), left, right, stationary)

class OpticalTests(unittest.TestCase):
    def test_report_calibration_and_polarity(self):
        self.assertEqual(normalize(RAW,[100]*5,[900]*5), Q)
        self.assertEqual(normalize([860,820,740,260,180],[900]*5,[100]*5), Q)
        self.assertEqual(digital([1,1,0,0,0],0),[0.,0.,1.,1.,1.])
    def test_report_centroid(self):
        r=centroid(Q)
        self.assertTrue(r.valid)
        self.assertAlmostEqual(r.e_y_m,.024/2.05,places=12)
        self.assertAlmostEqual(r.e,.5853658536585366,places=12)
    def test_report_dos(self):
        self.assertAlmostEqual(difference_over_sum(Q).e,.775/.925,places=12)
    def test_symmetry_and_scale(self):
        self.assertAlmostEqual(centroid(Q[::-1]).e,-centroid(Q).e)
        self.assertAlmostEqual(difference_over_sum(Q[::-1]).e,-difference_over_sum(Q).e)
        self.assertAlmostEqual(centroid([q*.5 for q in Q]).e,centroid(Q).e)
        self.assertAlmostEqual(difference_over_sum([q*.5 for q in Q]).e,difference_over_sum(Q).e)
    def test_group_information_loss(self):
        self.assertEqual(difference_over_sum([0,0,0,1,0]).e,1.)
        self.assertEqual(difference_over_sum([0,0,0,0,1]).e,1.)
        self.assertEqual(centroid([0,0,0,1,0]).e,.5)
        self.assertEqual(centroid([0,0,0,0,1]).e,1.)
    def test_center_is_not_lost(self):
        q=[0,0,1,0,0]
        self.assertTrue(centroid(q).valid)
        r=difference_over_sum(q)
        self.assertFalse(r.valid)
        self.assertEqual(r.reason,"CENTER_ONLY")
        out=OpticalController(method="dos").step(0,[100,100,900,100,100])
        self.assertTrue(out["valid"])
        self.assertEqual(out["method"],"centroid_fallback")
    def test_weak_and_zero_denominator(self):
        self.assertFalse(centroid([0]*5).valid)
        self.assertFalse(difference_over_sum([0]*5).valid)
        self.assertFalse(difference_over_sum([.001]*5).valid)
    def test_invalid_normalization(self):
        cases=[([1],[0],[0]),([float('nan')],[0],[900]),([2000],[0],[900]),([1,2],[0],[900])]
        for args in cases:
            with self.subTest(args=args),self.assertRaises(ValueError):normalize(*args)
        with self.assertRaises(ValueError):digital([2])
    def test_pid_report(self):
        p=PID(kp=.8,ki=.1,kd=.02,u_max=.5,tau_d=.02)
        p.step(.2,.01)
        p.integral=0.
        self.assertAlmostEqual(p.step(.3,.01),.30696666666666667,places=10)
    def test_pid_anti_windup(self):
        p=PID(kp=.8,ki=.1,kd=.02,u_max=.25,tau_d=.02)
        p.step(.2,.01);p.integral=0.
        self.assertEqual(p.step(.3,.01),.25)
        self.assertEqual(p.integral,0.)
        for _ in range(100):p.step(1.,.01)
        self.assertEqual(p.integral,0.)
    def test_pid_invalid_dt(self):
        for dt in [0,-.01,.1,float('nan')]:
            with self.subTest(dt=dt),self.assertRaises(ValueError):PID().step(.1,dt)
    def test_controller_stop_and_recover(self):
        c=OpticalController()
        self.assertTrue(c.step(0,RAW)["valid"])
        out=c.step(.01,[100]*5)
        self.assertFalse(out["valid"])
        self.assertEqual(out["v_left_m_s"],0.)
        self.assertEqual(out["u"],0.)
        self.assertTrue(c.step(.02,RAW)["valid"])
    def test_controller_ambiguous(self):
        for raw in [[900]*5,[900,100,100,100,900]]:
            out=OpticalController().step(0,raw)
            self.assertFalse(out["valid"])
            self.assertEqual(out["reason"],"AMBIGUOUS")
    def test_controller_time_recovery(self):
        c=OpticalController();c.step(0,RAW)
        self.assertFalse(c.step(-.01,RAW)["valid"])
        self.assertTrue(c.step(.01,RAW)["valid"])
        self.assertFalse(c.step(.5,RAW)["valid"])
        self.assertTrue(c.step(.51,RAW)["valid"])
    def test_controller_strict_json(self):
        for t,raw in [(float('nan'),RAW),(0,[float('nan')]*5)]:
            out=OpticalController().step(t,raw)
            self.assertFalse(out["valid"])
            json.dumps(out,allow_nan=False)

class InertialTests(unittest.TestCase):
    def test_invalid_timestamp_rebaseline_without_integration(self):
        n=Navigator(k_a=0);n.update(sample(0,ax=1.,left=0,right=0))
        bad=n.update(sample(float('nan'),ax=1.,left=10,right=10))
        self.assertFalse(bad["valid"])
        self.assertIn("incomplete_dead_reckoning",bad["quality"])
        json.dumps(bad,allow_nan=False)
        restored=n.update(sample(.02,ax=1.,left=20,right=20))
        self.assertEqual(restored["reason"],"REBASELINED_AFTER_INVALID_TIME")
        self.assertEqual(restored["v_n_m_s"][0],0.)
        self.assertEqual(restored["p_n_m"][0],0.)
        moved=n.update(sample(.03,ax=1.,left=30,right=30))
        self.assertAlmostEqual(moved["v_n_m_s"][0],.01)
        self.assertAlmostEqual(moved["odom"]["x_m"],.01)
    def test_tilt_initialization_and_gravity(self):
        roll=.2
        f=(0.,G*math.sin(roll),G*math.cos(roll))
        s0=IMUSample(0.,f,(0.,0.,0.),stationary=True)
        n=Navigator();n.initialize_tilt(s0);n.update(s0)
        r=n.update(IMUSample(.01,f,(0.,0.,0.),stationary=True))
        self.assertAlmostEqual(r["roll_rad"],roll,places=10)
        for a in r["a_n_m_s2"]:self.assertAlmostEqual(a,0.,places=10)
    def test_feedback_direction_and_gate(self):
        n=Navigator(initial_q_nb=(math.cos(.05),math.sin(.05),0.,0.))
        n.update(sample(0))
        before=.1
        r=n.update(sample(.01))
        self.assertLess(abs(r["roll_rad"]),before)
        n=Navigator();n.update(sample(0))
        r=n.update(sample(.01,ax=G))
        self.assertEqual(r["reason"],"ACCEL_FEEDBACK_REJECTED")
    def test_trusted_bias_calibration(self):
        data=[IMUSample(k*.01,(0.,0.,G),(.01,-.02,.03),stationary=True) for k in range(20)]
        b=stationary_gyro_bias(data)
        for got,wanted in zip(b,(.01,-.02,.03)):self.assertAlmostEqual(got,wanted)
        with self.assertRaises(ValueError):stationary_gyro_bias([sample(k*.01) for k in range(20)])
    def test_angular_disagreement(self):
        n=Navigator();n.update(sample(0,left=0,right=0))
        o=n.update(sample(.01,left=-10,right=10))["odom"]
        self.assertEqual(o["quality"],"angular_disagreement")
        self.assertEqual(o["yaw_rad"],0.)
    def test_gravity_and_unit_quaternion(self):
        n=Navigator();n.update(sample(0,stationary=True))
        for k in range(1,101):r=n.update(sample(k*.01,stationary=True))
        self.assertTrue(r["valid"])
        self.assertAlmostEqual(sum(v*v for v in r["q_nb"]),1.,places=10)
        for key in ["a_n_m_s2","v_n_m_s","p_n_m"]:
            for v in r[key]:self.assertAlmostEqual(v,0.,places=10)
    def test_constant_acceleration_report(self):
        n=Navigator(k_a=0);n.update(sample(0,ax=1.))
        r=n.update(sample(.01,ax=1.))
        self.assertAlmostEqual(r["v_n_m_s"][0],.01,places=12)
        self.assertAlmostEqual(r["p_n_m"][0],.00005,places=12)
        for k in range(2,101):r=n.update(sample(k*.01,ax=1.))
        self.assertAlmostEqual(r["v_n_m_s"][0],1.,places=10)
        self.assertAlmostEqual(r["p_n_m"][0],.5,places=10)
    def test_encoder_report(self):
        n=Navigator(k_a=0);n.update(sample(0,left=0,right=0))
        r=n.update(sample(.01,gz=1.8,left=8,right=12));o=r["odom"]
        self.assertTrue(r["valid"])
        self.assertAlmostEqual(o["yaw_rad"],.0186,places=12)
        self.assertAlmostEqual(o["speed_m_s"],1.,places=12)
        self.assertAlmostEqual(o["x_m"],.01*math.cos(.0093),places=12)
        self.assertAlmostEqual(o["y_m"],.01*math.sin(.0093),places=12)
        self.assertAlmostEqual(r["pure_yaw_rad"],.018,places=5)
    def test_straight_reverse_and_spin(self):
        for l,r in [(10,10),(-10,-10),(-10,10)]:
            n=Navigator(k_a=0,gyro_weight=0);n.update(sample(0,left=0,right=0))
            out=n.update(sample(.01,gz=10. if l!=r else 0.,left=l,right=r))["odom"]
            if l==r:
                self.assertAlmostEqual(out["x_m"],l*.001)
                self.assertAlmostEqual(out["yaw_rad"],0.)
            else:
                self.assertAlmostEqual(out["x_m"],0.)
                self.assertAlmostEqual(out["y_m"],0.)
                self.assertAlmostEqual(out["yaw_rad"],.1)
    def test_nonmonotonic_and_gap(self):
        n=Navigator();n.update(sample(0,left=0,right=0))
        self.assertFalse(n.update(sample(-.01,left=0,right=0))["valid"])
        self.assertTrue(n.update(sample(.01,left=10,right=10))["valid"])
        self.assertFalse(n.update(sample(.5,left=500,right=500))["valid"])
        out=n.update(sample(.51,left=510,right=510))
        self.assertTrue(out["valid"])
        self.assertAlmostEqual(out["odom"]["x_m"],.02)
    def test_nan_and_overrange(self):
        for bad in [sample(.01,ax=float('nan')),sample(.01,ax=G*20),sample(.01,gz=50)]:
            n=Navigator();n.update(sample(0))
            out=n.update(bad)
            self.assertFalse(out["valid"])
            json.dumps(out,allow_nan=False)
            self.assertTrue(n.update(sample(.02))["valid"])
    def test_encoder_jump_rebaseline(self):
        n=Navigator();n.update(sample(0,left=0,right=0))
        self.assertFalse(n.update(sample(.01,left=1000,right=1000))["valid"])
        o=n.update(sample(.02,left=1010,right=1010))["odom"]
        self.assertAlmostEqual(o["x_m"],.01)
    def test_no_false_stationary_zero_velocity(self):
        n=Navigator(k_a=0);n.update(sample(0,ax=1.))
        r=n.update(sample(.01,ax=1.))
        before=r["v_n_m_s"][0]
        r=n.update(sample(.02))  # 无stationary，低gyro与g幅值不能判停。
        self.assertAlmostEqual(r["v_n_m_s"][0],before)
    def test_reject_stationary_with_motion(self):
        n=Navigator(k_a=0);n.update(sample(0,left=0,right=0))
        r=n.update(sample(.01,gz=1.8,left=8,right=12,stationary=True))
        self.assertAlmostEqual(r["odom"]["speed_m_s"],1.)
        self.assertIn("STATIONARY",r["reason"])
    def test_encoder_missing_recovery(self):
        n=Navigator();n.update(sample(0,left=0,right=0))
        n.update(sample(.01))
        n.update(sample(.02,left=20,right=20))
        r=n.update(sample(.03,left=30,right=30))
        self.assertAlmostEqual(r["odom"]["x_m"],.01)

if __name__=="__main__":unittest.main(verbosity=2)
