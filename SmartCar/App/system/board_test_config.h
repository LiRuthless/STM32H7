/**
 * @file board_test_config.h
 * @brief 编译期单项板测选择。
 * @details 定义单项测试编号及唯一选择；NONE进入正常路径，越界或未定义标识符编译失败。
 */
#ifndef SMARTCAR_APP_SYSTEM_BOARD_TEST_CONFIG_H
#define SMARTCAR_APP_SYSTEM_BOARD_TEST_CONFIG_H

#define BOARD_TEST_NONE 0
#define BOARD_TEST_LCD 1
#define BOARD_TEST_IND_HL 2
#define BOARD_TEST_IND_VL 3
#define BOARD_TEST_IND_VR 4
#define BOARD_TEST_IND_HR 5
#define BOARD_TEST_GRAY 6
#define BOARD_TEST_GYRO 7
#define BOARD_TEST_ACCEL 8
#define BOARD_TEST_DL1B 9
#define BOARD_TEST_ENCODER_LEFT 10
#define BOARD_TEST_ENCODER_RIGHT 11
#define BOARD_TEST_BATTERY 12
#define BOARD_TEST_ADC_KEY1 13
#define BOARD_TEST_ADC_KEY2 14
#define BOARD_TEST_PC13 15

#ifndef BOARD_TEST_SELECT
#define BOARD_TEST_SELECT BOARD_TEST_NONE
#endif
#if BOARD_TEST_SELECT < 0 || BOARD_TEST_SELECT > 15
#error Invalid_BOARD_TEST_SELECT
#endif
typedef char board_test_selection_valid_t[
    (BOARD_TEST_SELECT >= BOARD_TEST_NONE && BOARD_TEST_SELECT <= BOARD_TEST_PC13) ? 1 : -1];
#endif
