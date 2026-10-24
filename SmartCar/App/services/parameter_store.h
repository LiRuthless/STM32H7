/**
 * @file parameter_store.h
 * @brief 参数镜像加载及保存输入。
 * @details 保存只接收瞬时活动快照；具体应用由system协调。
 */
#ifndef SMARTCAR_APP_SERVICES_PARAMETER_STORE_H
#define SMARTCAR_APP_SERVICES_PARAMETER_STORE_H
#include "services/parameter_image.h"

/**
 * @brief 读取 v1/v2/v3 镜像并迁移到原 88 B v3。
 * @note 完整契约见 parameter_store.c::ParamStore_Load。
 */
uint8_t ParamStore_Load(void);
/**
 * @brief 复制镜像的菜单位置原字节。
 * @note 完整契约见 parameter_store.c::ParamStore_GetPosition。
 */
void ParamStore_GetPosition(uint8_t *page, uint8_t *arrow);
/**
 * @brief 只更新指定档位的运行或空闲风扇镜像。
 * @note 完整契约见 parameter_store.c::ParamStore_SetFan。
 */
void ParamStore_SetFan(uint8_t gear, uint8_t idle, int16_t duty);
/**
 * @brief 只更新共享速度PI镜像。
 * @note 完整契约见 parameter_store.c::ParamStore_SetSpeedGains。
 */
void ParamStore_SetSpeedGains(float kp, float ki);
/**
 * @brief 读取参数Flash可用状态。
 * @note 完整契约见 parameter_store.c::ParamStore_IsUsable。
 */
uint8_t ParamStore_IsUsable(void);
/**
 * @brief 读取参数待保存状态。
 * @note 完整契约见 parameter_store.c::ParamStore_IsDirty。
 */
uint8_t ParamStore_IsDirty(void);
/**
 * @brief 标记立即保存请求。
 * @note 完整契约见 parameter_store.c::ParamStore_RequestSave。
 */
void ParamStore_RequestSave(void);
/**
 * @brief 检查静默3000ms保存期限。
 * @note 完整契约见 parameter_store.c::ParamStore_IsSaveDue。
 */
uint8_t ParamStore_IsSaveDue(void);
/**
 * @brief 将待写 bank 和共享参数同步到参数镜像。
 * @note 完整契约见 parameter_store.c::ParamStore_TryFlushPending。
 */
uint8_t ParamStore_TryFlushPending(const param_save_snapshot_t *runtime);
/**
 * @brief 读取两档共用风扇参数。
 * @note 完整契约见 parameter_store.c::Param_GetFan。
 */
int16_t Param_GetFan(uint8_t gear, uint8_t idle);
/**
 * @brief 复制持久化镜像中的共享速度PI。
 * @note 完整契约见 parameter_store.c::Param_GetSpeedGains。
 */
void Param_GetSpeedGains(float *kp, float *ki);
/**
 * @brief 标记参数待写并更新静默起点。
 * @note 完整契约见 parameter_store.c::Param_MarkDirty。
 */
void Param_MarkDirty(void);

#endif /* SMARTCAR_APP_SERVICES_PARAMETER_STORE_H */
