/**
 * @file bsp_board_test.h
 * @brief 独立板测最小初始化及发送。
 * @details 独立板测专用接口；正常模式不调用，对应实现见bsp_board_test.c。
 */
#ifndef SMARTCAR_BSP_BSP_BOARD_TEST_H
#define SMARTCAR_BSP_BSP_BOARD_TEST_H
#include <stdint.h>
typedef struct
{
    uint32_t submitted; /**< 已提交UART字节数，不代表PC接收。 */
    uint8_t status; /**< 0完整、1部分、2失败。 */
} bsp_board_test_tx_t;
/** @brief 初始化当前测试；见bsp_board_test.c::BSP_BoardTest_Init。 */
uint8_t BSP_BoardTest_Init(void);
/** @brief 清挂起并启动所选定时器；见bsp_board_test.c::BSP_BoardTest_StartTimer。 */
uint8_t BSP_BoardTest_StartTimer(void);
/** @brief 停止所选定时器；见bsp_board_test.c::BSP_BoardTest_StopTimer。 */
void BSP_BoardTest_StopTimer(void);
/** @brief 前台发送16字节；见bsp_board_test.c::BSP_BoardTest_SendFrame。 */
bsp_board_test_tx_t BSP_BoardTest_SendFrame(const uint8_t *frame);
#endif
