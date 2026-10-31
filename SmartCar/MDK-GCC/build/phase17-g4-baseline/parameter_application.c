/**
 * @file parameter_application.c
 * @brief 参数应用与保存协调。
 * @details 停车前台显式连接bank、镜像、控制和菜单；无脏或不可用时不捕获活动快照。
 */
#include "system/parameter_application.h"
#include "services/parameter_bank.h"
#include "services/parameter_store.h"
#include "control/tracking.h"
#include "control/pid.h"
#include "control/motor.h"
#include "hmi/menu.h"

/**
 * @brief 把当前模式/档位的 bank 分发给控制器。
 * @details 按现有调用顺序使用 Param_GetDirection、Tracking_GetState、PID_SetTrackGains、Motor_SetBaseSpeed
 *     完成本函数处理。
 * @note 保持方向四增益后基础速度的顺序，阻尼固定 1.0f；不将运行状态表覆写值回收进 bank。
 */
void Param_ApplyCurrentDirection(void)
{
    const param_direction_t *p
        = Param_GetDirection(Tracking_GetState()->mode, Tracking_GetState()->gear);
    const pid_track_gains_t gains = {p->kp, p->k2p, p->kd, 1.0f};
    PID_SetTrackGains(&gains);
    Motor_SetBaseSpeed(p->base_speed);
}

/**
 * @brief 保存指定 bank，并按当前选择应用。
 * @details 按现有调用顺序使用 Param_MarkDirty、Tracking_GetState、Param_ApplyCurrentDirection
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] mode 0 电磁、1 光电，其他值拒绝。
 * @param[in] gear 0 低档、1 高档，其他值拒绝。
 * @param[in] p p 为调用方提供的只读输入，必须指向满足接口容量要求的有效对象。
 * @return 有效索引且指针非空返回 1，否则 0 且不写 bank。
 * @note 停车调参路径调用；写 bank 后标脏，只有当前模式及档位才更新活动增益。
 */
uint8_t Param_SetDirection(uint8_t mode, uint8_t gear, const param_direction_t *p)
{
    /* 先校验模式、档位和输入指针，再复制一组方向参数；若目标正是当前模式/档位则立即应用，最后标记延迟保存。 */
    if (!ParamBank_SetDirection(mode, gear, p))
    {
        return 0;
    }
    Param_MarkDirty();
    if (mode == Tracking_GetState()->mode && gear == Tracking_GetState()->gear)
    {
        Param_ApplyCurrentDirection();
    }
    return 1;
}

/**
 * @brief 切换活动档位并分发方向与共用风扇值。
 * @details 按现有调用顺序使用 Tracking_SetGear、Param_ApplyCurrentDirection、Motor_SetFanDuty、Param_GetFan
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] gear 0 低档、1 高档，其他值不操作。
 * @note 停车菜单或加载路径调用，不改变当前循迹模式。
 */
void Param_SelectGear(uint8_t gear)
{
    if (gear >= 2)
    {
        return;
    }
    Tracking_SetGear(gear);
    Param_ApplyCurrentDirection();
    Motor_SetFanDuty(Param_GetFan(gear, 0));
    Motor_SetFanDutyIdle(Param_GetFan(gear, 1));
}

/**
 * @brief 立即标脏并尝试保存。
 * @details 按现有调用顺序使用 Param_TryFlushPending 完成本函数处理。
 * @note 停车菜单前台调用，可阻塞；保持忽略尝试保存返回值，失败状态由模块锁存。
 */
void Param_Save(void)
{
    ParamStore_RequestSave();
    (void)Param_TryFlushPending();
}

/**
 * @brief 静默满 3000 ms 时尝试保存待写参数。
 * @details 按现有调用顺序使用 BSP_Time_GetTick、Param_TryFlushPending 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 仅停车主循环调用，可阻塞；保留无符号减法回绕和失败锁存。
 */
void Param_ServiceSave(void)
{
    if (ParamStore_IsSaveDue())
    {
        (void)Param_TryFlushPending();
    }
}

