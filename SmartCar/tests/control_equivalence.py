"""Compare the real pre-refactor and current control functions on fixed inputs.

Run from any directory: python SmartCar/tests/control_equivalence.py
The baseline is the committed Phase 11 firmware, before Phase 13 edits.
"""

from pathlib import Path
import difflib
import re
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / "SmartCar" / "App"
BASELINE = "60b9f4a"

STUB_BSP = r"""
#ifndef TEST_BSP_H
#define TEST_BSP_H
#include <stdint.h>
typedef enum {
    BSP_ADC_IND_H_L, BSP_ADC_IND_V_L, BSP_ADC_IND_V_R, BSP_ADC_IND_H_R
} bsp_adc_channel_t;
uint16_t BSP_ADC_Read(bsp_adc_channel_t ch);
void BSP_ADC_Init(void);
uint8_t BSP_IMU660RB_Init(void);
void BSP_IMU660RB_GetAcc(int16_t *x, int16_t *y, int16_t *z);
void BSP_IMU660RB_GetGyro(int16_t *x, int16_t *y, int16_t *z);
void BSP_Sampler_GetGyroRaw(int16_t *x, int16_t *y, int16_t *z);
int16_t BSP_Sampler_ConsumeEncL(void);
int16_t BSP_Sampler_ConsumeEncR(void);
void BSP_Encoder_Init(void);
void BSP_PWM_Init(void);
void BSP_PWM_SetDuty(int channel, uint32_t duty);
enum { BSP_PWM_MOTOR_L, BSP_PWM_MOTOR_R, BSP_PWM_FAN };
enum { GPIO_PIN_RESET, GPIO_PIN_SET };
#define MOTOR_L_DIR_GPIO_Port ((void *)1)
#define MOTOR_R_DIR_GPIO_Port ((void *)2)
#define MOTOR_L_DIR_Pin 1u
#define MOTOR_R_DIR_Pin 2u
void HAL_GPIO_WritePin(void *port, uint16_t pin, int state);
#endif
"""

