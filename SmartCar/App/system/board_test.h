/**
 * @file board_test.h
 * @brief 独立板测入口及IRQ分派。
 * @details 声明测试专用接口，状态和完整实现位于board_test.c。
 */
#ifndef SMARTCAR_APP_SYSTEM_BOARD_TEST_H
#define SMARTCAR_APP_SYSTEM_BOARD_TEST_H
#include "system/board_test_config.h"
/** @brief 测试专用入口；见board_test.c::BoardTest_Run。 */
void BoardTest_Run(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_RunLcd。 */
void BoardTest_RunLcd(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_RunInductanceHL。 */
void BoardTest_RunInductanceHL(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_RunInductanceVL。 */
void BoardTest_RunInductanceVL(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_RunInductanceVR。 */
void BoardTest_RunInductanceVR(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_RunInductanceHR。 */
void BoardTest_RunInductanceHR(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_RunGray。 */
void BoardTest_RunGray(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_RunGyro。 */
void BoardTest_RunGyro(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_RunAccel。 */
void BoardTest_RunAccel(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_RunDl1b。 */
void BoardTest_RunDl1b(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_RunEncoderLeft。 */
void BoardTest_RunEncoderLeft(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_RunEncoderRight。 */
void BoardTest_RunEncoderRight(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_RunBattery。 */
void BoardTest_RunBattery(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_RunAdcKey1。 */
void BoardTest_RunAdcKey1(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_RunAdcKey2。 */
void BoardTest_RunAdcKey2(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_RunPc13。 */
void BoardTest_RunPc13(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_ControlISR。 */
void BoardTest_ControlISR(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_TaskISR。 */
void BoardTest_TaskISR(void);
/** @brief 测试专用入口；见board_test.c::BoardTest_SampleISR。 */
void BoardTest_SampleISR(void);
#endif
