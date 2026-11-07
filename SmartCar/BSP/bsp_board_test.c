/**
 * @file bsp_board_test.c
 * @brief 独立板测最小硬件启动。
 * @details 前台按编译选择配置GPIO和所需外设，状态由App先初始化；不启动正常业务。
 *     定时器沿用现有配置和IRQ，串口只在IMU测试发送，不启用接收业务。
 */
#include "bsp.h"
#include "bsp_board_test.h"
#include "system/board_test_config.h"

/**
 * @brief 配置一组测试GPIO。
 * @details 以零初始化配置填写引脚、模式、上下拉及速度字段，再调用HAL应用；不写输出数据寄存器。
 * @param[in] port GPIO端口，必须非空且时钟已开启。
 * @param[in] pins 引脚位掩码。
 * @param[in] mode 本模块使用HAL输入或推挽输出模式。
 * @param[in] pull HAL上下拉配置。
 * @note 仅启动前台调用，不配置EXTI。
 */
static void BSP_BoardTest_InitPins(GPIO_TypeDef *port, uint32_t pins,
    uint32_t mode, uint32_t pull)
{
    GPIO_InitTypeDef config = {0};
    config.Pin = pins;
    config.Mode = mode;
    config.Pull = pull;
    config.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(port, &config);
}

/**
 * @brief 仅初始化当前测试的外设和驱动。
 * @details 先释放SPI4片选，配置必要GPIO，再调用所需MX并恢复SPI45；不调用整体GPIO或业务入口。
 *     编码器仅启用所选一轮；ADC保持既有十rank和DMA存放位置。驱动无返回值时不虚构成功检测。
 * @return 有可检测的初始化失败返回1，否则0；NONE不执行硬件动作。
 * @pre App测试状态已清零，采集定时器尚未启动。
 * @note 前台阻塞初始化；IMU失败静默，显示模式错误由App显示。
 */
