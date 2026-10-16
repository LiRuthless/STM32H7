"""Phase 14 host verification: compile the real C modules with BSP substitutes."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import hashlib

ROOT = Path(__file__).resolve().parents[1]

MAIN_STUB = r'''
#ifndef MAIN_H
#define MAIN_H
#include <stdint.h>
typedef struct { uint32_t BSRR; } GPIO_TypeDef;
extern GPIO_TypeDef gpio;
#define GPIOE (&gpio)
#define GRAY_IO1_Pin (1u<<4)
#define GRAY_IO2_Pin (1u<<6)
#define GRAY_IO3_Pin (1u<<0)
#define GRAY_IO4_Pin (1u<<1)
typedef struct { uint32_t CR; } DMA_Stream_TypeDef;
typedef struct { DMA_Stream_TypeDef *Instance; } DMA_HandleTypeDef;
typedef struct { DMA_HandleTypeDef *DMA_Handle; } ADC_HandleTypeDef;
extern ADC_HandleTypeDef hadc1;
#define DMA_SxCR_EN 1u
#define ADC_CALIB_OFFSET 0
#define ADC_SINGLE_ENDED 0
unsigned test_counter(void);
extern int dma_error;
#define __HAL_DMA_GET_COUNTER(d) test_counter()
#define __HAL_DMA_GET_FLAG(d,f) dma_error
#define __HAL_DMA_GET_TE_FLAG_INDEX(d) 0
#define __HAL_DMA_GET_DME_FLAG_INDEX(d) 0
#define __HAL_DMA_GET_FE_FLAG_INDEX(d) 0
int HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef*,int,int);
int HAL_ADC_Start_DMA(ADC_HandleTypeDef*,uint32_t*,uint32_t);
#endif
'''
GRAY_HARNESS = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bsp_adc.c"
#include "gray_sensor.h"
#include "bsp_gray.h"
GPIO_TypeDef gpio;
DMA_Stream_TypeDef stream = {1};
DMA_HandleTypeDef dma = {&stream};
ADC_HandleTypeDef hadc1 = {&dma};
int dma_error;
static uint32_t cycles, increment=10;
static unsigned polls, conversions, stuck, initial_after, reports;
uint32_t BSP_Time_GetCycles(void) { uint32_t v=cycles; cycles+=increment; return v; }
void BSP_Time_Init(void) {}
int HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef*d,int a,int b){(void)d;(void)a;(void)b;return 0;}
int HAL_ADC_Start_DMA(ADC_HandleTypeDef*d,uint32_t*b,uint32_t n){(void)d;(void)b;(void)n;return 0;}
void BSP_UART_WriteString(const char *s) { if(strstr(s,"ch")) ++reports; }
unsigned test_counter(void)
{
    unsigned ch, bits=gpio.BSRR;
    if(stuck) return 10;
    if(initial_after) { initial_after=0; return 5; }
    if((polls++ & 1u)==0) return 10;
    ch=((bits>>4)&1u)|(((bits>>6)&1u)<<1)|((bits&1u)<<2)|(((bits>>1)&1u)<<3);
    assert(ch==conversions/5);
    { static const unsigned values[5]={10,20,30,1000,2000};
      s_adc_buf[BSP_ADC_GRAY]=(uint16_t)((values[conversions%5]+ch)<<4); }
    ++conversions;
    return 5;
}
static void reset(void) { polls=conversions=stuck=0;dma_error=0;stream.CR=1;increment=10; }
int main(void)
{
    bsp_gray_frame_t f, saved;
    int16_t b[16],w[16]; uint16_t q[16]={0}; uint8_t valid;
    BSP_Gray_Init(); memset(&f,0x55,sizeof f);
    initial_after=1;
    assert(BSP_Gray_ReadFrame(&f)==BSP_GRAY_OK && conversions==80 && f.sequence==1);
    for(unsigned i=0;i<16;++i) assert(f.raw[i]==1500+i);
    saved=f;reset();stuck=1;increment=1000;
    assert(BSP_Gray_ReadFrame(&f)==BSP_GRAY_TIMEOUT && memcmp(&f,&saved,sizeof f)==0);
    reset();dma_error=1;
    assert(BSP_Gray_ReadFrame(&f)==BSP_GRAY_DMA_ERROR && memcmp(&f,&saved,sizeof f)==0);
    reset();stream.CR=0;assert(BSP_Gray_ReadFrame(&f)==BSP_GRAY_DMA_ERROR);
    reset();cycles=0xfffffff0u;
    assert(BSP_Gray_ReadFrame(&f)==BSP_GRAY_OK && f.sequence==2 && f.scan_cycles<336000);
    assert(BSP_Gray_ReadFrame(0)==BSP_GRAY_ARG);
    for(unsigned i=0;i<16;++i){b[i]=0;w[i]=4095;}
    assert(!Gray_ValidateCalibration(b,w));
    for(unsigned i=0;i<16;++i) { b[i]=-1;assert(Gray_ValidateCalibration(b,w)==(1u<<i)); b[i]=0; }
    w[0]=199;assert(Gray_ValidateCalibration(b,w)==1);w[0]=200;assert(!Gray_ValidateCalibration(b,w));
    w[0]=4096;assert(Gray_ValidateCalibration(b,w)==1);w[0]=0;b[0]=4095;assert(!Gray_ValidateCalibration(b,w));
    assert(Gray_Normalize(0,0,1000)==0 && Gray_Normalize(1000,0,1000)==1000);
    assert(Gray_Normalize(49,0,1000)==0 && Gray_Normalize(50,0,1000)==50);
    assert(Gray_Normalize(199,0,1000)==199 && Gray_Normalize(200,0,1000)==200);
    assert(Gray_Normalize(0,1000,0)==1000 && Gray_Normalize(1000,1000,0)==0);
    assert(Gray_Normalize(0,1000,2000)==0 && Gray_Normalize(4095,0,1000)==1000);
    assert(Gray_ComputeError(q,&valid)==0 && !valid);
    q[0]=199;assert(Gray_ComputeError(q,&valid)==100 && !valid);
    q[0]=200;assert(Gray_ComputeError(q,&valid)==100 && valid);
    q[0]=0;q[15]=1000;assert(Gray_ComputeError(q,&valid)==-100 && valid);
    q[15]=0;q[7]=q[8]=1000;assert(Gray_ComputeError(q,&valid)==0 && valid);
    for(unsigned i=0;i<16;++i)q[i]=1000;
    assert(Gray_ComputeError(q,&valid)==0 && valid);
    reset();Gray_Init();assert(reports==16 && !Gray_GetState()->calibration_valid);
    Gray_Update(); assert(Gray_GetState()->percent[0]==1500*100/4095 && !Gray_GetState()->line_valid);
    saved=f;stuck=1;increment=1000;Gray_Update();
    assert(Gray_GetState()->sequence==1 && Gray_GetState()->raw[0]==1500 && Gray_GetState()->acquisition_status==BSP_GRAY_TIMEOUT);
    puts("PASS V-04/05/06: real DMA/gray C, 80 conversions, rollback, timeout/wrap, all calibration and centroid boundaries");
}
'''

def compile_run(work, name, harness, sources, includes=()):
    source = work / (name + '.c')
    source.write_text(harness, encoding='utf-8')
    binary = work / (name + '.exe')
    command = ['gcc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
               '-I', str(work), *sum((['-I', str(p)] for p in includes), []),
               str(source), *map(str, sources), '-o', str(binary)]
    subprocess.run(command, check=True)
    subprocess.run([str(binary)], check=True)

def gray_tests(work):
    for name in ('bsp_adc.c','bsp_adc.h','bsp_gray.c','bsp_gray.h','bsp_time.h'):
        shutil.copyfile(ROOT/'BSP'/name, work/name)
    (work/'main.h').write_text(MAIN_STUB, encoding='utf-8')
    (work/'adc.h').write_text('#include "main.h"\n', encoding='utf-8')
    (work/'bsp.h').write_text('#include "bsp_gray.h"\nvoid BSP_UART_WriteString(const char *);\n', encoding='utf-8')
    compile_run(work,'gray',GRAY_HARNESS,[work/'bsp_gray.c',ROOT/'App/gray_sensor.c'],[ROOT/'App'])
    source=Path('C:/Users/Li/Desktop/LQ_Track.c')
    assert hashlib.sha256(source.read_bytes()).hexdigest().upper()=='6AB7B2B68AFE7C00C6CF2096E56953D398C1E73C028D792431B90786F792DCEF'
    ioc=(ROOT/'SmartCar.ioc').read_text(encoding='utf-8')
    for pin in (0,1,4,6): assert f'PE{pin}.Signal=GPIO_Output' in ioc
    assert 'ADC1.NbrOfConversion=10' in ioc and 'ADC1.Rank-4\\#ChannelRegularConversion=5' in ioc
    print('PASS V-02/03: source SHA256, pin mapping and unchanged ten ADC ranks')

if __name__ == '__main__':
    with tempfile.TemporaryDirectory(prefix='dual_tracking_') as directory:
        gray_tests(Path(directory))