HARNESS = r"""
#include <stdint.h>
#include <stdio.h>
#include "bsp.h"
#include "pid.h"
#include "track_sensor.h"
#include "imu_proc.h"
#include "motor.h"
static uint16_t adc_input[4];
static int16_t gyro_input[3];
static int16_t encoder_input[2];
uint16_t BSP_ADC_Read(bsp_adc_channel_t ch) { return adc_input[ch]; }
void BSP_ADC_Init(void) {}
int16_t BSP_Sampler_ConsumeEncL(void) { int16_t v = encoder_input[0]; encoder_input[0] = 0; return v; }
int16_t BSP_Sampler_ConsumeEncR(void) { int16_t v = encoder_input[1]; encoder_input[1] = 0; return v; }
void BSP_Encoder_Init(void) {}
void BSP_PWM_Init(void) {}
void BSP_PWM_SetDuty(int channel, uint32_t duty) { (void)channel; (void)duty; }
void HAL_GPIO_WritePin(void *port, uint16_t pin, int state)
{ (void)port; (void)pin; (void)state; }
uint8_t BSP_IMU660RB_Init(void) { return 0; }
void BSP_IMU660RB_GetAcc(int16_t *x, int16_t *y, int16_t *z)
{ *x = 0; *y = 0; *z = 0; }
void BSP_IMU660RB_GetGyro(int16_t *x, int16_t *y, int16_t *z)
{ *x = gyro_input[0]; *y = gyro_input[1]; *z = gyro_input[2]; }
void BSP_Sampler_GetGyroRaw(int16_t *x, int16_t *y, int16_t *z)
{ BSP_IMU660RB_GetGyro(x, y, z); }

static void set_wheels(int16_t tl, int16_t tr, int16_t ml, int16_t mr)
{
#ifdef REFACTORED
    Motor_SetTargets(tl, tr);
#else
    target_speed_L = tl; target_speed_R = tr;
#endif
    encoder_input[0] = ml; encoder_input[1] = mr;
    read_encoder();
}

int main(void)
{
    static const uint16_t samples[][4] = {
        {0, 0, 0, 0}, {2000, 1500, 1500, 2000},
        {1000, 2000, 300, 500}, {4095, 0, 0, 500},
        {0, 1900, 1900, 0}
    };
    static const int16_t wheels[][4] = {
        {0, 0, 0, 0}, {245, 245, 0, 0}, {500, 500, 100, 50},
        {2000, 2000, 0, 0}, {2000, -2000, 0, 0},
        {-500, 500, 100, -100}, {0, 0, 200, -200},
        {245, 245, 0, 0}, {-245, -245, 0, 0}
    };
    static const int16_t gyros[][3] = {
        {0, 0, 0}, {143, 286, -143}, {143, 286, -143},
        {-143, 0, 143}, {0, -286, 0}, {0, 0, 0}
    };
    imu_proc_init();
    gyro_calibrate();
    encoder_init();
#ifdef REFACTORED
    const track_weights_t weights = {15, 20, 22, 10};
    const pid_track_gains_t track_gains = {2.0f, 0.008f, 15.0f, 1.0f};
    Track_SetWeights(&weights);
    PID_SetTrackGains(&track_gains);
    PID_SetGain(PID_GAIN_KP_V, 20.0f);
    PID_SetGain(PID_GAIN_KI_V, 0.75f);
#else
    weight_x = 15; weight_xx = 20; weight_y = 22; weight_abs = 10;
    KP_x = 2.0f; K2P_x = 0.008f; KD_x = 15.0f; K2D_x = 1.0f;
    KP_v = 20.0f; KI_v = 0.75f;
#endif
    PID_ResetAll();
    for(unsigned i = 0; i < sizeof(samples)/sizeof(samples[0]); ++i)
    {
        for(unsigned j = 0; j < 4; ++j) adc_input[j] = samples[i][j];
        read_adc();
        int16_t error = get_track_error();
#ifndef REFACTORED
        track_error = error;
#endif
        symmetry_adc();
#ifdef REFACTORED
        printf("track %u %d %d %d %d\n", i, error,
               Track_GetState()->symmetry_x, Track_GetState()->symmetry_y,
               PID_track());
#else
        printf("track %u %d %d %d %d\n", i, error,
               symmetry_x, symmetry_y, PID_track());
#endif
    }
    for(unsigned i = 0; i < sizeof(wheels)/sizeof(wheels[0]); ++i)
    {
        if(i == 7) { Motor_ResetRunState(); PID_ResetAll(); }
        set_wheels(wheels[i][0], wheels[i][1], wheels[i][2], wheels[i][3]);
        int16_t left = PID_L_pos();
        int16_t right = PID_R_pos();
#ifdef REFACTORED
        printf("wheel %u %d %d %a %a\n", i, left, right,
               PID_GetState()->out_l, PID_GetState()->out_r);
#else
        printf("wheel %u %d %d %a %a\n", i, left, right, PID_outL, PID_outR);
#endif
    }
#ifdef REFACTORED
    Motor_SetBaseSpeed(245);
#else
    base_speed = 245;
#endif
    speed_control(100);
#ifdef REFACTORED
    printf("split %d %d %ld\n", Motor_GetState()->left.target_speed,
           Motor_GetState()->right.target_speed, (long)Motor_GetState()->distance);
#else
    printf("split %d %d %ld\n", target_speed_L, target_speed_R, (long)Distance);
#endif
    speed_control(-100);
#ifdef REFACTORED
    printf("split %d %d\n", Motor_GetState()->left.target_speed,
           Motor_GetState()->right.target_speed);
#else
    printf("split %d %d\n", target_speed_L, target_speed_R);
#endif
    for(unsigned i = 0; i < sizeof(gyros)/sizeof(gyros[0]); ++i)
    {
        for(unsigned j = 0; j < 3; ++j) gyro_input[j] = gyros[i][j];
        read_gyro_angle();
#ifdef REFACTORED
        const imu_state_t *state = IMU_GetState();
        printf("imu %u %a %a %a %a %a %a\n", i,
               state->gyro[0], state->gyro[1], state->gyro[2],
               state->angle[0], state->angle[1], state->angle[2]);
#else
        printf("imu %u %a %a %a %a %a %a\n", i,
               gyro_x, gyro_y, gyro_z, angle_x, angle_y, angle_z);
#endif
    }
    IMU_ResetRunState();
#ifdef REFACTORED
    printf("reset %a %a\n", IMU_GetState()->gyro[1], IMU_GetState()->angle[1]);
#else
    printf("reset %a %a\n", gyro_y, angle_y);
#endif
    return 0;
}
"""


