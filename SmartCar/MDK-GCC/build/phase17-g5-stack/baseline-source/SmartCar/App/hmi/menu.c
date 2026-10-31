/**
 * @file menu.c
 * @brief 停车菜单导航与编辑。
 * @details 持有页面、光标、编辑模式、待保存和按键沿；按键采集与绘制分别委托独立owner。
 */
/*
 * 移植来源：Sirius20260718/project/code/menu.c，STC AI8051U省赛快照（2026-07-18）。
 * 原算法未单列作者或独立许可证；导航、采集和绘制职责由Phase17拆分。
 */
#include "bsp.h"
#include "app_config.h"
#include "app.h"
#include "sensing/battery.h"
#include "hmi/menu.h"
#include "system/parameter_application.h"
#include "sensing/track_sensor.h"
#include "sensing/imu_proc.h"
#include "control/pid.h"
#include "control/tracking.h"
#include "control/motor.h"
#include "sensing/filter.h"

#include "hmi/menu_keys.h"
#include "hmi/menu_render.h"

/* 停车前台拥有导航及编辑状态，参数加载仅恢复页面和光标。 */
static uint8_t s_page = MENU_PAGE_HOME;
static uint8_t s_arrow = 1;
static uint8_t s_mode = 1;
static uint8_t s_param_dirty = 0;
static uint8_t s_fan_edit_idle = 0;

/**
 * @brief 执行原菜单导航与调参动作。
 * @details 入口先复制当前模式、档位及bank，即使只是导航也执行该读取。
 *     浏览按页面项数循环，BACK调参先保存旧位置再回主页；左右编辑保留五个独立分支和重复标脏。
 *     调参快捷键先切页面、应用档位，再选择运行风扇及编辑模式，保留原光标。
 * @param[in] key MENU_KEY_* 键值，未知值不操作。
 * @note 仅停车前台；公开声明供协调和宿主验证，活动调用仍由Menu_Update处理按键沿。
 */
void Menu_HandleKey(uint8_t key)
{
    uint8_t current_mode = Tracking_GetState()->mode;
    uint8_t gear = Tracking_GetState()->gear;
    param_direction_t direction = *Param_GetDirection(current_mode, gear);
    switch (key)
    {
        case MENU_KEY_UP:
            if (s_page == MENU_PAGE_ADJUST1 || s_page == MENU_PAGE_ADJUST2)
            {
                s_arrow = (s_arrow == 1) ? 5 : s_arrow - 1; /* 调参页 5 项 */
            }
            else if (s_page == MENU_PAGE_HOME)
            {
                s_arrow = (s_arrow == 1) ? 3 : s_arrow - 1; /* 主页 3 项 */
            }
            else
            {
                s_arrow = (s_arrow == 1) ? 4 : s_arrow - 1;
            }
            break;

        case MENU_KEY_DOWN:
            if (s_page == MENU_PAGE_ADJUST1 || s_page == MENU_PAGE_ADJUST2)
            {
                s_arrow = (s_arrow == 5) ? 1 : s_arrow + 1;
            }
            else if (s_page == MENU_PAGE_HOME)
            {
                s_arrow = (s_arrow == 3) ? 1 : s_arrow + 1;
            }
            else
            {
                s_arrow = (s_arrow == 4) ? 1 : s_arrow + 1;
            }
            break;

        case MENU_KEY_OK:
            if (s_page == MENU_PAGE_HOME)
            {
                s_page = 20 + s_arrow; /* 主页 → ADC_ERR/SPD_DIS/GYRO */
                s_arrow = 1;
            }
            else if ((s_page == MENU_PAGE_ADJUST1 || s_page == MENU_PAGE_ADJUST2)
                && s_arrow == 5)
            {
                s_fan_edit_idle = !s_fan_edit_idle;
            }
            break;

        case MENU_KEY_BACK:
            if (s_page == MENU_PAGE_ADJUST1 || s_page == MENU_PAGE_ADJUST2)
            {
                Param_Save(); /* 退出调参页保存参数（对应源 write_speed_low/high） */
                s_page = MENU_PAGE_HOME;
                s_arrow = 1;
                s_mode = 1;
            }
            else if (s_page == MENU_PAGE_ADC_ERR || s_page == MENU_PAGE_SPD_DIS
                || s_page == MENU_PAGE_GYRO)
            {
                s_page = MENU_PAGE_HOME;
                s_arrow = 1;
            }
            break;

        case MENU_KEY_LEFT:
            if ((s_page == MENU_PAGE_ADJUST1 || s_page == MENU_PAGE_ADJUST2)
                && s_mode == 2)
            {
                if (s_arrow == 1)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t)
    {
        direction.kp - 0.001f, direction.k2p, direction.kd,
                            direction.base_speed});
                }
                if (s_arrow == 2)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t)
    {
        direction.kp, direction.k2p - 0.001f, direction.kd,
                            direction.base_speed});
                }
                if (s_arrow == 3)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t)
    {
        direction.kp, direction.k2p, direction.kd - 0.001f,
                            direction.base_speed});
                }
                if (s_arrow == 4)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t)
    {
        direction.kp, direction.k2p, direction.kd,
                            (int16_t)(direction.base_speed - 100)});
                }
                if (s_arrow == 5)
                {
                    int32_t duty = s_fan_edit_idle ? Motor_GetState()->fan_duty_idle
                                                   : Motor_GetState()->fan_duty;
                    duty -= 100;
                    if (duty < 0)
                    {
                        duty = 0;
                    }
                    Param_SetFan(gear, s_fan_edit_idle, (int16_t)duty);
                }
                s_param_dirty = 1;
                Param_MarkDirty();
            }
            break;

        case MENU_KEY_RIGHT:
            if ((s_page == MENU_PAGE_ADJUST1 || s_page == MENU_PAGE_ADJUST2)
                && s_mode == 2)
            {
                if (s_arrow == 1)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t)
    {
        direction.kp + 0.001f, direction.k2p, direction.kd,
                            direction.base_speed});
                }
                if (s_arrow == 2)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t)
    {
        direction.kp, direction.k2p + 0.001f, direction.kd,
                            direction.base_speed});
                }
                if (s_arrow == 3)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t)
    {
        direction.kp, direction.k2p, direction.kd + 0.001f,
                            direction.base_speed});
                }
                if (s_arrow == 4)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t)
    {
        direction.kp, direction.k2p, direction.kd,
                            (int16_t)(direction.base_speed + 100)});
                }
                if (s_arrow == 5)
                {
                    int32_t duty = s_fan_edit_idle ? Motor_GetState()->fan_duty_idle
                                                   : Motor_GetState()->fan_duty;
                    duty += 100;
                    if (duty > 10000)
                    {
                        duty = 10000;
                    }
                    Param_SetFan(gear, s_fan_edit_idle, (int16_t)duty);
                }
                s_param_dirty = 1;
                Param_MarkDirty();
            }
            break;

        case MENU_KEY_ADJUST1:
            s_page = MENU_PAGE_ADJUST1;
            Param_SelectGear(PARAM_GEAR_LOW); /* 对应源 read_speed_low() */
            s_fan_edit_idle = 0;
            s_mode = 2;
            break;

        case MENU_KEY_ADJUST2:
            s_page = MENU_PAGE_ADJUST2;
            Param_SelectGear(PARAM_GEAR_HIGH); /* 对应源 read_speed_high() */
            s_fan_edit_idle = 0;
            s_mode = 2;
            break;

        default:
            break;
    }
}

