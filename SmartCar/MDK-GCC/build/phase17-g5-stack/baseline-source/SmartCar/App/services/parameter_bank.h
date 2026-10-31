/**
 * @file parameter_bank.h
 * @brief 四组方向参数访问。
 * @details 只声明bank读写，不应用控制参数或访问Flash。
 */
#ifndef SMARTCAR_APP_SERVICES_PARAMETER_BANK_H
#define SMARTCAR_APP_SERVICES_PARAMETER_BANK_H
#include "app_types.h"
#include "services/parameter_image.h"

/**
 * @brief 按显式模式/档位装载四个方向 bank。
 * @note 完整契约见 parameter_bank.c::ParamBank_LoadBanks。
 */
void ParamBank_LoadBanks(const param_image_v3_t *image);
/**
 * @brief 把显式四 bank 写入持久化镜像。
 * @note 完整契约见 parameter_bank.c::ParamBank_StoreBanks。
 */
void ParamBank_StoreBanks(param_image_v3_t *image);
/**
 * @brief 读取显式模式/档位的方向 bank。
 * @note 完整契约见 parameter_bank.c::Param_GetDirection。
 */
const param_direction_t *Param_GetDirection(uint8_t mode, uint8_t gear);
/**
 * @brief 复制有效目标的一组方向参数。
 * @note 完整契约见 parameter_bank.c::ParamBank_SetDirection。
 */
uint8_t ParamBank_SetDirection(uint8_t mode, uint8_t gear, const param_direction_t *profile);

#endif /* SMARTCAR_APP_SERVICES_PARAMETER_BANK_H */
