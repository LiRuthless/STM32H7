/**
 * @file parameter_bank.c
 * @brief 四组方向参数的唯一存储。
 * @details 前台加载或设置，控制拍只读；镜像由调用方传入，不保存地址。
 */
#include "services/parameter_bank.h"

/* 前台写入、控制拍读取的电磁/光电低高档参数。 */
static param_direction_t s_direction[2][2];

/**
 * @brief 按显式模式/档位装载四个方向 bank。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @note 仅参数加载路径调用，保持电磁低/高、光电低/高顺序与字段类型。
 */
void ParamBank_LoadBanks(const param_image_v3_t *image)
{
    s_direction[0][0] = (param_direction_t){
        image->kp_x_low, image->k2p_x_low, image->kd_x_low, image->base_speed_low};
    s_direction[0][1] = (param_direction_t){
        image->kp_x_high, image->k2p_x_high, image->kd_x_high, image->base_speed_high};
    s_direction[1][0] = (param_direction_t){image->optical_kp_low, image->optical_k2p_low,
        image->optical_kd_low, image->optical_speed_low};
    s_direction[1][1] = (param_direction_t){image->optical_kp_high, image->optical_k2p_high,
        image->optical_kd_high, image->optical_speed_high};
}

/**
 * @brief 把显式四 bank 写入持久化镜像。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @note 不从被状态表覆写的活动方向增益反推参数；保留保留字段清零顺序。
 */
void ParamBank_StoreBanks(param_image_v3_t *image)
{
    /* 持久化来源是显式四 bank，不是每拍被状态配置部分覆写的活动 PID。 */
    const param_direction_t *p = &s_direction[0][0];
    image->kp_x_low = p->kp;
    image->k2p_x_low = p->k2p;
    image->kd_x_low = p->kd;
    image->base_speed_low = p->base_speed;
    p = &s_direction[0][1];
    image->kp_x_high = p->kp;
    image->k2p_x_high = p->k2p;
    image->kd_x_high = p->kd;
    image->base_speed_high = p->base_speed;
    p = &s_direction[1][0];
    image->optical_kp_low = p->kp;
    image->optical_k2p_low = p->k2p;
    image->optical_kd_low = p->kd;
    image->optical_speed_low = p->base_speed;
    p = &s_direction[1][1];
    image->optical_kp_high = p->kp;
    image->optical_k2p_high = p->k2p;
    image->optical_kd_high = p->kd;
    image->optical_speed_high = p->base_speed;
    image->optical_reserved_low = image->optical_reserved_high = 0;
}

/**
 * @brief 读取显式模式/档位的方向 bank。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @param[in] mode 0 电磁、1 光电；其他值无效。
 * @param[in] gear 0 低档、1 高档；其他值无效。
 * @return 有效索引返回非空只读指针，否则返回空指针。
 * @note 指针指向模块持有的 bank，直至后续加载或设置有效，不可直接修改。
 */
const param_direction_t *Param_GetDirection(uint8_t mode, uint8_t gear)
{
    return mode < 2 && gear < 2 ? &s_direction[mode][gear] : 0;
}

/**
 * @brief 复制有效目标的一组方向参数。
 * @details 非法模式、档位或空输入提前返回0；有效复制后返回1，不应用或标脏。
 * @param[in] mode 0电磁、1光电。
 * @param[in] gear 0低档、1高档。
 * @param[in] profile 非空输入；空指针拒绝。
 * @return 成功1；非法输入0且不写入。
 * @note 仅参数加载或停车前台写入，不访问硬件。
 */
uint8_t ParamBank_SetDirection(uint8_t mode, uint8_t gear, const param_direction_t *profile)
{
    if (mode >= 2 || gear >= 2 || !profile)
    {
        return 0;
    }
    s_direction[mode][gear] = *profile;
    return 1;
}
