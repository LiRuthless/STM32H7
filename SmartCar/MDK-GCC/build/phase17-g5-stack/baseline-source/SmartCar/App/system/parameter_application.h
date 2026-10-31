/**
 * @file parameter_application.h
 * @brief 参数使用者和保存流程协调。
 * @details 只声明业务应用接口；bank与镜像各自拥有存储。
 */
#ifndef SMARTCAR_APP_SYSTEM_PARAMETER_APPLICATION_H
#define SMARTCAR_APP_SYSTEM_PARAMETER_APPLICATION_H
#include "app_types.h"
#include "services/parameter_bank.h"
#include "services/parameter_store.h"

/* 速度档位（对应源 eeprom.c 的 speed_low / speed_high 两块存储区） */
#define PARAM_GEAR_LOW 0  /* 慢速档（菜单 MENU_KEY_ADJUST1） */
#define PARAM_GEAR_HIGH 1 /* 快速档（菜单 MENU_KEY_ADJUST2） */


/**
 * @brief 保存指定 bank，并按当前选择应用。
 * @note 完整契约见 parameter_application.c::Param_SetDirection。
 */
uint8_t Param_SetDirection(uint8_t mode, uint8_t gear, const param_direction_t *profile);

/**
 * @brief 把当前模式/档位的 bank 分发给控制器。
 * @note 完整契约见 parameter_application.c::Param_ApplyCurrentDirection。
 */
void Param_ApplyCurrentDirection(void);


/**
 * @brief 钳位并保存风扇参数，当前档同步活动值。
 * @note 完整契约见 parameter_application.c::Param_SetFan。
 */
void Param_SetFan(uint8_t gear, uint8_t idle, int16_t duty);

/**
 * @brief 将待写 bank 和共享参数同步到参数镜像。
 * @note 完整契约见 parameter_application.c::Param_TryFlushPending。
 */
uint8_t Param_TryFlushPending(void);

/**
 * @brief 加载镜像并依次恢复活动PI、档位和菜单。
 * @note 完整契约见 parameter_application.c::Param_Load。
 */
uint8_t Param_Load(void);

/**
 * @brief 立即标脏并尝试保存。
 * @note 完整契约见 parameter_application.c::Param_Save。
 */
void Param_Save(void);

/**
 * @brief 切换活动档位并分发方向与共用风扇值。
 * @note 完整契约见 parameter_application.c::Param_SelectGear。
 */
void Param_SelectGear(uint8_t gear);


/**
 * @brief 静默满 3000 ms 时尝试保存待写参数。
 * @note 完整契约见 parameter_application.c::Param_ServiceSave。
 */
void Param_ServiceSave(void);

/**
 * @brief 保留立即尝试保存的无返回包装。
 * @note 完整契约见 parameter_application.c::Param_FlushPending。
 */
void Param_FlushPending(void);

/**
 * @brief 设置共享速度PI镜像及活动值，保持积分历史。
 * @note 完整契约见 parameter_application.c::Param_SetSpeedGains。
 */
void Param_SetSpeedGains(float kp, float ki);

#endif /* SMARTCAR_APP_SYSTEM_PARAMETER_APPLICATION_H */
