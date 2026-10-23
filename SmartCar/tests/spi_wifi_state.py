"""Run the real Wi-Fi BSP against a fault-injectable native HAL replacement."""
from pathlib import Path
import shutil
import subprocess
import tempfile
from host_support import ROOT, build

BSP = r'''
#include <stdint.h>
#include <stddef.h>
#include "bsp_wifi.h"
typedef enum {HAL_OK, HAL_ERROR, HAL_BUSY} HAL_StatusTypeDef;
typedef struct {unsigned CR;} DMA_Stream_TypeDef;
typedef enum {HAL_DMA_STATE_RESET, HAL_DMA_STATE_READY, HAL_DMA_STATE_BUSY,
              HAL_DMA_STATE_ERROR} HAL_DMA_StateTypeDef;
typedef struct {void *Instance; HAL_DMA_StateTypeDef State; unsigned ErrorCode;} DMA_HandleTypeDef;
#define DMA_SxCR_EN 1u
#define HAL_DMA_ERROR_NONE 0u
typedef struct {unsigned BaudRatePrescaler;} SPI_InitTypeDef;
typedef struct {unsigned Instance; SPI_InitTypeDef Init; unsigned ErrorCode;
                DMA_HandleTypeDef *hdmatx,*hdmarx;} SPI_HandleTypeDef;
extern SPI_HandleTypeDef hspi2;
#define SPI2 2
#define HAL_SPI_ERROR_NONE 0u
#define HAL_SPI_ERROR_ABORT 0x40u
#define WIFI_CS_GPIO_Port 1
#define WIFI_CS_Pin 12
#define WIFI_INT_GPIO_Port 1
#define WIFI_INT_Pin 15
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1
#define DMA1_Stream1_IRQn 1
#define DMA1_Stream2_IRQn 2
#define SPI2_IRQn 3
#define SPI_BAUDRATEPRESCALER_64 64
#define SPI_BAUDRATEPRESCALER_256 256
void Test_ClearExti(unsigned);
#define __HAL_GPIO_EXTI_CLEAR_IT(x) Test_ClearExti(x)
uint32_t BSP_Time_EnterCritical(void);
void BSP_Time_ExitCritical(uint32_t);
void BSP_Time_MemoryBarrier(void);
void HAL_GPIO_WritePin(unsigned,unsigned,unsigned);
unsigned HAL_GPIO_ReadPin(unsigned,unsigned);
uint32_t HAL_GetTick(void);
HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(SPI_HandleTypeDef*,uint8_t*,uint8_t*,uint16_t);
HAL_StatusTypeDef HAL_SPI_Abort(SPI_HandleTypeDef*);
HAL_StatusTypeDef HAL_DMA_Abort(DMA_HandleTypeDef*);
HAL_DMA_StateTypeDef HAL_DMA_GetState(DMA_HandleTypeDef*);
HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef*);
void HAL_NVIC_DisableIRQ(unsigned);
void HAL_NVIC_EnableIRQ(unsigned);
void HAL_NVIC_ClearPendingIRQ(unsigned);
void Test_ForceReset(void);
void Test_ReleaseReset(void);
#define __HAL_RCC_SPI2_FORCE_RESET() Test_ForceReset()
#define __HAL_RCC_SPI2_RELEASE_RESET() Test_ReleaseReset()
'''
HARNESS = r'''
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
static unsigned rcc_forced,spi_inits,spi_ovr_latch;
static HAL_StatusTypeDef spi_init_result=HAL_OK;
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
void Test_ForceReset(void){++rcc_forced;spi_ovr_latch=0;} /* RCC脉冲清除外设锁存病灶。 */
void Test_ReleaseReset(void){}
HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef*s)
{assert(s==&hspi2);++spi_inits;return spi_init_result;}
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
    s_consecutive_errors=s_reinit_count=0;
    rcc_forced=spi_inits=spi_ovr_latch=0;spi_init_result=HAL_OK;
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
static void r4_cases(void)
{
    /* OVR锁存：定向修补机械成功但事务持续失败，第3次恢复升级外设重初始化。 */
    recovery_setup(0);s_status.state=0;ready=1;spi_ovr_latch=1;
    for(unsigned cycle=1;cycle<=2;++cycle)
    {
        HAL_GPIO_EXTI_Callback(15);assert(BSP_WiFi_TryExchange()==1);
        HAL_SPI_ErrorCallback(&hspi2);
        BSP_WiFi_Service();
        assert(!s_status.state&&s_consecutive_errors==cycle);
        assert(!s_reinit_count&&!spi_inits&&spi_ovr_latch);
    }
    HAL_GPIO_EXTI_Callback(15);assert(BSP_WiFi_TryExchange()==1);
    HAL_SPI_ErrorCallback(&hspi2);assert(s_consecutive_errors==3);
    BSP_WiFi_Service();
    assert(!s_status.state&&!s_consecutive_errors&&s_reinit_count==1);
    assert(spi_inits==1&&rcc_forced==1&&!spi_ovr_latch);
    HAL_GPIO_EXTI_Callback(15);assert(BSP_WiFi_TryExchange()==1);
    Test_Complete();BSP_WiFi_ReleaseRx();assert(!s_consecutive_errors);
    /* 成功完成后需重新累计3次连续失败才第2次升级。 */
    spi_ovr_latch=1;
    for(unsigned cycle=1;cycle<=3;++cycle)
    {
        HAL_GPIO_EXTI_Callback(15);assert(BSP_WiFi_TryExchange()==1);
        HAL_SPI_ErrorCallback(&hspi2);
        BSP_WiFi_Service();
        assert(s_reinit_count==(cycle==3?2u:1u)&&!s_status.state);
    }
    HAL_GPIO_EXTI_Callback(15);assert(BSP_WiFi_TryExchange()==1);
    Test_Complete();BSP_WiFi_ReleaseRx();
    /* 重初始化失败：保持错误态不发布空闲，下一前台周期有界重试。 */
    recovery_setup(0);s_consecutive_errors=3;spi_init_result=HAL_ERROR;
    BSP_WiFi_Service();
    assert(s_status.state==3&&spi_inits==1&&rcc_forced==1&&!s_reinit_count);
    assert(s_consecutive_errors==3&&!disabled);
    spi_init_result=HAL_OK;BSP_WiFi_Service();
    assert(!s_status.state&&spi_inits==2&&rcc_forced==2&&s_reinit_count==1);
    HAL_GPIO_EXTI_Callback(15);
    assert(BSP_WiFi_TryExchange()==1);Test_Complete();BSP_WiFi_ReleaseRx();
    puts("PASS R4: escalation after 3 consecutive failures, RCC pulse heals latch, bounded retry");
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
    r4_cases();
    puts("PASS V-03 real BSP: READY permits, 9/10/11ms, busy/error, late callback, CS, recovery");
    return 0;
}
'''