def run(args, **kwargs):
    return subprocess.run(args, check=True, capture_output=True, text=True, **kwargs)


def check_profiles():
    fields = ("weight_x", "weight_xx", "weight_y", "weight_abs", "base_speed",
              "KP_x", "K2P_x", "KD_x", "K2D_x", "KP_a", "KD_a", "KG_a")
    groups = (
        ("control.c", ("KERNEL_TRACKING", "KERNEL_ISLAND_L", "KERNEL_ISLAND_R",
                       "KERNEL_TEETERBOARD", "KERNEL_CROSSROADS", "KERNEL_CASK",
                       "KERNEL_REISLAND")),
        ("roundabout.c", ("ISLAND_LPREENTER", "ISLAND_TURN_LEFT", "ISLAND_IN",
                          "ISLAND_OUT")),
    )
    for filename, states in groups:
        old = run(["git", "show", f"{BASELINE}:SmartCar/App/{filename}"], cwd=ROOT).stdout
        new = (APP / filename).read_text(encoding="utf-8")
        for state in states:
            old_case = re.search(r"\bcase\s+" + state + r"\s*:(.*?)\bbreak\s*;", old, re.S)
            assert old_case, (filename, state, "old case")
            old_values = {}
            for field, value in re.findall(r"\b(" + "|".join(fields) +
                                           r")\s*=\s*([0-9.]+)\s*;", old_case.group(1)):
                old_values[field] = float(value)
            if filename == "roundabout.c":
                profile = {"ISLAND_LPREENTER": "ROUND_PROFILE_PREENTER",
                           "ISLAND_TURN_LEFT": "ROUND_PROFILE_TURN",
                           "ISLAND_IN": "ROUND_PROFILE_IN",
                           "ISLAND_OUT": "ROUND_PROFILE_OUT"}[state]
            else:
                profile = state
            entry = re.search(r"\[" + profile + r"\]\s*=\s*\{(.*?)\n\s*\}", new, re.S)
            assert entry, (filename, profile, "new entry")
            payload = entry.group(1)
            if profile == "ROUND_PROFILE_TURN":
                new_values = {}
            else:
                flags, remainder = payload.strip().split(",", 1)
                values = re.search(r"\{([^}]*)\}\s*,\s*([0-9.]+)\s*,\s*"
                                   r"\{([^}]*)\}\s*,\s*\{([^}]*)\}", remainder)
                assert values, (filename, profile, "values")
                weights, base, track, angle = values.groups()
                parse = lambda items: [float(value.strip().removesuffix("f"))
                                       for value in items.split(",")]
                new_values = {}
                if "CONTROL_PROFILE_WEIGHTS" in flags:
                    new_values.update(zip(fields[:4], parse(weights)))
                if "CONTROL_PROFILE_BASE" in flags:
                    new_values["base_speed"] = float(base)
                if "CONTROL_PROFILE_TRACK" in flags:
                    new_values.update(zip(fields[5:9], parse(track)))
                if "CONTROL_PROFILE_ANGLE" in flags:
                    new_values.update(zip(fields[9:], parse(angle)))
            if old_values != new_values:
                raise AssertionError((filename, state, old_values, new_values))
            body = re.search(r"\bcase\s+" + state + r"\s*:(.*?)\bbreak\s*;", new, re.S)
            assert body and "Control_ApplyProfile" in body.group(1), (filename, state, "call")
    print("PASS: 7 kernel and 4 roundabout profiles preserve assigned fields and values")


