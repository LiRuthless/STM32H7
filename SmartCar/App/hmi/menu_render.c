/**
 * @file menu_render.c
 * @brief 停车菜单绘制与刷新缓存。
 * @details 只读菜单和业务状态，保持显示调用顺序；仅拥有上次页面和光标，不编辑参数。
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
/* 小屏布局常量（像素） */
#define MENU_LCD_COL_LABEL 0    /* 标签列 */
#define MENU_LCD_COL_VAL 72     /* 数值列 */
#define MENU_LCD_COL_CURSOR 144 /* 光标 "<<" 列 */
#define MENU_LCD_ROW(n) (16 * (n))

#define MENU_TXT_FG BSP_LCD_BLACK
#define MENU_TXT_BG BSP_LCD_WHITE
#define MENU_CUR_FG BSP_LCD_RED

/* 仅绘制前台维护；复位清屏后强制下一轮重画。 */
static uint16_t s_last_displayed_page = 0xFFFF;
static uint8_t s_last_displayed_arrow = 0xFF;

/**
 * @brief 按当前页面绘制内容。
 * @details 按当前页面的固定坐标、颜色和数值位宽输出LCD调用；方向、速度和风扇显示活动值，
 *     ADC显示取按键owner滤波值。仅绘制不改bank、页面、编辑状态或按键沿。
 * @note 仅停车前台调用，可阻塞；不改坐标、文字、显示单位或参数路由。
 */
static void Menu_DrawContent(void)
{
    switch (Menu_GetPage())
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
            BSP_LCD_ShowInt(24, MENU_LCD_ROW(3), MenuKeys_GetAdc1(), 4, MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(80, MENU_LCD_ROW(3), "K2:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(104, MENU_LCD_ROW(3), MenuKeys_GetAdc2(), 4, MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowString(
                MENU_LCD_COL_LABEL, MENU_LCD_ROW(4), "BAT:", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(
                32, MENU_LCD_ROW(4), Battery_GetFiltered(), 4, MENU_TXT_FG, MENU_TXT_BG);
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
            BSP_LCD_ShowString(152, MENU_LCD_ROW(0),
                (Menu_GetPage() == MENU_PAGE_ADJUST1) ? "1" : "2",
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
                Menu_GetFanEditIdle() ? "FAN-I" : "FAN-R", MENU_TXT_FG, MENU_TXT_BG);
            BSP_LCD_ShowInt(MENU_LCD_COL_VAL, MENU_LCD_ROW(4),
                Menu_GetFanEditIdle() ? Motor_GetState()->fan_duty_idle
                    : Motor_GetState()->fan_duty, 5,
                MENU_TXT_FG, MENU_TXT_BG);
            break;

        default:
            break;
    }
}

/**
 * @brief 位置或页面变化时擦除并重绘光标。
 * @details 仅页面或位置变化时执行；按原页面行数用空格擦旧光标，再画新光标并更新arrow缓存，
 *     不修复非法位置，不改变页面导航。
 * @note 仅停车前台调用，可阻塞；保持前次位置哨兵 0xFF/0xFFFF。
 */
static void Menu_DrawCursor(void)
{
    /* 逐行清除旧光标区域，再仅在当前行绘制光标；行号超出可见范围时只执行清除。 */
    if (Menu_GetArrow() != s_last_displayed_arrow || Menu_GetPage() != s_last_displayed_page)
    {
        if (s_last_displayed_arrow >= 1 && s_last_displayed_arrow <= 5)
        {
            BSP_LCD_ShowString(MENU_LCD_COL_CURSOR, MENU_LCD_ROW(s_last_displayed_arrow - 1), "  ",
                MENU_TXT_FG, MENU_TXT_BG);
        }
        BSP_LCD_ShowString(
            MENU_LCD_COL_CURSOR, MENU_LCD_ROW(Menu_GetArrow() - 1), "<<", MENU_CUR_FG, MENU_TXT_BG);
        s_last_displayed_arrow = Menu_GetArrow();
    }
}

/**
 * @brief 清屏并使下次页面绘制失效。
 * @details RST按键沿调用后立即返回；下一轮仍需要按页面哨兵再次清屏。
 */
void MenuRender_Reset(void)
{
    BSP_LCD_Clear(BSP_LCD_WHITE);
    s_last_displayed_page = 0xFFFF;
    s_last_displayed_arrow = 0xFF;
}

/**
 * @brief 页面改变时清屏并复位光标缓存。
 * @details 清屏先于菜单松键保存及内容绘制；不修改导航状态。
 */
void MenuRender_PreparePage(void)
{
    if (Menu_GetPage() != s_last_displayed_page)
    {
        BSP_LCD_Clear(BSP_LCD_WHITE);
        s_last_displayed_page = Menu_GetPage();
        s_last_displayed_arrow = 0xFF;
    }
}

/**
 * @brief 依次绘制页面内容与光标。
 * @details 内容每轮刷新，光标只在位置或页面变化时刷新。
 */
void MenuRender_Draw(void)
{
    Menu_DrawContent();
    Menu_DrawCursor();
}
