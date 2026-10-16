#ifndef OPTICAL_FSM_H
#define OPTICAL_FSM_H
#include "pid.h"
typedef enum { OPTICAL_WAIT=0, OPTICAL_TRACKING=1, OPTICAL_LOST=2, OPTICAL_STOPPED=3 } optical_phase_t;
typedef struct {
    uint8_t running, gray_valid, em_valid, stop_reason;
    int32_t time;
    int16_t error, base_speed;
    float accel;
    pid_track_gains_t gains;
} optical_inputs_t;
typedef struct { optical_phase_t state; int16_t error, output, base_speed; } optical_state_t;
void OpticalFSM_Reset(void);
void OpticalFSM_Update(const optical_inputs_t *inputs);
const optical_state_t *OpticalFSM_GetState(void);
#endif
