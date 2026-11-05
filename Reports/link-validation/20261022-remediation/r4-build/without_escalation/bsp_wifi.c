/**
 * @file bsp_wifi.c
 * @brief SPI2 DMA、READY许可及10ms前台恢复。
 * @details READY IRQ只发布许可；前台独占启动、RX消费、中止与积压重基线。完成回调仅释放片选及置位，
 *     不做解析、CRC或等待。连续失败达到阈值时升级为SPI2外设级复位重初始化。两个绝对地址缓冲
 *     位于AXI，D-Cache保持关闭，不操作ESP EN。
 */
#include "bsp.h"
#include <string.h>

#if defined(__CC_ARM)
/* AC5由绝对定位段负责分配，必须另查AC5 map，不能用GCC结果替代。 */
static uint8_t s_tx[BSP_WIFI_TRANSACTION_SIZE] __attribute__((at(0x2407f000), aligned(32)));
static uint8_t s_rx[BSP_WIFI_TRANSACTION_SIZE] __attribute__((at(0x2407f400), aligned(32)));
#else
/* GCC专段由链接脚本固定；不与普通RAM、栈或ADC D2缓冲重叠。 */
static uint8_t s_tx[BSP_WIFI_TRANSACTION_SIZE]
    __attribute__((section(".ARM.__at_0x2407F000"), aligned(32)));
static uint8_t s_rx[BSP_WIFI_TRANSACTION_SIZE]
    __attribute__((section(".ARM.__at_0x2407F400"), aligned(32)));
#endif

/* READY IRQ单写累计值，前台只写consumed；state由前台和完成IRQ有界交接。 */
static volatile bsp_wifi_status_t s_status;
static uint32_t s_started;

/* 定向修补连续失败达到该代数时升级为SPI2外设重初始化。 */
#define BSP_WIFI_REINIT_THRESHOLD 3u
/* 自上次成功完成以来连续失败的事务代数；仅真实成功完成清零。 */
static uint32_t s_consecutive_errors;
/* SPI2外设重初始化累计次数；仅供调试器经符号读取，不进遥测与协议。 */
static volatile uint32_t s_reinit_count;

/**
 * @brief 按逻辑有效状态驱动CS。
 * @param[in] active 1拉低，0拉高。
 * @note 前台或SPI回调调用；不操作EN。
 */
