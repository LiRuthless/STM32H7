
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
    Motor_ReadEncoder();
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
    IMU_Init();
    IMU_CalibrateGyro();
    Motor_InitEncoder();
#ifdef REFACTORED
    const track_weights_t weights = {15, 20, 22, 10};
    const pid_track_gains_t track_gains = {2.0f, 0.008f, 15.0f, 1.0f};
    Track_SetWeights(&weights);
    PID_SetTrackGains(&track_gains);
    PID_SetGain(PID_GAIN_KP_V, 20.0f);
    PID_SetGain(PID_GAIN_KI_V, 0.75f);
#else
    weight_x = 15; weight_xx = 20; weight_y = 22; weight_abs = 10;
    kp_x = 2.0f; k2p_x = 0.008f; kd_x = 15.0f; k2d_x = 1.0f;
    kp_v = 20.0f; ki_v = 0.75f;
#endif
    PID_ResetAll();
    for(unsigned i = 0; i < sizeof(samples)/sizeof(samples[0]); ++i)
    {
        for(unsigned j = 0; j < 4; ++j) adc_input[j] = samples[i][j];
        Track_ReadAdc();
        int16_t error = Track_UpdateError();
#ifndef REFACTORED
        track_error = error;
#endif
        Track_UpdateSymmetry();
#ifdef REFACTORED
        printf("track %u %d %d %d %d\n", i, error,
               Track_GetState()->symmetry_x, Track_GetState()->symmetry_y,
               PID_UpdateTrack());
#else
        printf("track %u %d %d %d %d\n", i, error,
               symmetry_x, symmetry_y, PID_UpdateTrack());
#endif
    }
    for(unsigned i = 0; i < sizeof(wheels)/sizeof(wheels[0]); ++i)
    {
        if(i == 7) { Motor_ResetRunState(); PID_ResetAll(); }
        set_wheels(wheels[i][0], wheels[i][1], wheels[i][2], wheels[i][3]);
        int16_t left = PID_UpdateSpeedLeft();
        int16_t right = PID_UpdateSpeedRight();
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
    Motor_DistributeSpeed(100);
#ifdef REFACTORED
    printf("split %d %d %ld\n", Motor_GetState()->left.target_speed,
           Motor_GetState()->right.target_speed, (long)Motor_GetState()->distance);
#else
    printf("split %d %d %ld\n", target_speed_L, target_speed_R, (long)Distance);
#endif
    Motor_DistributeSpeed(-100);
#ifdef REFACTORED
    printf("split %d %d\n", Motor_GetState()->left.target_speed,
           Motor_GetState()->right.target_speed);
#else
    printf("split %d %d\n", target_speed_L, target_speed_R);
#endif
    for(unsigned i = 0; i < sizeof(gyros)/sizeof(gyros[0]); ++i)
    {
        for(unsigned j = 0; j < 3; ++j) gyro_input[j] = gyros[i][j];
        IMU_UpdateGyroAngle();
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
