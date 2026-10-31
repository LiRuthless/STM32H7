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
 * @brief 由镜像装载四组方向参数
 * @note 完整契约见 parameter_bank.c::ParamBank_LoadBanks。
 */
void ParamBank_LoadBanks(const param_image_v3_t *image);
/**
 * @brief 把四组参数按字段顺序写入镜像
 * @note 完整契约见 parameter_bank.c::ParamBank_StoreBanks。
 */
void ParamBank_StoreBanks(param_image_v3_t *image);
/**
 * @brief 读取指定模式档位的方向参数
 * @note 完整契约见 parameter_bank.c::Param_GetDirection。
 */
const param_direction_t *Param_GetDirection(uint8_t mode, uint8_t gear);
/**
 * @brief 复制有效目标的一组方向参数
 * @note 完整契约见 parameter_bank.c::ParamBank_SetDirection。
 */
uint8_t ParamBank_SetDirection(uint8_t mode, uint8_t gear, const param_direction_t *profile);

#endif /* SMARTCAR_APP_SERVICES_PARAMETER_BANK_H */