uint8_t BSP_BoardTest_Init(void)
{
    if (BOARD_TEST_SELECT == BOARD_TEST_NONE)
    {
        return 0;
    }
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    if (BOARD_TEST_SELECT == BOARD_TEST_PC13)
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
    {
        HAL_GPIO_WritePin(GPIOE, LCD_CS_Pin | LCD2_CS_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOD, IMU_CS_Pin, GPIO_PIN_SET);
        BSP_BoardTest_InitPins(GPIOE, LCD_CS_Pin | LCD2_CS_Pin,
            GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
        BSP_BoardTest_InitPins(GPIOD, IMU_CS_Pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
    }
    if (BOARD_TEST_SELECT != BOARD_TEST_GYRO && BOARD_TEST_SELECT != BOARD_TEST_ACCEL)
    {
        HAL_GPIO_WritePin(GPIOE, LCD_WR_RS_Pin, GPIO_PIN_SET);
        BSP_BoardTest_InitPins(GPIOE, LCD_WR_RS_Pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
        MX_SPI4_Init();
        MX_TIM1_Init();
    }
    else
    {
        MX_SPI4_Init();
        MX_USART1_UART_Init();
        HAL_NVIC_DisableIRQ(USART1_IRQn);
        huart1.Init.BaudRate = 460800;
        huart1.Init.Mode = UART_MODE_TX;
        if (HAL_UART_Init(&huart1) != HAL_OK)
        {
            return 1;
        }
    }
    BSP_Runtime_ConfigureSpiClock();
    BSP_Time_Init();
    if (BOARD_TEST_SELECT != BOARD_TEST_GYRO && BOARD_TEST_SELECT != BOARD_TEST_ACCEL)
    {
        BSP_LCD_Init();
    }
    if ((BOARD_TEST_SELECT >= BOARD_TEST_IND_HL && BOARD_TEST_SELECT <= BOARD_TEST_GRAY)
        || (BOARD_TEST_SELECT >= BOARD_TEST_BATTERY && BOARD_TEST_SELECT <= BOARD_TEST_ADC_KEY2))
    {
        /* 整体DMA入口会额外启用Wi-Fi的Stream1/2；仅保留ADC的Stream0及原优先级。 */
        __HAL_RCC_DMA1_CLK_ENABLE();
        HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
        MX_ADC1_Init();
        BSP_ADC_Init();
    }
    if (BOARD_TEST_SELECT == BOARD_TEST_GRAY)
    {
        BSP_BoardTest_InitPins(GPIOE,
            GRAY_IO1_Pin | GRAY_IO2_Pin | GRAY_IO3_Pin | GRAY_IO4_Pin,
            GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
        BSP_Gray_Init();
    }
    if (BOARD_TEST_SELECT == BOARD_TEST_PC13)
    {
        BSP_BoardTest_InitPins(KEY_GPIO_Port, KEY_Pin, GPIO_MODE_INPUT, GPIO_PULLDOWN);
        BSP_Key_Init();
    }
    if (BOARD_TEST_SELECT == BOARD_TEST_ENCODER_LEFT)
    {
        MX_TIM5_Init();
        __HAL_TIM_SET_COUNTER(&htim5, 0);
        if (HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL) != HAL_OK)
        {
            return 1;
        }
    }
    if (BOARD_TEST_SELECT == BOARD_TEST_ENCODER_RIGHT)
    {
        MX_TIM3_Init();
        __HAL_TIM_SET_COUNTER(&htim3, 0);
        if (HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL) != HAL_OK)
        {
            return 1;
        }
    }
    if (BOARD_TEST_SELECT == BOARD_TEST_DL1B)
    {
        BSP_BoardTest_InitPins(GPIOE, DL1B_XS_Pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
        MX_I2C2_Init();
        if (BSP_DL1B_Init() != 0)
        {
            return 1;
        }
    }
    if (BOARD_TEST_SELECT == BOARD_TEST_GYRO || BOARD_TEST_SELECT == BOARD_TEST_ACCEL)
    {
        if (BSP_IMU660RB_Init() != 0)
        {
            return 1;
        }
    }
    if ((BOARD_TEST_SELECT >= BOARD_TEST_IND_HL && BOARD_TEST_SELECT <= BOARD_TEST_GRAY)
        || BOARD_TEST_SELECT == BOARD_TEST_DL1B)
    {
        MX_TIM6_Init();
    }
    if (BOARD_TEST_SELECT == BOARD_TEST_BATTERY)
    {
        MX_TIM7_Init();
    }
    if (BOARD_TEST_SELECT == BOARD_TEST_GYRO || BOARD_TEST_SELECT == BOARD_TEST_ACCEL
        || BOARD_TEST_SELECT == BOARD_TEST_ENCODER_LEFT
        || BOARD_TEST_SELECT == BOARD_TEST_ENCODER_RIGHT)
    {
        BSP_Sampler_Init();
    }
    /* 最后一个所需MX之后再次恢复，避免外设时钟初始化覆盖SPI45选择。 */
    BSP_Runtime_ConfigureSpiClock();
    return 0;
}

/**
 * @brief 清更新标志和NVIC挂起后启动所选采集定时器。
 * @details HAL定时器启用失败返回错误；TIM15复用既有采样器启动接口，不执行其综合采集体。
 * @return 成功或无采集定时器返回0，HAL启用失败返回1。
 * @pre 全部采集状态及外设已初始化。
 * @note 仅测试前台调用，不捆绑启动其他定时器。
 */
uint8_t BSP_BoardTest_StartTimer(void)
{
    if ((BOARD_TEST_SELECT >= BOARD_TEST_IND_HL && BOARD_TEST_SELECT <= BOARD_TEST_GRAY)
        || BOARD_TEST_SELECT == BOARD_TEST_DL1B)
    {
        __HAL_TIM_CLEAR_FLAG(&htim6, TIM_FLAG_UPDATE);
        HAL_NVIC_ClearPendingIRQ(TIM6_DAC_IRQn);
        return HAL_TIM_Base_Start_IT(&htim6) != HAL_OK;
    }
    if (BOARD_TEST_SELECT == BOARD_TEST_BATTERY)
    {
        __HAL_TIM_CLEAR_FLAG(&htim7, TIM_FLAG_UPDATE);
        HAL_NVIC_ClearPendingIRQ(TIM7_IRQn);
        return HAL_TIM_Base_Start_IT(&htim7) != HAL_OK;
    }
    if (BOARD_TEST_SELECT == BOARD_TEST_GYRO || BOARD_TEST_SELECT == BOARD_TEST_ACCEL
        || BOARD_TEST_SELECT == BOARD_TEST_ENCODER_LEFT
        || BOARD_TEST_SELECT == BOARD_TEST_ENCODER_RIGHT)
    {
        HAL_NVIC_ClearPendingIRQ(TIM15_IRQn);
        BSP_Sampler_Start();
    }
    return 0;
}

/**
 * @brief 停止当前测试采集。
 * @details 仅所选定时器停止，不更改普通运行状态；保留全部测试快照和队列。
 * @note 前台发送失败或TIM15溢出/观察结束时调用。
 */
void BSP_BoardTest_StopTimer(void)
{
    if (BOARD_TEST_SELECT == BOARD_TEST_GYRO || BOARD_TEST_SELECT == BOARD_TEST_ACCEL
        || BOARD_TEST_SELECT == BOARD_TEST_ENCODER_LEFT
        || BOARD_TEST_SELECT == BOARD_TEST_ENCODER_RIGHT)
    {
        /* 测试结束不再重启；先屏蔽本IRQ，清除采集中可能积压的更新事件，避免停后补采。 */
        HAL_NVIC_DisableIRQ(TIM15_IRQn);
        BSP_Sampler_Stop();
        TIM15->DIER &= (uint16_t)~TIM_DIER_UIE;
        TIM15->SR = (uint16_t)~TIM_SR_UIF;
        HAL_NVIC_ClearPendingIRQ(TIM15_IRQn);
    }
    else if (BOARD_TEST_SELECT == BOARD_TEST_BATTERY)
    {
        (void)HAL_TIM_Base_Stop_IT(&htim7);
    }
    else if ((BOARD_TEST_SELECT >= BOARD_TEST_IND_HL && BOARD_TEST_SELECT <= BOARD_TEST_GRAY)
        || BOARD_TEST_SELECT == BOARD_TEST_DL1B)
    {
        (void)HAL_TIM_Base_Stop_IT(&htim6);
    }
}

/**
 * @brief 发送一帧测试JustFloat数据并返回可证实的完成状态。
 * @details 先检查指针和HAL发送状态，避免失败时读取旧计数；HAL_OK且TC才完整。
 *     部分提交不是完整线路发送，不重试，不改变公共UART接口。
 * @param[in] frame 非空16字节缓冲；函数返回前必须保持有效。
 * @return status为0完整、1部分、2失败；submitted仅为提交UART的字节数。
 * @pre USART1已配置460800/8N1，唯一前台发送者，未启用正常串口业务。
 * @note 前台HAL超时参数10ms；tick量化及高优先级采集阻塞会延长墙钟时间，不在ISR调用。
 */
bsp_board_test_tx_t BSP_BoardTest_SendFrame(const uint8_t *frame)
{
    bsp_board_test_tx_t result = {0, 2};
    HAL_StatusTypeDef status;
    if (frame == 0 || huart1.gState != HAL_UART_STATE_READY)
    {
        return result;
    }
    status = HAL_UART_Transmit(&huart1, (uint8_t *)frame, 16, 10);
    result.submitted = 16u - huart1.TxXferCount;
    if (status == HAL_OK && __HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC))
    {
        result.status = 0;
    }
    else if (result.submitted != 0)
    {
        result.status = 1;
    }
    return result;
}