def check_wire_formats():
    old_param = run(["git", "show", f"{BASELINE}:SmartCar/App/param.c"], cwd=ROOT).stdout
    new_param = (APP / "param.c").read_text(encoding="utf-8")
    layouts = r"typedef struct\s*\{[^}]*\}\s*(param_store_v1_t|param_store_t);"
    old_layouts = {name: body for body, name in re.findall(r"(" + layouts + r")", old_param)}
    new_layouts = {name: body for body, name in re.findall(r"(" + layouts + r")", new_param)}
    assert old_layouts == new_layouts and len(old_layouts) == 2, "parameter layouts changed"
    for function in ("param_crc_v1", "param_crc"):
        pattern = r"static uint16_t " + function + r"\([^)]*\)\s*\{.*?^\}"
        old_crc = re.search(pattern, old_param, re.S | re.M)
        new_crc = re.search(pattern, new_param, re.S | re.M)
        assert old_crc and new_crc and old_crc.group() == new_crc.group(), function
    old_log = run(["git", "show", f"{BASELINE}:SmartCar/App/datalog.h"], cwd=ROOT).stdout
    assert old_log == (APP / "datalog.h").read_text(encoding="utf-8"), "log record changed"
    old_wireless = run(["git", "show", f"{BASELINE}:SmartCar/App/wireless.c"], cwd=ROOT).stdout
    new_wireless = (APP / "wireless.c").read_text(encoding="utf-8")
    assert re.findall(r"\bcase\s+'([a-z])'", old_wireless) == re.findall(
        r"\bcase\s+'([a-z])'", new_wireless), "serial commands changed"
    assert 'if(len < 4 || len > 7)' in new_wireless
    print("PASS: parameter layouts/CRC, log record and serial command set unchanged")


def main():
    compiler = shutil.which("gcc")
    if compiler is None:
        raise SystemExit("host gcc not found")
    with tempfile.TemporaryDirectory(prefix="control_equiv_") as temp:
        work = Path(temp)
        (work / "bsp.h").write_text(STUB_BSP, encoding="utf-8")
        (work / "harness.c").write_text(HARNESS, encoding="utf-8")
        baseline = work / "baseline"
        baseline.mkdir()
        for name in ("pid.c", "pid.h", "track_sensor.c", "track_sensor.h",
                     "imu_proc.c", "imu_proc.h", "filter.c", "filter.h",
                     "motor.c", "motor.h"):
            source = run(["git", "show", f"{BASELINE}:SmartCar/App/{name}"], cwd=ROOT).stdout
            (baseline / name).write_text(source, encoding="utf-8")
        outputs = []
        for label, source_dir, extra in (
            ("baseline", baseline, []), ("current", APP, ["-DREFACTORED"])
        ):
            binary = work / (label + ".exe")
            cmd = [compiler, "-std=c99", "-O2", "-Wall", "-Wextra", *extra,
                   "-I", str(work), "-I", str(source_dir), "-I", str(APP),
                   str(work / "harness.c"), str(source_dir / "pid.c"),
                   str(source_dir / "track_sensor.c"),
                   str(source_dir / "imu_proc.c"), str(source_dir / "filter.c"),
                   str(source_dir / "motor.c"),
                   "-o", str(binary)]
            run(cmd, cwd=ROOT)
            outputs.append(run([str(binary)], cwd=ROOT).stdout)
        if outputs[0] != outputs[1]:
            print("".join(difflib.unified_diff(outputs[0].splitlines(True),
                                               outputs[1].splitlines(True),
                                               fromfile="baseline", tofile="current")))
            raise SystemExit(1)
        print("PASS: 5 sensor, 9 wheel, 2 speed split and 6 IMU steps match the Phase 11 baseline")
    check_profiles()
    check_wire_formats()


if __name__ == "__main__":
    main()