/**
 * @brief 处理停车菜单按键沿、保存及显示。
 * @details 先扫描并处理无键沿；RST新沿清屏失效后立即返回，当拍不导航、保存或绘制。
 *     其余新沿先处理业务，再准备页面；有菜单脏且松键才保存，最后依次绘制内容和光标。
 * @note 仅外接屏停车前台调用，可阻塞；保留无键沿、复位键早退、页面刷新与松键保存顺序。
 */
void Menu_Update(void)
{
    static uint8_t s_last_key = 0;

    uint8_t key = Menu_ScanKey();

    if (key == 0) /* 无键清零（消抖沿检测） */
    {
        s_last_key = 0;
    }

    /* MENU_KEY_RST键：清屏重绘 */
    if (key == MENU_KEY_RST && key != s_last_key)
    {
        MenuRender_Reset();
        s_last_key = key;
        return;
    }

    /* 按键按下沿检测 */
    if (key != s_last_key && key != 0)
    {
        Menu_HandleKey(key);
        s_last_key = key;
    }

    /* 页面切换时清屏，避免每帧闪烁 */
    MenuRender_PreparePage();

    /* 参数修改后、按键松开时写一次 Flash
     *（源工程 MENU_KEY_ADJUST1 页每帧写 Flash，此处改为空闲时单次写入，减少擦除损耗） */
    if (s_param_dirty && key == 0)
    {
        Param_Save();
        s_param_dirty = 0;
    }

    MenuRender_Draw();
}

/**
 * @brief 读取当前页面编号。
 * @return 当前页面编号原值。
 */
uint8_t Menu_GetPage(void)
{
    return s_page;
}

/**
 * @brief 读取当前光标编号。
 * @return 当前光标编号原值。
 */
uint8_t Menu_GetArrow(void)
{
    return s_arrow;
}

/**
 * @brief 读取当前风扇编辑选择。
 * @return 当前风扇编辑选择原值。
 */
uint8_t Menu_GetFanEditIdle(void)
{
    return s_fan_edit_idle;
}

/**
 * @brief 按镜像合法性恢复页面与光标。
 * @details 只接受六个既定页面及1..5光标；不改编辑模式、滤波、按键沿或绘制缓存。
 * @param[in] page 镜像页面原字节。
 * @param[in] arrow 镜像光标原字节。
 */
void Menu_RestorePosition(uint8_t page, uint8_t arrow)
{
    if (page == MENU_PAGE_HOME || page == MENU_PAGE_ADC_ERR
        || page == MENU_PAGE_SPD_DIS || page == MENU_PAGE_GYRO
        || page == MENU_PAGE_ADJUST1 || page == MENU_PAGE_ADJUST2)
    {
        s_page = page;
    }
    else
    {
        s_page = MENU_PAGE_HOME;
    }
    s_arrow = (arrow >= 1 && arrow <= 5) ? arrow : 1;
}