/**
 * @brief 保留立即尝试保存的无返回包装。
 * @note 前台调用，可能阻塞；保持忽略返回值，不新增错误路径。
 */
void Param_FlushPending(void)
{
    (void)Param_TryFlushPending();
}

/**
 * @brief 加载镜像并依次恢复活动PI、档位和菜单。
 * @details 底层读错仍应用默认RAM；必要重写失败不回滚已装载参数。
 * @return Flash可用1；读取或重写失败0。
 * @note 仅上电前台，可阻塞。
 */
uint8_t Param_Load(void)
{
    uint8_t usable = ParamStore_Load();
    float kp;
    float ki;
    uint8_t page;
    uint8_t arrow;
    Param_GetSpeedGains(&kp, &ki);
    PID_SetGain(PID_GAIN_KP_V, kp);
    PID_SetGain(PID_GAIN_KI_V, ki);
    Param_SelectGear(PARAM_GEAR_LOW);
    ParamStore_GetPosition(&page, &arrow);
    Menu_RestorePosition(page, arrow);
    return usable;
}

/**
 * @brief 钳位并保存风扇参数，当前档同步活动值。
 * @details 先把输入占空比限制到 0..10000，再按档位和运行/空闲索引写入对应参数槽；其他三项风扇参数保持不变。
 * @param[in] gear 0 低档、1 高档；其他值不操作。
 * @param[in] idle 0 运行值，非零空闲值。
 * @param[in] value 风扇占空比，单位为万分比；写入前钳位到 0..10000。
 * @note 停车前台调用；有效档位会更新镜像、按当前档位应用输出并标记延迟保存。
 */
void Param_SetFan(uint8_t gear, uint8_t idle, int16_t value)
{
    /* 先校验档位并钳位万分比，再按 idle 选择运行或空闲槽；当前档位同步更新输出，最后标记延迟保存。 */
    if (gear >= 2)
    {
        return;
    }
    ParamStore_SetFan(gear, idle, value);
    value = Param_GetFan(gear, idle);
    if (gear == Tracking_GetState()->gear)
    {
        if (idle)
        {
            Motor_SetFanDutyIdle(value);
        }
        else
        {
            Motor_SetFanDuty(value);
        }
    }
    Param_MarkDirty();
}

/**
 * @brief 设置共享速度PI镜像及活动值，保持积分历史。
 * @details 先更新镜像共享值，再分发活动PI并标记原延迟保存，不调用积分复位。
 * @param[in] kp 已由调用方校验的有限比例增益。
 * @param[in] ki 已由调用方校验的有限积分增益。
 * @note 仅停车前台，标脏但不Flash；不改变88B镜像布局。
 */
void Param_SetSpeedGains(float kp, float ki)
{
    ParamStore_SetSpeedGains(kp, ki);
    PID_SetGain(PID_GAIN_KP_V, kp);
    PID_SetGain(PID_GAIN_KI_V, ki);
    Param_MarkDirty();
}

/**
 * @brief 将待写 bank 和共享参数同步到参数镜像。
 * @details 按现有调用顺序使用 PID_GetState、Param_WriteMirror 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @return 可用且无待写或保存成功为 1，Flash 不可用或保存失败为 0。
 * @note 停车或起跑门禁前台调用，可能阻塞；写失败锁存 Flash 不可用，标脏保持。
 */
uint8_t Param_TryFlushPending(void)
{
    /* Flash 不可用立即失败；无待保存直接成功，否则尝试写镜像，成功后清脏标志，失败保留待保存状态。 */
    if (!ParamStore_IsUsable())
    {
        return 0;
    }
    if (!ParamStore_IsDirty())
    {
        return 1;
    }
    param_save_snapshot_t runtime;
    runtime.kp_v = PID_GetState()->kp_v;
    runtime.ki_v = PID_GetState()->ki_v;
    runtime.page = Menu_GetPage();
    runtime.arrow = Menu_GetArrow();
    return ParamStore_TryFlushPending(&runtime);
}
