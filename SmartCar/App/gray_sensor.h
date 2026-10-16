#ifndef GRAY_SENSOR_H
#define GRAY_SENSOR_H
#include <stdint.h>
typedef struct {
    uint16_t raw[16], percent[16], strength[16];
    int16_t error;
    uint32_t sequence, scan_cycles;
    uint8_t calibration_valid, line_valid, acquisition_status;
} gray_state_t;
void Gray_Init(void);
void Gray_ResetSequence(void);
void Gray_Update(void);
const gray_state_t *Gray_GetState(void);
uint16_t Gray_ValidateCalibration(const int16_t *black, const int16_t *white);
uint8_t Gray_CheckCalibration(uint8_t report);
uint16_t Gray_Normalize(uint16_t raw, int16_t black, int16_t white);
int16_t Gray_ComputeError(const uint16_t *strength, uint8_t *line_valid);
const int16_t *Gray_GetBlack(void);
const int16_t *Gray_GetWhite(void);
#endif
