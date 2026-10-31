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
typedef struct {unsigned BaudRatePrescaler;} SPI_InitTypeDef;
typedef struct {unsigned Instance; SPI_InitTypeDef Init;} SPI_HandleTypeDef;
extern SPI_HandleTypeDef hspi2;
#define SPI2 2
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
HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef*);
void HAL_NVIC_DisableIRQ(unsigned);
void HAL_NVIC_EnableIRQ(unsigned);
void HAL_NVIC_ClearPendingIRQ(unsigned);
'''
HARNESS = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bsp_wifi.c"
SPI_HandleTypeDef hspi2={.Instance=SPI2};
static uint32_t tick;
static unsigned cs=1,ready,aborts,starts,clears,disabled;
static HAL_StatusTypeDef start_result;
static unsigned primask,pending,init_injection,exti_clears;
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
HAL_StatusTypeDef HAL_SPI_Abort(SPI_HandleTypeDef*s){assert(s==&hspi2);++aborts;HAL_SPI_TxRxCpltCallback(s);return HAL_OK;}
HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(SPI_HandleTypeDef*s,uint8_t*tx,uint8_t*rx,uint16_t n)
{assert(s==&hspi2&&tx==s_tx&&rx==s_rx&&n==1024&&!cs);++starts;return start_result;}
static void startup_case(unsigned level,unsigned old_pending,unsigned injection,unsigned expected)
{
    ready=level;pending=old_pending;init_injection=injection;exti_clears=0;
    primask=0;start_result=HAL_OK;unsigned before=starts;BSP_WiFi_Init();
    assert(s_status.permits==expected&&!pending&&!primask);
    for(unsigned i=0;i<expected;++i)
    {assert(BSP_WiFi_TryExchange()==1);HAL_SPI_TxRxCpltCallback(&hspi2);BSP_WiFi_ReleaseRx();}
    assert(!BSP_WiFi_TryExchange()&&starts==before+expected);
}
int main(void)
{
    BSP_WiFi_Init();assert(cs&&s_status.permits==1);
    assert(BSP_WiFi_TryExchange()==1&&!cs&&!BSP_WiFi_GetTx());
    assert(!BSP_WiFi_TryExchange()&&starts==1);
    HAL_GPIO_EXTI_Callback(15);HAL_GPIO_EXTI_Callback(15);
    tick=9;BSP_WiFi_Service();assert(!aborts&&!cs);
    HAL_SPI_TxRxCpltCallback(&hspi2);assert(cs&&BSP_WiFi_GetRx());
    assert(!BSP_WiFi_TryExchange());BSP_WiFi_ReleaseRx();
    assert(BSP_WiFi_TryExchange()==1);HAL_SPI_TxRxCpltCallback(&hspi2);BSP_WiFi_ReleaseRx();
    assert(BSP_WiFi_TryExchange()==1);HAL_SPI_TxRxCpltCallback(&hspi2);BSP_WiFi_ReleaseRx();
    assert(!BSP_WiFi_TryExchange()); /* 持续低电平不能新增许可。 */
    HAL_GPIO_EXTI_Callback(15);start_result=HAL_BUSY;
    unsigned consumed=s_status.consumed;assert(!BSP_WiFi_TryExchange()&&cs);
    assert(s_status.consumed==consumed);start_result=HAL_OK;
    assert(BSP_WiFi_TryExchange());tick=19;BSP_WiFi_Service();
    assert(aborts==1&&cs&&s_status.timeouts==1&&clears==3&&!disabled);
    HAL_SPI_TxRxCpltCallback(&hspi2);assert(!BSP_WiFi_GetRx());
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
    puts("PASS V-03 real BSP: READY permits, 9/10/11ms, busy/error, late callback, CS, recovery");
    return 0;
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix="spi_wifi_state_") as temp:
        work = Path(temp)
        shutil.copyfile(ROOT / "BSP/bsp_wifi.c", work / "bsp_wifi.c")
        (work / "bsp.h").write_text(BSP)
        (work / "test.c").write_text(HARNESS)
        binary = build(work, [work / "test.c"])
        subprocess.run([str(binary)], check=True)
        # Negative control proves the new scheduling model detects the original race.
        current = (work / "bsp_wifi.c").read_text(encoding="utf-8")
        patch = "    if (s_status.permits)\n    {\n        __HAL_GPIO_EXTI_CLEAR_IT(WIFI_INT_Pin);\n    }\n"
        assert current.count(patch) == 1
        (work / "bsp_wifi.c").write_text(current.replace(patch, "", 1), encoding="utf-8")
        before = build(work, [work / "test.c"], name="before")
        result = subprocess.run([str(before)], capture_output=True)
        assert result.returncode and b"s_status.permits==expected" in result.stderr
        print("PASS V-03 startup edge injection: old code fails, current code grants exactly once")


if __name__ == "__main__":
    main()
