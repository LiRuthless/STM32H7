/**
 * @file param.h
 * @brief App 参数 bank 和 v1/v2/v3 镜像兼容；Flash 只由前台访问。
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
 * @param[in] mode 0 电磁、1 光电；其他值无效。
 * @param[in] gear 0 低档、1 高档；其他值无效。
 * @return 有效索引返回非空只读指针，否则返回空指针。
 * @note 指针指向模块持有的 bank，直至后续加载或设置有效，不可直接修改。
 */
const param_direction_t *Param_GetDirection(uint8_t mode, uint8_t gear);

/**
 * @brief 保存指定 bank，并按当前选择应用。
 * @param[in] mode 0 电磁、1 光电，其他值拒绝。
 * @param[in] gear 0 低档、1 高档，其他值拒绝。
 * @param[in] profile 非空方向参数；空指针拒绝，不新增增益范围检查。
 * @return 有效索引且指针非空返回 1，否则 0 且不写 bank。
 * @note 停车调参路径调用；写 bank 后标脏，只有当前模式及档位才更新活动增益。
 */
uint8_t Param_SetDirection(uint8_t mode, uint8_t gear, const param_direction_t *profile);

/**
 * @brief 把当前模式/档位的 bank 分发给控制器。
 * @note 保持方向四增益后基础速度的顺序，阻尼固定 1.0f；不将运行状态表覆写值回收进 bank。
 */
void Param_ApplyCurrentDirection(void);

/**
 * @brief 读取两档共用风扇参数。
 * @param[in] gear 0 低档、1 高档；其他值返回 0。
 * @param[in] idle 0 运行值，非零空闲值。
 * @return 有效档位返回原 int16_t 万分比，约定 0..10000。
 * @note 只读，不按循迹模式拆分风扇 bank。
 */
int16_t Param_GetFan(uint8_t gear, uint8_t idle);

/**
 * @brief 钳位并保存风扇参数，当前档同步活动值。
 * @param[in] gear 0 低档、1 高档；其他值不操作。
 * @param[in] idle 0 运行值，非零空闲值。
 * @param[in] duty 原 int16_t 万分比，钳位到 0..10000。
 * @note 停车前台调用，保持钳位、保存、应用、标脏顺序。
 */
void Param_SetFan(uint8_t gear, uint8_t idle, int16_t duty);

/**
 * @brief 将待写 bank 和共享参数同步到参数镜像。
 * @return 可用且无待写或保存成功为 1，Flash 不可用或保存失败为 0。
 * @note 停车或起跑门禁前台调用，可能阻塞；写失败锁存 Flash 不可用，标脏保持。
 */
uint8_t Param_TryFlushPending(void);

/**
 * @brief 读取 v1/v2/v3 镜像并迁移到原 88 B v3。
 * @return Flash 可继续服务返回 1，读错或必要重写失败返回 0；RAM 状态仍按原流程装载。
 * @note 启动前台调用，可阻塞；保持错误回显、CRC、默认/迁移值、风扇钳位和重写失败路径。
 */
uint8_t Param_Load(void);

/**
 * @brief 立即标脏并尝试保存。
 * @note 停车菜单前台调用，可阻塞；保持忽略尝试保存返回值，失败状态由模块锁存。
 */
void Param_Save(void);

/**
 * @brief 切换活动档位并分发方向与共用风扇值。
 * @param[in] gear 0 低档、1 高档，其他值不操作。
 * @note 停车菜单或加载路径调用，不改变当前循迹模式。
 */
void Param_SelectGear(uint8_t gear);

/**
 * @brief 标记参数待写并更新静默起点。
 * @note 停车调参路径调用，HAL 毫秒 tick；不立即写 Flash。
 */
void Param_MarkDirty(void);

/**
 * @brief 静默满 3000 ms 时尝试保存待写参数。
 * @note 仅停车主循环调用，可阻塞；保留无符号减法回绕和失败锁存。
 */
void Param_ServiceSave(void);

/**
 * @brief 保留立即尝试保存的无返回包装。
 * @note 前台调用，可能阻塞；保持忽略返回值，不新增错误路径。
 */
void Param_FlushPending(void);

#endif /* SMARTCAR_APP_PARAM_H */