static void BSP_WiFi_SetCs(uint8_t active)
{
    HAL_GPIO_WritePin(WIFI_CS_GPIO_Port, WIFI_CS_Pin, active ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

/**
 * @brief 初始化事务并补获上电低电平。
 * @details 先清旧pending再采样；补获低电平时二次清pending，将采样窗口的同一边沿合并。
 *     高电平采样后不再清除，新下降沿留给IRQ；二次清除后的新边沿也由IRQ独立计数。
 * @note 仅前台上电一次调用；不在在途重初始化，不更改EN。
 */
void BSP_WiFi_Init(void)
{
    uint32_t mask = BSP_Time_EnterCritical();
    memset((void *)&s_status, 0, sizeof(s_status));
    BSP_WiFi_SetCs(0);
    __HAL_GPIO_EXTI_CLEAR_IT(WIFI_INT_Pin);
    s_status.permits = HAL_GPIO_ReadPin(WIFI_INT_GPIO_Port, WIFI_INT_Pin) == GPIO_PIN_RESET;
    if (s_status.permits)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(WIFI_INT_Pin);
    }
    BSP_Time_ExitCritical(mask);
    memset(s_tx, 0, sizeof(s_tx));
    memset(s_rx, 0, sizeof(s_rx));
}

/**
 * @brief 取得静态发送缓冲。
 * @return 仅空闲时返回1024B可写区域，其余为空。
 * @note 仅前台写；启动后直到收取完成不能改写。
 */
uint8_t *BSP_WiFi_GetTx(void)
{
    return s_status.state == 0 ? s_tx : NULL;
}

/**
 * @brief 取得成功完成事务。
 * @return state为2时借用1024B只读区域，否则为空。
 * @note 仅前台读取，ReleaseRx后指针内容可被下一DMA覆写。
 */
const uint8_t *BSP_WiFi_GetRx(void)
{
    BSP_Time_MemoryBarrier();
    return s_status.state == 2 ? s_rx : NULL;
}

/**
 * @brief 消费一次已完成RX。
 * @details 只有成功完成态返回空闲，错误和在途不被调用方清除。
 * @note 前台解析结束后调用；保留在途期间新到达的READY许可。
 */
void BSP_WiFi_ReleaseRx(void)
{
    if (s_status.state == 2)
    {
        s_status.state = 0;
    }
}

/**
 * @brief 消费许可并启动单笔DMA。
 * @details 空闲和有新许可才拉低CS；HAL_BUSY保留许可，启动失败消费本次许可并等待新沿。
 *     启动失败计入连续失败代数，供恢复路径判断是否升级外设重初始化。
 * @return 1已启动，0空闲条件不满足或HAL_BUSY，2启动失败。
 * @note 前台调用，缓冲已填好；HAL启动不等待传输完成，控制IRQ保持开放。
 */
uint8_t BSP_WiFi_TryExchange(void)
{
    HAL_StatusTypeDef result;
    if (s_status.state || s_status.consumed == s_status.permits)
    {
        return 0;
    }
    s_started = HAL_GetTick();
    s_status.state = 1;
    BSP_WiFi_SetCs(1);
    result = HAL_SPI_TransmitReceive_DMA(&hspi2, s_tx, s_rx, BSP_WIFI_TRANSACTION_SIZE);
    if (result == HAL_BUSY)
    {
        BSP_WiFi_SetCs(0);
        s_status.state = 0;
        return 0;
    }
    ++s_status.consumed;
    if (result != HAL_OK)
    {
        BSP_WiFi_SetCs(0);
        ++s_status.errors;
        ++s_consecutive_errors;
        s_status.state = 3;
        return 2;
    }
    return 1;
}

/**
 * @brief 中止残留DMA并确认句柄与硬件均已停止。
 * @details BUSY句柄先请求HAL中止；失败立即保留不可复用结果。ERROR/TIMEOUT/ABORT态
 *     句柄不调用HAL_DMA_Abort（其对非BUSY只记录NO_XFER而不清理），改为软件清Stream
 *     使能位并有界等待归零，随后清句柄错误码并复位为READY；流错误标志由下一次
 *     HAL启动序列清理。READY句柄不动作。最后同时检查READY状态和Stream使能位，
 *     避免只恢复句柄而硬件仍占用缓冲时发布成功。
 * @param[in,out] dma 非空SPI2 TX或RX句柄，固定关联DMA1 Stream1或Stream2；中止时更新状态。
 * @return 1表示句柄已就绪且流使能位清零，0表示中止失败或仍不可复用。
 * @pre 调用方已置错误态并屏蔽本链DMA/SPI IRQ；句柄及其Instance非空。
 * @note 仅前台调用，等待界限沿用HAL或2ms有界轮询；不按SPI请求位推断DMA所有权已归还。
 */
static uint8_t BSP_WiFi_AbortDma(DMA_HandleTypeDef *dma)
{
    DMA_Stream_TypeDef *stream = (DMA_Stream_TypeDef *)dma->Instance;
    uint32_t started;

    if (HAL_DMA_GetState(dma) == HAL_DMA_STATE_BUSY)
    {
        if (HAL_DMA_Abort(dma) != HAL_OK)
        {
            return 0;
        }
    }
    else if (HAL_DMA_GetState(dma) != HAL_DMA_STATE_READY)
    {
        if (stream->CR & DMA_SxCR_EN)
        {
            started = HAL_GetTick();
            stream->CR &= ~DMA_SxCR_EN;
            while (stream->CR & DMA_SxCR_EN)
            {
                if ((uint32_t)(HAL_GetTick() - started) >= 2u)
                {
                    return 0;
                }
            }
        }
        dma->ErrorCode = HAL_DMA_ERROR_NONE;
        dma->State = HAL_DMA_STATE_READY;
    }
    return HAL_DMA_GetState(dma) == HAL_DMA_STATE_READY
        && (stream->CR & DMA_SxCR_EN) == 0;
}

/**
 * @brief 按READY电平将许可积压重新基线。
 * @details 复用启动补获去重顺序：短临界区先清pending再采样电平；低电平保留一笔
 *     已装载许可并二次清pending，将采样窗口的同一边沿合并；高电平采样后到达的
 *     新边沿由IRQ独立计数。ESP32从机队列深度为1，合法积压不超过1；失步产生的
 *     幻影积压按电平恢复真实值。采样窗口内刚装载而尚未计数的边沿允许consumed
 *     回退1，使积压如实反映当前装载状态。
 * @note 仅前台在无在途事务的安全点调用；不更改IRQ沿计数路径。
 */
static void BSP_WiFi_ResyncPermits(void)
{
    uint32_t mask = BSP_Time_EnterCritical();
    uint32_t backlog;
    __HAL_GPIO_EXTI_CLEAR_IT(WIFI_INT_Pin);
    backlog = HAL_GPIO_ReadPin(WIFI_INT_GPIO_Port, WIFI_INT_Pin) == GPIO_PIN_RESET ? 1u : 0u;
    s_status.consumed = s_status.permits - backlog;
    if (backlog)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(WIFI_INT_Pin);
    }
    BSP_Time_ExitCritical(mask);
}

