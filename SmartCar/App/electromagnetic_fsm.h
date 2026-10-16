#ifndef ELECTROMAGNETIC_FSM_H
#define ELECTROMAGNETIC_FSM_H
#include "control.h"
typedef struct {
    const int16_t *adc;
    uint16_t dl1b_mm;
    float accel, angle_delta;
    uint8_t enabled;
} electromagnetic_inputs_t;
typedef struct {
    uint8_t kernel_state, roundabout_state, left_flag, right_flag, cask_flag;
    uint8_t reset_distance, zero_angle;
    int32_t left_distance, right_distance;
    float angle;
    int16_t error, symmetry_x, symmetry_y, output, steering;
    control_profile_t configuration;
} electromagnetic_state_t;
void ElectromagneticFSM_Reset(void);
void ElectromagneticFSM_Update(const electromagnetic_inputs_t *inputs);
void ElectromagneticFSM_Accumulate(int16_t left, int16_t right);
const electromagnetic_state_t *ElectromagneticFSM_GetState(void);
#endif
