/**
 * @file param.h
 * @brief App 参数 bank 和 v1/v2/v3 镜像兼容；Flash 只由前台访问。
 * @details 声明参数镜像、档位和持久化接口；完整布局、迁移及失败语义位于 param.c。
 */
#ifndef SMARTCAR_APP_PARAM_H
#define SMARTCAR_APP_PARAM_H

#include <stdint.h>

typedef struct
{
    float kp;           /**< 方向比例增益，保留原标度。 */
    float k2p;          /**< 方向非线性比例增益。 */
    float kd;           /**< 方向拍间差分增益。 */
    int16_t base_speed; /**< 基础速度，脉冲/2 ms。 */
} param_direction_t;

/* 速度档位（对应源 eeprom.c 的 speed_low / speed_high 两块存储区） */
#define PARAM_GEAR_LOW 0  /* 慢速档（菜单 MENU_KEY_ADJUST1） */
#define PARAM_GEAR_HIGH 1 /* 快速档（菜单 MENU_KEY_ADJUST2） */

/**
 * @brief 读取显式模式/档位的方向 bank。
 * @note 指针指向模块持有的 bank，直至后续加载或设置有效，不可直接修改。 完整契约见 param.c::Param_GetDirection。
 */
const param_direction_t *Param_GetDirection(uint8_t mode, uint8_t gear);

/**
 * @brief 保存指定 bank，并按当前选择应用。
 * @note profile 非空方向参数；空指针拒绝，不新增增益范围检查。 停车调参路径调用；写 bank 后标脏，只有当前模式及档位才更新活动增益。 完整契约见
 *     param.c::Param_SetDirection。
 */
uint8_t Param_SetDirection(uint8_t mode, uint8_t gear, const param_direction_t *profile);

/**
 * @brief 把当前模式/档位的 bank 分发给控制器。
 * @note 保持方向四增益后基础速度的顺序，阻尼固定 1.0f；不将运行状态表覆写值回收进 bank。 完整契约见 param.c::Param_ApplyCurrentDirection。
 */
void Param_ApplyCurrentDirection(void);

/**
 * @brief 读取两档共用风扇参数。
 * @note 只读，不按循迹模式拆分风扇 bank。 完整契约见 param.c::Param_GetFan。
 */
int16_t Param_GetFan(uint8_t gear, uint8_t idle);

/**
 * @brief 钳位并保存风扇参数，当前档同步活动值。
 * @note duty 原 int16_t 万分比，钳位到 0..10000。 停车前台调用，保持钳位、保存、应用、标脏顺序。 完整契约见 param.c::Param_SetFan。
 */
void Param_SetFan(uint8_t gear, uint8_t idle, int16_t duty);

/**
 * @brief 将待写 bank 和共享参数同步到参数镜像。
 * @note 停车或起跑门禁前台调用，可能阻塞；写失败锁存 Flash 不可用，标脏保持。 完整契约见 param.c::Param_TryFlushPending。
 */
uint8_t Param_TryFlushPending(void);

/**
 * @brief 读取 v1/v2/v3 镜像并迁移到原 88 B v3。
 * @note 启动前台调用，可阻塞；保持错误回显、CRC、默认/迁移值、风扇钳位和重写失败路径。 完整契约见 param.c::Param_Load。
 */
uint8_t Param_Load(void);

/**
 * @brief 立即标脏并尝试保存。
 * @note 停车菜单前台调用，可阻塞；保持忽略尝试保存返回值，失败状态由模块锁存。 完整契约见 param.c::Param_Save。
 */
void Param_Save(void);

/**
 * @brief 切换活动档位并分发方向与共用风扇值。
 * @note 停车菜单或加载路径调用，不改变当前循迹模式。 完整契约见 param.c::Param_SelectGear。
 */
void Param_SelectGear(uint8_t gear);

/**
 * @brief 标记参数待写并更新静默起点。
 * @note 停车调参路径调用，HAL 毫秒 tick；不立即写 Flash。 完整契约见 param.c::Param_MarkDirty。
 */
void Param_MarkDirty(void);

/**
 * @brief 静默满 3000 ms 时尝试保存待写参数。
 * @note 仅停车主循环调用，可阻塞；保留无符号减法回绕和失败锁存。 完整契约见 param.c::Param_ServiceSave。
 */
void Param_ServiceSave(void);

/**
 * @brief 保留立即尝试保存的无返回包装。
 * @note 前台调用，可能阻塞；保持忽略返回值，不新增错误路径。 完整契约见 param.c::Param_FlushPending。
 */
void Param_FlushPending(void);

/**
 * @brief 复制存储的共享PI。
 * @note 完整契约见param.c::Param_GetSpeedGains。
 */
void Param_GetSpeedGains(float *kp, float *ki);
/**
 * @brief 设置共享PI并标脏。
 * @note 完整契约见param.c::Param_SetSpeedGains。
 */
void Param_SetSpeedGains(float kp, float ki);
#endif /* SMARTCAR_APP_PARAM_H */