/**
 * @brief 在前台处理错误或达到10ms的事务。
 * @details 屏蔽本链DMA/SPI IRQ后中止HAL，清除DMA/SPI残留pending再开放；READY IRQ不屏蔽，
 *     新许可仍可累计。显式检查双向DMA，全部回收后才发布空闲；失败保持错误态再重试。
 *     旧回调因状态检查和硬件中止清理不能完成新一代事务。HAL_SPI_Abort对粘性ABORT位
 *     直接失败且只有成功路径清ErrorCode，失败后清错误码并有界重试一次，单次调用
 *     最多两次中止尝试。恢复成功发布空闲前按READY电平重基线许可积压；无在途事务的
 *     安全点（空闲或待读取）发现积压超过队列深度1的合法上界时同样重基线，失步幻影
 *     不长期残留；待读取点覆盖持续流式工况，同趟启动先于下一趟本检查到达空闲。
 *     定向修补只能恢复HAL句柄与DMA，对驻留在SPI2外设/请求逻辑中的病灶无效
 *     （实机证据：OVR死循环定向修补机械成功但每笔事务仍失败）。连续失败事务代数
 *     达到BSP_WIFI_REINIT_THRESHOLD时，在中止完成、CS拉高、DMA流停止后升级为SPI2
 *     外设重初始化：RCC APB1L复位脉冲清空外设寄存器域，再HAL_SPI_Init按当前Init
 *     重跑MSP（时钟选择、GPIO、DMA重新链接、IRQ使能均重写为相同配置）。重初始化
 *     成功才清连续失败代数并累计s_reinit_count；失败保持错误态，下一前台周期按同
 *     规则重试。RCC脉冲不影响RCC时钟配置、DMAMUX、DMA与其他外设。
 * @note 前台调用，可能执行HAL有界中止与外设重初始化；全程不屏蔽TIM15/6/7，不改变
 *     车辆运行状态；外设重初始化无无界等待，不调用MspDeInit以避免GPIO毛刺。
 */