def cut_function(source, anchor):
    """Remove one function body (doc comment left in place) for negative variants."""
    start = source.index(anchor)
    end = source.index("/**", start)
    return source[:start] + source[end:]


def variants(source):
    """Negative-control variants: (name, patched source, expected failure fragment)."""
    out = []
    startup = "    if (s_status.permits)\n    {\n        __HAL_GPIO_EXTI_CLEAR_IT(WIFI_INT_Pin);\n    }\n"
    assert source.count(startup) == 1
    out.append(("without_startup_dedup", source.replace(startup, "", 1),
                "s_status.permits==expected"))
    dma = ("    tx_stopped = BSP_WiFi_AbortDma(hspi2.hdmatx);\n"
           "    rx_stopped = BSP_WiFi_AbortDma(hspi2.hdmarx);\n")
    assert source.count(dma) == 1
    old = source.replace(dma, "    tx_stopped = 1;\n    rx_stopped = 1;\n", 1)
    out.append(("without_dma_reclaim",
                cut_function(old, "static uint8_t BSP_WiFi_AbortDma("), "tx_dma.State"))
    error_reclaim = '''    else if (HAL_DMA_GetState(dma) != HAL_DMA_STATE_READY)
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
'''
    assert source.count(error_reclaim) == 1
    old = source.replace(error_reclaim, "", 1).replace("    uint32_t started;\n", "", 1)
    out.append(("without_error_dma_reclaim", old, "tx_dma.ErrorCode"))
    retry = '''    if (spi_result != HAL_OK)
    {
        hspi2.ErrorCode = HAL_SPI_ERROR_NONE;
        spi_result = HAL_SPI_Abort(&hspi2);
    }
'''
    assert source.count(retry) == 1
    out.append(("without_abort_retry", source.replace(retry, "", 1), "aborts-ab==2"))
    resync_idle = '''        if ((s_status.state == 0 || s_status.state == 2)
            && s_status.permits - s_status.consumed > 1u)
        {
            BSP_WiFi_ResyncPermits();
        }
'''
    resync_recovery = "        BSP_WiFi_ResyncPermits();\n        s_status.state = 0;"
    assert source.count(resync_idle) == 1 and source.count(resync_recovery) == 1
    old = source.replace(resync_idle, "", 1)
    old = old.replace(resync_recovery, "        s_status.state = 0;", 1)
    out.append(("without_resync",
                cut_function(old, "static void BSP_WiFi_ResyncPermits("),
                "s_status.permits-s_status.consumed==0"))
    state2 = ("(s_status.state == 0 || s_status.state == 2)\n"
              "            && s_status.permits - s_status.consumed > 1u")
    assert source.count(state2) == 1
    out.append(("without_state2_resync",
                source.replace(state2,
                               "s_status.state == 0 && s_status.permits - s_status.consumed > 1u", 1),
                "s_status.state==2&&s_status.permits-s_status.consumed==0"))
    escalation = '''    if (spi_result == HAL_OK && tx_stopped && rx_stopped
        && s_consecutive_errors >= BSP_WIFI_REINIT_THRESHOLD)
    {
        __HAL_RCC_SPI2_FORCE_RESET();
        __HAL_RCC_SPI2_RELEASE_RESET();
        if (HAL_SPI_Init(&hspi2) == HAL_OK)
        {
            s_consecutive_errors = 0;
            ++s_reinit_count;
        }
        else
        {
            reinit_done = 0;
        }
    }
'''
    assert source.count(escalation) == 1
    out.append(("without_escalation", source.replace(escalation, "", 1),
                "s_reinit_count==1"))
    return out


