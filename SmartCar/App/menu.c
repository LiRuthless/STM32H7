/**
 * @file menu.c
 * @brief App 外接屏菜单与 ADC 按键调参；只在停车前台调用。
 * @note 移植来源：Sirius20260718/project/code/menu.c。
 * @note STC AI8051U 省赛快照（2026-07-18）；原算法未单列作者或独立许可证。
 * @note 保留源工程出处，关联 Phase 13/14/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include "app.h"
#include "menu.h"
#include "param.h"
#include "track_sensor.h"
#include "imu_proc.h"
#include "pid.h"
#include "control.h"
#include "motor.h"
#include "filter.h"

/* 小屏布局常量（像素） */
#define MENU_LCD_COL_LABEL 0    /* 标签列 */
#define MENU_LCD_COL_VAL 72     /* 数值列 */
#define MENU_LCD_COL_CURSOR 144 /* 光标 "<<" 列 */
#define MENU_LCD_ROW(n) (16 * (n))

#define MENU_TXT_FG BSP_LCD_BLACK
#define MENU_TXT_BG BSP_LCD_WHITE
#define MENU_CUR_FG BSP_LCD_RED

/* ==================== 菜单状态变量 ==================== */
/* 停车前台持有页面、编辑、滤波和前次显示状态；只在既有参数加载路径恢复页面/光标。 */
uint8_t g_menu_page = MENU_PAGE_HOME; /* 当前页面编号 */
uint8_t g_menu_arrow = 1;             /* 当前光标位置（从1开始计数） */
static uint8_t s_mode = 1;            /* 菜单模式（1=浏览，2=调参） */
uint16_t g_menu_key_adc1 = 0;         /* 按键ADC通道1采样值 */
uint16_t g_menu_key_adc2 = 0;         /* 按键ADC通道2采样值 */

static uint8_t s_param_dirty = 0;   /* 参数被修改待保存 */
static uint8_t s_fan_edit_idle = 0; /* g_menu_arrow=5 时当前编辑空闲风扇值 */

/* 显示状态（减少不必要刷新与闪烁） */
static uint16_t s_last_displayed_page = 0xFFFF;
static uint8_t s_last_displayed_arrow = 0xFF;

static filter_lowpass_t s_filt_key1;
static filter_lowpass_t s_filt_key2;
static uint8_t s_filt_inited = 0;

/* ==================== 静态函数声明 ==================== */
static void Menu_DrawContent(void);
static void Menu_DrawCursor(void);

/* ==================== 按键扫描 ==================== */

/* ADC按键扫描：两路分压键盘，/100 区间判定（与源一致） */

/* ==================== 主菜单函数 ==================== */

/* 一路按键 ADC 三次平均（对应源 adc_mean_filter_convert(ch, 3)） */
static uint16_t Menu_ReadKeyAverage(bsp_adc_channel_t ch);

/**
 * @brief 对指定按键 ADC 内存读三次求整数均值。
 * @param[in] ch BSP_ADC_KEY1 或 BSP_ADC_KEY2。
 * @return ADC 0..4095 的三读均值。
 * @note 停车前台调用，无额外延时或新转换要求。
 */
static uint16_t Menu_ReadKeyAverage(bsp_adc_channel_t ch)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 3; i++)
    {
        sum += BSP_ADC_Read(ch);
    }
    return (uint16_t)(sum / 3);
}