void BSP_WiFi_Service(void)
{
    uint8_t timeout = s_status.state == 1 && (uint32_t)(HAL_GetTick() - s_started) >= 10u;
    HAL_StatusTypeDef spi_result;
    uint8_t tx_stopped;
    uint8_t rx_stopped;
    uint8_t reinit_done;
    if (!timeout && s_status.state != 3)
    {
        if ((s_status.state == 0 || s_status.state == 2)
            && s_status.permits - s_status.consumed > 1u)
        {
            BSP_WiFi_ResyncPermits();
        }
        return;
    }
    HAL_NVIC_DisableIRQ(DMA1_Stream1_IRQn);
    HAL_NVIC_DisableIRQ(DMA1_Stream2_IRQn);
    HAL_NVIC_DisableIRQ(SPI2_IRQn);
    s_status.state = 3;
    /* SPI错误处理可能已关闭请求位，HAL_SPI_Abort此时会跳过仍在忙的DMA流。 */
    spi_result = HAL_SPI_Abort(&hspi2);
    if (spi_result != HAL_OK)
    {
        hspi2.ErrorCode = HAL_SPI_ERROR_NONE;
        spi_result = HAL_SPI_Abort(&hspi2);
    }
    tx_stopped = BSP_WiFi_AbortDma(hspi2.hdmatx);
    rx_stopped = BSP_WiFi_AbortDma(hspi2.hdmarx);
    BSP_WiFi_SetCs(0);
    HAL_NVIC_ClearPendingIRQ(DMA1_Stream1_IRQn);
    HAL_NVIC_ClearPendingIRQ(DMA1_Stream2_IRQn);
    HAL_NVIC_ClearPendingIRQ(SPI2_IRQn);
    if (timeout)
    {
        ++s_status.timeouts;
        ++s_consecutive_errors;
    }
    /* 失败时保持缓冲不可借用；共同收尾仍恢复IRQ，新READY许可不被丢弃。 */
    reinit_done = 1;
    if (spi_result == HAL_OK && tx_stopped && rx_stopped && reinit_done)
    {
        BSP_WiFi_ResyncPermits();
        s_status.state = 0;
    }
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
    HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
    HAL_NVIC_EnableIRQ(SPI2_IRQn);
}

/**
 * @brief 复制事务计数与状态。
 * @details 短临界区中复制，避免permit与完成计数来自不同更新时刻。
 * @param[out] status 非空调用方对象。
 * @note 前台使用，不暴露内部可写状态。
 */
void BSP_WiFi_GetStatus(bsp_wifi_status_t *status)
{
    uint32_t mask = BSP_Time_EnterCritical();
    *status = s_status;
    BSP_Time_ExitCritical(mask);
}

/**
 * @brief 在联调前台切换SPI2分频。
 * @details 只接受空闲态，PLL1Q保持480MHz，对应256/64分频为1.875/7.5MHz。
 * @param[in] fast 非零选择64，否则256。
 * @return HAL初始化成功1，忙或初始化失败0。
 * @pre 调用方确认车辆停车且低速波形已通过；默认初始化始终低速。
 * @note 不改共享PLL和SPI4/5时钟源。
 */
uint8_t BSP_WiFi_SetFastClock(uint8_t fast)
{
    if (s_status.state)
    {
        return 0;
    }
    hspi2.Init.BaudRatePrescaler = fast ? SPI_BAUDRATEPRESCALER_64 : SPI_BAUDRATEPRESCALER_256;
    return HAL_SPI_Init(&hspi2) == HAL_OK;
}

/**
 * @brief 发布一次READY下降沿许可。
 * @details 不根据持续低电平重复加许可，快速高低沿在在途期间仍累计。
 * @param[in] pin HAL提供的EXTI引脚位图，仅处理PA15。
 * @note EXTI优先级4，无等待或解析。
 */
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (pin == WIFI_INT_Pin)
    {
        ++s_status.permits;
    }
}

/**
 * @brief 结束成功DMA的片选并发布RX可读。
 * @details HAL的SPI EOT完成路径调用；只有在途状态接管，迟到回调不改变空闲或错误态。
 *     真实成功完成是连续失败代数的唯一清零点，定向修补的机械成功不清零。
 * @param[in] spi HAL非空SPI句柄，仅SPI2有效。
 * @note IRQ仅片选和状态；CRC及分发全部留在前台。
 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *spi)
{
    if (spi->Instance == SPI2 && s_status.state == 1)
    {
        BSP_WiFi_SetCs(0);
        ++s_status.completed;
        s_consecutive_errors = 0;
        BSP_Time_MemoryBarrier();
        s_status.state = 2;
    }
}

/**
 * @brief 结束出错事务的片选并请求前台中止。
 * @details 仅接管SPI2在途错误，不在IRQ调用HAL_Abort或阻塞恢复。错误计入连续失败
 *     代数，供前台恢复路径判断是否升级SPI2外设重初始化。
 * @param[in] spi HAL非空SPI句柄。
 * @note IRQ仅片选和标志，车辆运行状态不受通信故障影响。
 */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *spi)
{
    if (spi->Instance == SPI2 && s_status.state == 1)
    {
        BSP_WiFi_SetCs(0);
        ++s_status.errors;
        ++s_consecutive_errors;
        s_status.state = 3;
    }
}