def main():
    with tempfile.TemporaryDirectory(prefix="spi_wifi_state_") as temp:
        work = Path(temp)
        shutil.copyfile(ROOT / "BSP/bsp_wifi.c", work / "bsp_wifi.c")
        (work / "bsp.h").write_text(BSP)
        (work / "test.c").write_text(HARNESS)
        binary = build(work, [work / "test.c"])
        subprocess.run([str(binary)], check=True)
        # Negative controls prove each new path is what makes the assertions pass.
        current = (work / "bsp_wifi.c").read_text(encoding="utf-8")
        messages = {
            "without_startup_dedup":
                "PASS V-03 startup edge injection: old code fails, current grants exactly once",
            "without_dma_reclaim":
                "PASS R2 negative control: skipped DMA reclaim fails at the orphan-TX assertion",
            "without_error_dma_reclaim":
                "PASS R3 negative control: ERROR-state handle stays unreclaimed on the old path",
            "without_abort_retry":
                "PASS R3 negative control: sticky ABORT bit blocks recovery without the retry",
            "without_resync":
                "PASS R3 negative control: phantom backlog persists without the level resync",
            "without_state2_resync":
                "PASS R3 negative control: streaming-state phantom backlog persists without the state==2 trigger",
            "without_escalation":
                "PASS R4 negative control: OVR latch never heals without the peripheral reinit escalation",
        }
        for name, candidate, expected in variants(current):
            (work / "bsp_wifi.c").write_text(candidate, encoding="utf-8")
            before = build(work, [work / "test.c"], name=name)
            result = subprocess.run([str(before)], capture_output=True)
            assert result.returncode and expected.encode() in result.stderr, name
            print(messages[name])


if __name__ == "__main__":
    main()