/* 返回 MENU_KEY_*；0 表示无按键，第二路判定保持原覆盖语义。 */
uint8_t Menu_ScanKey(void)
{
    uint8_t key_status = 0;

    if (!s_filt_inited) /* 按键低通滤波器（源 key_init，α=0.7） */
    {
        Filter_InitLowpass(&s_filt_key1, 0.7f);
        Filter_InitLowpass(&s_filt_key2, 0.7f);
        s_filt_inited = 1;
    }

    g_menu_key_adc1 = Menu_ReadKeyAverage(BSP_ADC_KEY1); /* 方向键+确定 */
    g_menu_key_adc2 = Menu_ReadKeyAverage(BSP_ADC_KEY2); /* 复位+返回+调参 */

    if (g_menu_key_adc1 / 100 > 3 && g_menu_key_adc1 / 100 < 9)
    {
        key_status = MENU_KEY_UP; /* 约542 */
    }
    else if (g_menu_key_adc1 / 100 > 9 && g_menu_key_adc1 / 100 < 15)
    {
        key_status = MENU_KEY_DOWN; /* 约1262 */
    }
    else if (g_menu_key_adc1 / 100 > 15 && g_menu_key_adc1 / 100 < 20)
    {
        key_status = MENU_KEY_OK; /* 约1840 */
    }
    else if (g_menu_key_adc1 / 100 > 20 && g_menu_key_adc1 / 100 < 25)
    {
        key_status = MENU_KEY_LEFT; /* 约2467 */
    }
    else if (g_menu_key_adc1 / 100 > 25 && g_menu_key_adc1 / 100 < 31)
    {
        key_status = MENU_KEY_RIGHT; /* 约3082 */
    }

    if (g_menu_key_adc2 / 100 > 3 && g_menu_key_adc2 / 100 < 9)
    {
        key_status = MENU_KEY_RST; /* 约542 */
    }
    else if (g_menu_key_adc2 / 100 > 9 && g_menu_key_adc2 / 100 < 15)
    {
        key_status = MENU_KEY_ADJUST1; /* 约1262 */
    }
    else if (g_menu_key_adc2 / 100 > 15 && g_menu_key_adc2 / 100 < 20)
    {
        key_status = MENU_KEY_ADJUST2; /* 约1840 */
    }
    else if (g_menu_key_adc2 / 100 > 20 && g_menu_key_adc2 / 100 < 25)
    {
        key_status = MENU_KEY_FOUR; /* 约2467（预留） */
    }
    else if (g_menu_key_adc2 / 100 > 25 && g_menu_key_adc2 / 100 < 31)
    {
        key_status = MENU_KEY_BACK; /* 约3082 */
    }

    g_menu_key_adc1 = (uint16_t)Filter_UpdateLowpass(&s_filt_key1, (float)g_menu_key_adc1);
    g_menu_key_adc2 = (uint16_t)Filter_UpdateLowpass(&s_filt_key2, (float)g_menu_key_adc2);
    return key_status;
}

/* ==================== 按键动作处理 ==================== */

/**
 * @brief 执行原菜单导航与调参动作。
 * @param[in] key MENU_KEY_* 键值，未知值不操作。
 * @note 现有外部链接入口无头文件导出；仅停车菜单调用，保留当前模式/档位和 bank 来源。
 */
