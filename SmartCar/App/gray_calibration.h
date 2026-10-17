/**
 * @file gray_calibration.h
 * @brief App 十六路实测标定表；仅供算法使用，生产值保持未标定。
 * @details 保存十六路黑底与白线手工标定常量；当前 -1 哨兵保持起跑门禁，不提供示例标定值。
 */
#ifndef SMARTCAR_APP_GRAY_CALIBRATION_H
#define SMARTCAR_APP_GRAY_CALIBRATION_H
#include <stdint.h>
/* 用户实测填写位置：ch0 最左、ch15 最右，12bit ADC 0..4095。
 * 每路黑白差绝对值须 >=200；-1 表示未填写，两种模式均禁止起跑。
 * 不要用示例数据解除门禁。 */
static const int16_t s_gray_black[16] =
{
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    /* ch0..7 黑底 */ -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1  /* ch8..15 黑底 */
};
static const int16_t s_gray_white[16] =
{
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    /* ch0..7 白线 */ -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1  /* ch8..15 白线 */
};

#endif
