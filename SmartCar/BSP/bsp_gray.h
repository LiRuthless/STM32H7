#ifndef BSP_GRAY_H
#define BSP_GRAY_H
#include "bsp_adc.h"
typedef struct {
    uint16_t raw[16];
    uint32_t sequence, scan_cycles;
} bsp_gray_frame_t;
void BSP_Gray_Init(void);
void BSP_Gray_ResetSequence(void);
bsp_gray_status_t BSP_Gray_ReadFrame(bsp_gray_frame_t *out);
#endif