void Menu_HandleKey(uint8_t key)
{
    uint8_t current_mode = Tracking_GetState()->mode;
    uint8_t gear = Tracking_GetState()->gear;
    param_direction_t direction = *Param_GetDirection(current_mode, gear);
    switch (key)
    {
        case MENU_KEY_UP:
            if (g_menu_page == MENU_PAGE_ADJUST1 || g_menu_page == MENU_PAGE_ADJUST2)
            {
                g_menu_arrow = (g_menu_arrow == 1) ? 5 : g_menu_arrow - 1; /* 调参页 5 项 */
            }
            else if (g_menu_page == MENU_PAGE_HOME)
            {
                g_menu_arrow = (g_menu_arrow == 1) ? 3 : g_menu_arrow - 1; /* 主页 3 项 */
            }
            else
            {
                g_menu_arrow = (g_menu_arrow == 1) ? 4 : g_menu_arrow - 1;
            }
            break;

        case MENU_KEY_DOWN:
            if (g_menu_page == MENU_PAGE_ADJUST1 || g_menu_page == MENU_PAGE_ADJUST2)
            {
                g_menu_arrow = (g_menu_arrow == 5) ? 1 : g_menu_arrow + 1;
            }
            else if (g_menu_page == MENU_PAGE_HOME)
            {
                g_menu_arrow = (g_menu_arrow == 3) ? 1 : g_menu_arrow + 1;
            }
            else
            {
                g_menu_arrow = (g_menu_arrow == 4) ? 1 : g_menu_arrow + 1;
            }
            break;

        case MENU_KEY_OK:
            if (g_menu_page == MENU_PAGE_HOME)
            {
                g_menu_page = 20 + g_menu_arrow; /* 主页 → ADC_ERR/SPD_DIS/GYRO */
                g_menu_arrow = 1;
            }
            else if ((g_menu_page == MENU_PAGE_ADJUST1 || g_menu_page == MENU_PAGE_ADJUST2)
                && g_menu_arrow == 5)
            {
                s_fan_edit_idle = !s_fan_edit_idle;
            }
            break;

        case MENU_KEY_BACK:
            if (g_menu_page == MENU_PAGE_ADJUST1 || g_menu_page == MENU_PAGE_ADJUST2)
            {
                Param_Save(); /* 退出调参页保存参数（对应源 write_speed_low/high） */
                g_menu_page = MENU_PAGE_HOME;
                g_menu_arrow = 1;
                s_mode = 1;
            }
            else if (g_menu_page == MENU_PAGE_ADC_ERR || g_menu_page == MENU_PAGE_SPD_DIS
                || g_menu_page == MENU_PAGE_GYRO)
            {
                g_menu_page = MENU_PAGE_HOME;
                g_menu_arrow = 1;
            }
            break;

        case MENU_KEY_LEFT:
            if ((g_menu_page == MENU_PAGE_ADJUST1 || g_menu_page == MENU_PAGE_ADJUST2)
                && s_mode == 2)
            {
                if (g_menu_arrow == 1)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t){direction.kp - 0.001f, direction.k2p, direction.kd,
                            direction.base_speed});
                }
                if (g_menu_arrow == 2)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t){direction.kp, direction.k2p - 0.001f, direction.kd,
                            direction.base_speed});
                }
                if (g_menu_arrow == 3)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t){direction.kp, direction.k2p, direction.kd - 0.001f,
                            direction.base_speed});
                }
                if (g_menu_arrow == 4)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t){direction.kp, direction.k2p, direction.kd,
                            (int16_t)(direction.base_speed - 100)});
                }
                if (g_menu_arrow == 5)
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
            if ((g_menu_page == MENU_PAGE_ADJUST1 || g_menu_page == MENU_PAGE_ADJUST2)
                && s_mode == 2)
            {
                if (g_menu_arrow == 1)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t){direction.kp + 0.001f, direction.k2p, direction.kd,
                            direction.base_speed});
                }
                if (g_menu_arrow == 2)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t){direction.kp, direction.k2p + 0.001f, direction.kd,
                            direction.base_speed});
                }
                if (g_menu_arrow == 3)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t){direction.kp, direction.k2p, direction.kd + 0.001f,
                            direction.base_speed});
                }
                if (g_menu_arrow == 4)
                {
                    Param_SetDirection(current_mode, gear,
                        &(param_direction_t){direction.kp, direction.k2p, direction.kd,
                            (int16_t)(direction.base_speed + 100)});
                }
                if (g_menu_arrow == 5)
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
            g_menu_page = MENU_PAGE_ADJUST1;
            Param_SelectGear(PARAM_GEAR_LOW); /* 对应源 read_speed_low() */
            s_fan_edit_idle = 0;
            s_mode = 2;
            break;

        case MENU_KEY_ADJUST2:
            g_menu_page = MENU_PAGE_ADJUST2;
            Param_SelectGear(PARAM_GEAR_HIGH); /* 对应源 read_speed_high() */
            s_fan_edit_idle = 0;
            s_mode = 2;
            break;

        default:
            break;
    }
}

/* ==================== 显示绘制函数 ==================== */

/**
 * @brief 按当前页面绘制内容。
 * @note 仅停车前台调用，可阻塞；不改坐标、文字、显示单位或参数路由。
 */
