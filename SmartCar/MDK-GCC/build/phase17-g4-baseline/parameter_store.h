/**
 * @file parameter_store.h
 * @brief 参数镜像加载及保存输入。
 * @details 保存只接收瞬时活动快照；具体应用由system协调。
 */
#ifndef SMARTCAR_APP_SERVICES_PARAMETER_STORE_H
#define SMARTCAR_APP_SERVICES_PARAMETER_STORE_H
#include "services/parameter_image.h"

/**
 * @brief 访问参数镜像或保存状态
 * @note 完整契约见 parameter_store.c::ParamStore_Load。
 */
uint8_t ParamStore_Load(void);
/**
 * @brief 访问参数镜像或保存状态
 * @note 完整契约见 parameter_store.c::ParamStore_GetPosition。
 */
void ParamStore_GetPosition(uint8_t *page, uint8_t *arrow);
/**
 * @brief 访问参数镜像或保存状态
 * @note 完整契约见 parameter_store.c::ParamStore_SetFan。
 */
void ParamStore_SetFan(uint8_t gear, uint8_t idle, int16_t duty);
/**
 * @brief 访问参数镜像或保存状态
 * @note 完整契约见 parameter_store.c::ParamStore_SetSpeedGains。
 */
void ParamStore_SetSpeedGains(float kp, float ki);
/**
 * @brief 访问参数镜像或保存状态
 * @note 完整契约见 parameter_store.c::ParamStore_IsUsable。
 */
uint8_t ParamStore_IsUsable(void);
/**
 * @brief 访问参数镜像或保存状态
 * @note 完整契约见 parameter_store.c::ParamStore_IsDirty。
 */
uint8_t ParamStore_IsDirty(void);
/**
 * @brief 访问参数镜像或保存状态
 * @note 完整契约见 parameter_store.c::ParamStore_RequestSave。
 */
void ParamStore_RequestSave(void);
/**
 * @brief 访问参数镜像或保存状态
 * @note 完整契约见 parameter_store.c::ParamStore_IsSaveDue。
 */
uint8_t ParamStore_IsSaveDue(void);
/**
 * @brief 访问参数镜像或保存状态
 * @note 完整契约见 parameter_store.c::ParamStore_TryFlushPending。
 */
uint8_t ParamStore_TryFlushPending(const param_save_snapshot_t *runtime);
/**
 * @brief 访问参数镜像或保存状态
 * @note 完整契约见 parameter_store.c::Param_GetFan。
 */
int16_t Param_GetFan(uint8_t gear, uint8_t idle);
/**
 * @brief 访问参数镜像或保存状态
 * @note 完整契约见 parameter_store.c::Param_GetSpeedGains。
 */
void Param_GetSpeedGains(float *kp, float *ki);
/**
 * @brief 访问参数镜像或保存状态
 * @note 完整契约见 parameter_store.c::Param_MarkDirty。
 */
void Param_MarkDirty(void);

#endif /* SMARTCAR_APP_SERVICES_PARAMETER_STORE_H */
