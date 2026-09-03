/**
  ******************************************************************************
  * @file    bsp_flash.c
  * @brief   片内 Flash 模拟 EEPROM：Bank2 Sector7（0x081E0000，128KB）。
  *          H743 最小擦除单位为 128KB 扇区、最小写入单位为 32 字节 Flash Word，
  *          实现为「RAM 影子缓冲 + 整扇区读-改-擦-写」。
  *          影子缓冲 128KB（131072 字节），DTCM(RW_IRAM1) 仅 128KB 且需放栈/堆，
  *          故固定放置在 AXI SRAM 末尾 0x24060000~0x2407FFFF（RW_IRAM2 内，
  *          armlink 自动避让绝对地址段，无需修改分散加载文件）。
  ******************************************************************************
  */

/* 包含头文件 ------------------------------------------------------------------*/
#include "bsp_flash.h"
#include <string.h>

/* 私有宏定义 ------------------------------------------------------------------*/
#define BSP_FLASH_WORD_SIZE     (32u)                       /* H743 Flash Word = 32 字节 */
#define BSP_FLASH_SHADOW_ADDR   (0x24060000u)               /* AXI SRAM 末尾 128KB */

/* 私有变量 ------------------------------------------------------------------*/
/* 128KB 影子缓冲，固定放置在 AXI SRAM（armlink 自动避让绝对地址段），避免占用 DTCM */
#if defined(__CC_ARM)
__attribute__((at(0x24060000)))
static uint8_t s_flash_shadow[BSP_FLASH_SECTOR_SIZE];
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__attribute__((section(".ARM.__at_0x24060000"), zero_init))
static uint8_t s_flash_shadow[BSP_FLASH_SECTOR_SIZE];
#else
static uint8_t s_flash_shadow[BSP_FLASH_SECTOR_SIZE];       /* 其他工具链：默认 .bss */
#endif

/* 私有函数定义 -------------------------------------------*/

/* 整扇区擦除 + 影子缓冲回写，调用前需已关中断，返回 0=成功 */
static uint8_t flash_erase_program(void)
{
  FLASH_EraseInitTypeDef erase_init;
  uint32_t page_error = 0;
  uint32_t addr;
  uint8_t  ret = 0;

  erase_init.TypeErase    = FLASH_TYPEERASE_SECTORS;
  erase_init.Banks        = FLASH_BANK_2;
  erase_init.Sector       = FLASH_SECTOR_7;
  erase_init.NbSectors    = 1;
  erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;

  /* 内容未变化则直接返回，避免无意义的整扇区擦写 */
  if (0 == memcmp(s_flash_shadow, (const void *)(uintptr_t)BSP_FLASH_BASE_ADDR,
                  BSP_FLASH_SECTOR_SIZE))
  {
    return 0;
  }

  if (HAL_OK != HAL_FLASH_Unlock())
  {
    return 1;
  }

  /* 擦除整扇区（约 1~2s） */
  if (HAL_OK != HAL_FLASHEx_Erase(&erase_init, &page_error))
  {
    ret = 1;
  }
  else
  {
    /* 按 32 字节 Flash Word 回写，跳过擦除态（全 0xFF）的字，大幅缩短关中断时间 */
    for (addr = 0; addr < BSP_FLASH_SECTOR_SIZE; addr += BSP_FLASH_WORD_SIZE)
    {
      uint32_t i;
      uint8_t  all_erased = 1;
      for (i = 0; i < BSP_FLASH_WORD_SIZE; i++)
      {
        if (0xFFu != s_flash_shadow[addr + i])
        {
          all_erased = 0;
          break;
        }
      }
      if (all_erased)
      {
        continue;
      }
      if (HAL_OK != HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                                      BSP_FLASH_BASE_ADDR + addr,
                                      (uint32_t)(uintptr_t)&s_flash_shadow[addr]))
      {
        ret = 1;
        break;
      }
    }
  }

  (void)HAL_FLASH_Lock();
  return ret;
}

/* 导出函数定义 -------------------------------------------*/

/**
  * @brief  初始化：把当前扇区内容加载到影子缓冲（预热，保持与头文件约定一致）
  *         无硬件配置需求；首次使用前不调用也不影响 Read/Write 正确性
  * @retval 无
  */
void BSP_Flash_Init(void)
{
  memcpy(s_flash_shadow, (const void *)(uintptr_t)BSP_FLASH_BASE_ADDR, BSP_FLASH_SECTOR_SIZE);
}

/**
  * @brief  按逻辑偏移读取（直接 memcpy 自 Flash 映射地址）
  * @retval 0=成功，1=参数越界/为空
  */
uint8_t BSP_Flash_Read(uint32_t offset, uint8_t *buf, uint32_t len)
{
  if ((0 == buf) || (0u == len) ||
      (offset >= BSP_FLASH_SECTOR_SIZE) ||
      (len > (BSP_FLASH_SECTOR_SIZE - offset)))
  {
    return 1;
  }
  memcpy(buf, (const void *)(uintptr_t)(BSP_FLASH_BASE_ADDR + offset), len);
  return 0;
}

/**
  * @brief  按逻辑偏移写入：整扇区读-改-擦-写，全程关中断（约 1~2s）
  * @retval 0=成功，1=参数错误或擦写/校验失败
  */
uint8_t BSP_Flash_Write(uint32_t offset, const uint8_t *buf, uint32_t len)
{
  uint32_t primask;
  uint8_t  ret;

  if ((0 == buf) || (0u == len) ||
      (offset >= BSP_FLASH_SECTOR_SIZE) ||
      (len > (BSP_FLASH_SECTOR_SIZE - offset)))
  {
    return 1;
  }

  /* 影子缓冲以 Flash 当前内容为准，再应用本次修改 */
  memcpy(s_flash_shadow, (const void *)(uintptr_t)BSP_FLASH_BASE_ADDR, BSP_FLASH_SECTOR_SIZE);
  memcpy(&s_flash_shadow[offset], buf, len);

  /* 临界区：擦写期间禁止中断（SysTick 一并暂停，擦写约 1~2s） */
  primask = __get_PRIMASK();
  __disable_irq();
  ret = flash_erase_program();
  __set_PRIMASK(primask);

  /* 回读校验 */
  if (0u == ret)
  {
    if (0 != memcmp((const void *)(uintptr_t)(BSP_FLASH_BASE_ADDR + offset), buf, len))
    {
      ret = 1;
    }
  }
  return ret;
}
