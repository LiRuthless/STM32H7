
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