static void Menu_DrawContent(void)
{
    switch (g_menu_page)
    {
        case MENU_PAGE_HOME:
            BSP_LCD_ShowString(
                MENU_LCD_COL_LABEL, MENU_LCD_ROW(0), "ADC_ERR", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(
                MENU_LCD_COL_LABEL, MENU_LCD_ROW(1), "SPD_DIS", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(
                MENU_LCD_COL_LABEL, MENU_LCD_ROW(2), "GYRO", MENU_TXT_FG, MENU_TXT_BG);
            /* 底部两栏辅助信息：按键ADC（校准用）与电池电压 */
            BSP_LCD_ShowString(
                MENU_LCD_COL_LABEL, MENU_LCD_ROW(3), "K1:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(24, MENU_LCD_ROW(3), g_menu_key_adc1, 4, MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(80, MENU_LCD_ROW(3), "K2:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(104, MENU_LCD_ROW(3), g_menu_key_adc2, 4, MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(
                MENU_LCD_COL_LABEL, MENU_LCD_ROW(4), "BAT:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(
                32, MENU_LCD_ROW(4), g_app_battery_filtered, 4, MENU_TXT_FG, MENU_TXT_BG);
            break;

        case MENU_PAGE_ADC_ERR:
            BSP_LCD_ShowString(0, MENU_LCD_ROW(0), "A1:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(
                24, MENU_LCD_ROW(0), Track_GetState()->filtered[0], 4, MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(80, MENU_LCD_ROW(0), "A2:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(
                104, MENU_LCD_ROW(0), Track_GetState()->filtered[1], 4, MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(0, MENU_LCD_ROW(1), "A3:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(
                24, MENU_LCD_ROW(1), Track_GetState()->filtered[2], 4, MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(80, MENU_LCD_ROW(1), "A4:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(
                104, MENU_LCD_ROW(1), Track_GetState()->filtered[3], 4, MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(0, MENU_LCD_ROW(2), "ERR:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(
                32, MENU_LCD_ROW(2), Track_GetState()->error, 6, MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(0, MENU_LCD_ROW(3), "SX:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(
                24, MENU_LCD_ROW(3), Track_GetState()->symmetry_x, 4, MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(80, MENU_LCD_ROW(3), "SY:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(
                104, MENU_LCD_ROW(3), Track_GetState()->symmetry_y, 4, MENU_TXT_FG, MENU_TXT_BG);
            break;

        case MENU_PAGE_SPD_DIS:
            BSP_LCD_ShowString(
                MENU_LCD_COL_LABEL, MENU_LCD_ROW(0), "SpL:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(40, MENU_LCD_ROW(0), Motor_GetState()->left.real_speed, 6, MENU_TXT_FG,
                MENU_TXT_BG);
            BSP_LCD_ShowString(
                MENU_LCD_COL_LABEL, MENU_LCD_ROW(1), "SpR:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(40, MENU_LCD_ROW(1), Motor_GetState()->right.real_speed, 6, MENU_TXT_FG,
                MENU_TXT_BG);
            BSP_LCD_ShowString(
                MENU_LCD_COL_LABEL, MENU_LCD_ROW(2), "Dis:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(
                40, MENU_LCD_ROW(2), Motor_GetState()->distance, 8, MENU_TXT_FG, MENU_TXT_BG);
            break;

        case MENU_PAGE_GYRO:
            BSP_LCD_ShowString(
                MENU_LCD_COL_LABEL, MENU_LCD_ROW(0), "GX:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowFloat(
                40, MENU_LCD_ROW(0), IMU_GetState()->gyro[0], 4, 1, MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(
                MENU_LCD_COL_LABEL, MENU_LCD_ROW(1), "GY:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowFloat(
                40, MENU_LCD_ROW(1), IMU_GetState()->gyro[1], 4, 1, MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(
                MENU_LCD_COL_LABEL, MENU_LCD_ROW(2), "GZ:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowFloat(
                40, MENU_LCD_ROW(2), IMU_GetState()->gyro[2], 4, 1, MENU_TXT_FG, MENU_TXT_BG);
            break;

        case MENU_PAGE_ADJUST1:
        case MENU_PAGE_ADJUST2:
            BSP_LCD_ShowString(152, MENU_LCD_ROW(0), (g_menu_page == MENU_PAGE_ADJUST1) ? "1" : "2",
                MENU_CUR_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(MENU_LCD_COL_LABEL, MENU_LCD_ROW(0),
                Tracking_GetState()->mode == TRACKING_MODE_OPTICAL
                    ? (Tracking_GetState()->gear ? "O2 KP" : "O1 KP")
                    : (Tracking_GetState()->gear ? "E2 KP" : "E1 KP"),
                MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowFloat(MENU_LCD_COL_VAL, MENU_LCD_ROW(0), PID_GetState()->kp_x, 2, 3,
                MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(
                MENU_LCD_COL_LABEL, MENU_LCD_ROW(1), "K2P_x", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowFloat(MENU_LCD_COL_VAL, MENU_LCD_ROW(1), PID_GetState()->k2p_x, 2, 3,
                MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(
                MENU_LCD_COL_LABEL, MENU_LCD_ROW(2), "KD_x", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowFloat(MENU_LCD_COL_VAL, MENU_LCD_ROW(2), PID_GetState()->kd_x, 2, 3,
                MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(
                MENU_LCD_COL_LABEL, MENU_LCD_ROW(3), "SPD", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(MENU_LCD_COL_VAL, MENU_LCD_ROW(3), Motor_GetState()->base_speed, 5,
                MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(MENU_LCD_COL_LABEL, MENU_LCD_ROW(4),
                s_fan_edit_idle ? "FAN-I" : "FAN-R", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(MENU_LCD_COL_VAL, MENU_LCD_ROW(4),
                s_fan_edit_idle ? Motor_GetState()->fan_duty_idle : Motor_GetState()->fan_duty, 5,
                MENU_TXT_FG, MENU_TXT_BG);
            break;

        default:
            break;
    }
}

/* 绘制光标指示器（仅位置变化时更新） */
/**
 * @brief 位置或页面变化时擦除并重绘光标。
 * @note 仅停车前台调用，可阻塞；保持前次位置哨兵 0xFF/0xFFFF。
 */
static void Menu_DrawCursor(void)
{
    if (g_menu_arrow != s_last_displayed_arrow || g_menu_page != s_last_displayed_page)
    {
        if (s_last_displayed_arrow >= 1 && s_last_displayed_arrow <= 5)
        {
            BSP_LCD_ShowString(MENU_LCD_COL_CURSOR, MENU_LCD_ROW(s_last_displayed_arrow - 1), "  ",
                MENU_TXT_FG, MENU_TXT_BG);
        }
        BSP_LCD_ShowString(
            MENU_LCD_COL_CURSOR, MENU_LCD_ROW(g_menu_arrow - 1), "<<", MENU_CUR_FG, MENU_TXT_BG);
        s_last_displayed_arrow = g_menu_arrow;
    }
}

/* 主菜单显示与处理：主循环周期调用，完成按键响应和屏幕刷新（仅停车时运行） */
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
        BSP_LCD_Clear(BSP_LCD_WHITE);
        s_last_displayed_page = 0xFFFF;
        s_last_displayed_arrow = 0xFF;
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
    if (g_menu_page != s_last_displayed_page)
    {
        BSP_LCD_Clear(BSP_LCD_WHITE);
        s_last_displayed_page = g_menu_page;
        s_last_displayed_arrow = 0xFF;
    }

    /* 参数修改后、按键松开时写一次 Flash
     *（源工程 MENU_KEY_ADJUST1 页每帧写 Flash，此处改为空闲时单次写入，减少擦除损耗） */
    if (s_param_dirty && key == 0)
    {
        Param_Save();
        s_param_dirty = 0;
    }

    Menu_DrawContent();
    Menu_DrawCursor();
}
