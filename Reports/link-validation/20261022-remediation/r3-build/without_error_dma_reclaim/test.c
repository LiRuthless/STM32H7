
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bsp_wifi.c"
static DMA_Stream_TypeDef tx_stream,rx_stream;
static DMA_HandleTypeDef tx_dma={&tx_stream,HAL_DMA_STATE_READY,0};
static DMA_HandleTypeDef rx_dma={&rx_stream,HAL_DMA_STATE_READY,0};
SPI_HandleTypeDef hspi2={.Instance=SPI2,.hdmatx=&tx_dma,.hdmarx=&rx_dma};
static uint32_t tick;
static unsigned cs=1,ready,aborts,starts,clears,disabled;
static HAL_StatusTypeDef start_result;
static unsigned primask,pending,init_injection,exti_clears;
static unsigned spi_leaves_dma,dma_fail,dma_stuck,tx_aborts,rx_aborts;
static HAL_StatusTypeDef spi_abort_result=HAL_OK;
uint32_t BSP_Time_EnterCritical(void){unsigned old=primask;primask=1;return old;}
void BSP_Time_ExitCritical(uint32_t m)
{primask=m;if(!primask&&pending){pending=0;HAL_GPIO_EXTI_Callback(15);}}
void Test_ClearExti(unsigned pin)
{
    assert(pin==15&&primask);pending=0;++exti_clears;
    if((init_injection==1&&exti_clears==1)||(init_injection==3&&exti_clears==2))
    {ready=0;pending=1;init_injection=0;}
}
void BSP_Time_MemoryBarrier(void){}
uint32_t HAL_GetTick(void){return tick;}
void HAL_GPIO_WritePin(unsigned p,unsigned n,unsigned v){assert(p==1&&n==12);cs=v;}
unsigned HAL_GPIO_ReadPin(unsigned p,unsigned n)
{
    assert(p==1&&n==15);unsigned snapshot=ready;
    if(init_injection==2){ready=0;pending=1;init_injection=0;}
    return snapshot;
}
void HAL_NVIC_DisableIRQ(unsigned n){disabled|=1u<<n;}
void HAL_NVIC_EnableIRQ(unsigned n){disabled&=~(1u<<n);}
void HAL_NVIC_ClearPendingIRQ(unsigned n){assert(disabled&(1u<<n));++clears;}
HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef*s){assert(s==&hspi2);return HAL_OK;}
static void dma_ready(DMA_HandleTypeDef*d)
{d->State=HAL_DMA_STATE_READY;((DMA_Stream_TypeDef*)d->Instance)->CR=0;}
HAL_DMA_StateTypeDef HAL_DMA_GetState(DMA_HandleTypeDef*d){return d->State;}
HAL_StatusTypeDef HAL_DMA_Abort(DMA_HandleTypeDef*d)
{
    unsigned bit=d==&tx_dma?1u:2u;
    assert(disabled==14u&&d->State==HAL_DMA_STATE_BUSY);
    if(bit==1u)++tx_aborts;else ++rx_aborts;
    if(dma_fail&bit)return HAL_ERROR;
    dma_ready(d);
    if(dma_stuck&bit)((DMA_Stream_TypeDef*)d->Instance)->CR=DMA_SxCR_EN;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_SPI_Abort(SPI_HandleTypeDef*s)
{
    assert(s==&hspi2&&disabled==14u);++aborts;
    HAL_SPI_TxRxCpltCallback(s); /* 迟到回调不能发布错误态RX。 */
    if(!spi_leaves_dma){dma_ready(&tx_dma);dma_ready(&rx_dma);}
    if(s->ErrorCode&HAL_SPI_ERROR_ABORT)return HAL_ERROR; /* 粘性ABORT位直接失败。 */
    if(spi_abort_result!=HAL_OK){s->ErrorCode|=HAL_SPI_ERROR_ABORT;return HAL_ERROR;}
    s->ErrorCode=HAL_SPI_ERROR_NONE; /* 成功路径才清错误码。 */
    return HAL_OK;
}
HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(SPI_HandleTypeDef*s,uint8_t*tx,uint8_t*rx,uint16_t n)
{
    assert(s==&hspi2&&tx==s_tx&&rx==s_rx&&n==1024&&!cs);++starts;
    if(start_result==HAL_OK)
    {
        if(tx_dma.State!=HAL_DMA_STATE_READY||rx_dma.State!=HAL_DMA_STATE_READY)return HAL_ERROR;
        tx_dma.State=rx_dma.State=HAL_DMA_STATE_BUSY;
        tx_stream.CR=rx_stream.CR=DMA_SxCR_EN;
    }
    return start_result;
}
static void Test_Complete(void)
{
    SPI_HandleTypeDef *spi=&hspi2;
    dma_ready(&tx_dma);dma_ready(&rx_dma);HAL_SPI_TxRxCpltCallback(spi);
}
static void startup_case(unsigned level,unsigned old_pending,unsigned injection,unsigned expected)
{
    ready=level;pending=old_pending;init_injection=injection;exti_clears=0;
    primask=0;start_result=HAL_OK;unsigned before=starts;BSP_WiFi_Init();
    assert(s_status.permits==expected&&!pending&&!primask);
    for(unsigned i=0;i<expected;++i)
    {assert(BSP_WiFi_TryExchange()==1);Test_Complete();BSP_WiFi_ReleaseRx();}
    assert(!BSP_WiFi_TryExchange()&&starts==before+expected);
}
static void recovery_setup(unsigned mask)
{
    ready=1;pending=0;init_injection=0;exti_clears=0;primask=0;
    start_result=spi_abort_result=HAL_OK;spi_leaves_dma=1;dma_fail=dma_stuck=0;
    hspi2.ErrorCode=HAL_SPI_ERROR_NONE;
    dma_ready(&tx_dma);dma_ready(&rx_dma);
    tx_dma.ErrorCode=rx_dma.ErrorCode=HAL_DMA_ERROR_NONE;
    BSP_WiFi_Init();
    s_status.consumed=s_status.permits;s_status.state=3;
    if(mask&1u){tx_dma.State=HAL_DMA_STATE_BUSY;tx_stream.CR=DMA_SxCR_EN;}
    if(mask&2u){rx_dma.State=HAL_DMA_STATE_BUSY;rx_stream.CR=DMA_SxCR_EN;}
}
static void recovery_cases(void)
{
    for(unsigned mask=1;mask<=3;++mask)
    {
        recovery_setup(mask);unsigned ta=tx_aborts,ra=rx_aborts;
        BSP_WiFi_Service();
        assert(tx_dma.State==HAL_DMA_STATE_READY&&rx_dma.State==HAL_DMA_STATE_READY);
        assert(!tx_stream.CR&&!rx_stream.CR&&!s_status.state&&!disabled);
        assert(tx_aborts-ta==!!(mask&1u)&&rx_aborts-ra==!!(mask&2u));
        assert(!BSP_WiFi_TryExchange());HAL_GPIO_EXTI_Callback(15);
        assert(BSP_WiFi_TryExchange()==1);Test_Complete();BSP_WiFi_ReleaseRx();
    }
    for(unsigned bad=1;bad<=2;++bad)
    {
        recovery_setup(3);dma_fail=bad;unsigned before=starts,ta=tx_aborts,ra=rx_aborts;
        BSP_WiFi_Service();assert(s_status.state==3&&!disabled&&cs);
        assert(tx_aborts==ta+1&&rx_aborts==ra+1); /* 不短路另一个方向。 */
        HAL_SPI_TxRxCpltCallback(&hspi2);assert(!BSP_WiFi_GetRx()&&!BSP_WiFi_GetTx());
        HAL_GPIO_EXTI_Callback(15);assert(!BSP_WiFi_TryExchange()&&starts==before);
        dma_fail=0;BSP_WiFi_Service();assert(!s_status.state);
        HAL_GPIO_EXTI_Callback(15); /* 重同步后由新沿发放许可。 */
        assert(BSP_WiFi_TryExchange()==1);Test_Complete();BSP_WiFi_ReleaseRx();
    }
    recovery_setup(1);dma_stuck=1;BSP_WiFi_Service();
    assert(tx_dma.State==HAL_DMA_STATE_READY&&tx_stream.CR&&s_status.state==3);
    BSP_WiFi_Service();assert(s_status.state==3&&!BSP_WiFi_GetTx()&&!disabled);
    tx_stream.CR=0;dma_stuck=0;BSP_WiFi_Service();assert(!s_status.state);
    recovery_setup(3);spi_abort_result=HAL_ERROR;unsigned ab=aborts;
    BSP_WiFi_Service();assert(s_status.state==3&&aborts-ab==2); /* 有界重试一次。 */
    assert(!tx_stream.CR&&!rx_stream.CR&&!disabled);
    spi_abort_result=HAL_OK;BSP_WiFi_Service();assert(!s_status.state);
    /* 通道A：ERROR态句柄EN已清，直接回收并清错误码。 */
    recovery_setup(0);tx_dma.State=HAL_DMA_STATE_ERROR;tx_dma.ErrorCode=0x80;
    BSP_WiFi_Service();
    assert(!s_status.state&&tx_dma.State==HAL_DMA_STATE_READY&&!tx_dma.ErrorCode);
    /* 通道A：ERROR态句柄EN残留，软件清使能后回收。 */
    recovery_setup(0);rx_dma.State=HAL_DMA_STATE_ERROR;rx_stream.CR=DMA_SxCR_EN;
    BSP_WiFi_Service();
    assert(!s_status.state&&!rx_stream.CR&&rx_dma.State==HAL_DMA_STATE_READY);
    /* 通道B：粘性ABORT位使首次中止失败，清错误码后重试成功。 */
    recovery_setup(3);hspi2.ErrorCode=HAL_SPI_ERROR_ABORT;ab=aborts;
    BSP_WiFi_Service();assert(!s_status.state&&aborts-ab==2&&!hspi2.ErrorCode);
    recovery_setup(0);s_status.state=0;HAL_GPIO_EXTI_Callback(15);
    assert(BSP_WiFi_TryExchange()==1);dma_fail=1;tick+=10;BSP_WiFi_Service();
    assert(s_status.timeouts==1&&s_status.state==3&&!disabled);
    HAL_SPI_TxRxCpltCallback(&hspi2);HAL_GPIO_EXTI_Callback(15);BSP_WiFi_Service();
    assert(s_status.timeouts==1&&s_status.state==3&&!BSP_WiFi_GetRx());
    dma_fail=0;BSP_WiFi_Service();assert(!s_status.state&&s_status.timeouts==1);
    HAL_GPIO_EXTI_Callback(15); /* 重同步后由新沿发放许可。 */
    assert(BSP_WiFi_TryExchange()==1);Test_Complete();BSP_WiFi_ReleaseRx();
    spi_leaves_dma=0;
    puts("PASS R2: orphan DMA, both directions, abort/EN/SPI failure, retry and late callback");
    puts("PASS R3: ERROR-state DMA reclaim, sticky-ABORT retry, bounded double abort");
}
static void resync_cases(void)
{
    /* 空闲态幻影积压且电平高：无已装载事务，积压归零。 */
    recovery_setup(0);s_status.state=0;ready=1;
    s_status.consumed=s_status.permits-5;
    BSP_WiFi_Service();
    assert(s_status.permits-s_status.consumed==0&&!s_status.state);
    /* 电平低保留一笔已装载许可，无需新边沿即可启动。 */
    ready=0;s_status.consumed=s_status.permits-5;
    BSP_WiFi_Service();
    assert(s_status.permits-s_status.consumed==1);
    assert(BSP_WiFi_TryExchange()==1);Test_Complete();BSP_WiFi_ReleaseRx();
    assert(s_status.permits==s_status.consumed);
    /* 采样窗口边沿合并：clear后read前下降不双计。 */
    recovery_setup(0);s_status.state=0;ready=1;
    s_status.consumed=s_status.permits-5;init_injection=1;exti_clears=0;
    {unsigned before=s_status.permits;BSP_WiFi_Service();
     assert(s_status.permits==before&&s_status.permits-s_status.consumed==1);}
    /* read高之后下降：边沿由pending保留，不丢失。 */
    recovery_setup(0);s_status.state=0;ready=1;
    s_status.consumed=s_status.permits-5;init_injection=2;
    BSP_WiFi_Service();
    assert(s_status.permits-s_status.consumed==1);
    /* 恢复成功点同样重基线：电平低保留已装载许可。 */
    recovery_setup(1);ready=0;BSP_WiFi_Service();
    assert(!s_status.state&&s_status.permits-s_status.consumed==1);
    assert(BSP_WiFi_TryExchange()==1);Test_Complete();BSP_WiFi_ReleaseRx();
    /* 待读取入口幻影积压同样修剪：流式工况下同趟启动先于下一趟Service到达空闲。 */
    recovery_setup(0);s_status.state=0;ready=1;HAL_GPIO_EXTI_Callback(15);
    assert(BSP_WiFi_TryExchange()==1);Test_Complete();assert(s_status.state==2);
    s_status.consumed=s_status.permits-5;
    BSP_WiFi_Service();
    assert(s_status.state==2&&s_status.permits-s_status.consumed==0);
    BSP_WiFi_ReleaseRx();
    assert(!BSP_WiFi_TryExchange()); /* 无真实许可不启动。 */
    /* 积压不超过1时重基线不动作，许可计数不变。 */
    recovery_setup(0);s_status.state=0;ready=0;HAL_GPIO_EXTI_Callback(15);
    {unsigned before=s_status.permits;BSP_WiFi_Service();
     assert(s_status.permits==before&&s_status.permits-s_status.consumed==1);}
    assert(BSP_WiFi_TryExchange()==1);Test_Complete();BSP_WiFi_ReleaseRx();
    puts("PASS R3: permit resync by READY level, window merge, recovery resync");
}
int main(void)
{
    BSP_WiFi_Init();assert(cs&&s_status.permits==1);
    assert(BSP_WiFi_TryExchange()==1&&!cs&&!BSP_WiFi_GetTx());
    ready=1; /* 首笔已消费后READY回到高电平，后续许可由显式边沿注入。 */
    assert(!BSP_WiFi_TryExchange()&&starts==1);
    HAL_GPIO_EXTI_Callback(15);HAL_GPIO_EXTI_Callback(15);
    tick=9;BSP_WiFi_Service();assert(!aborts&&!cs);
    Test_Complete();assert(cs&&BSP_WiFi_GetRx());
    assert(!BSP_WiFi_TryExchange());BSP_WiFi_ReleaseRx();
    assert(BSP_WiFi_TryExchange()==1);Test_Complete();BSP_WiFi_ReleaseRx();
    assert(BSP_WiFi_TryExchange()==1);Test_Complete();BSP_WiFi_ReleaseRx();
    assert(!BSP_WiFi_TryExchange()); /* 持续低电平不能新增许可。 */
    HAL_GPIO_EXTI_Callback(15);start_result=HAL_BUSY;
    unsigned consumed=s_status.consumed;assert(!BSP_WiFi_TryExchange()&&cs);
    assert(s_status.consumed==consumed);start_result=HAL_OK;
    assert(BSP_WiFi_TryExchange());tick=19;BSP_WiFi_Service();
    assert(aborts==1&&cs&&s_status.timeouts==1&&clears==3&&!disabled);
    Test_Complete();assert(!BSP_WiFi_GetRx());
    assert(!BSP_WiFi_TryExchange());HAL_GPIO_EXTI_Callback(15);
    assert(BSP_WiFi_TryExchange());tick=30;BSP_WiFi_Service();assert(aborts==2);
    HAL_GPIO_EXTI_Callback(15);start_result=HAL_ERROR;
    assert(BSP_WiFi_TryExchange()==2&&cs);BSP_WiFi_Service();assert(aborts==3);
    start_result=HAL_OK;HAL_GPIO_EXTI_Callback(15);assert(BSP_WiFi_TryExchange());
    HAL_SPI_ErrorCallback(&hspi2);assert(cs);BSP_WiFi_Service();assert(aborts==4);
    assert(BSP_WiFi_SetFastClock(1)&&hspi2.Init.BaudRatePrescaler==64);
    ready=1;BSP_WiFi_Init();assert(!BSP_WiFi_TryExchange());
    startup_case(0,1,0,1); /* 既有pending与低电平合并。 */
    startup_case(1,1,0,0); /* 启动前旧pending不构成新高电平许可。 */
    startup_case(1,0,1,1); /* clear后、read前下降，不能双计。 */
    startup_case(1,0,2,1); /* read高之后下降，不能二次clear丢失。 */
    startup_case(0,0,3,2); /* 二次clear后的新边沿必须另外保留。 */
    recovery_cases();
    resync_cases();
    puts("PASS V-03 real BSP: READY permits, 9/10/11ms, busy/error, late callback, CS, recovery");
    return 0;
}
