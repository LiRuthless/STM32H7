/**
 * @file dashboard.c
 * @brief App 板载单页显示；只在停车前台按 100 ms 刷新。
 * @details 读取传感器、控制和日志快照并绘制运行仪表；仅在停车前台刷新，不参与实时控制。
 */
#include "bsp.h"
#include "app_config.h"
#include "hmi/dashboard.h"
#include "app.h"
#include "sensing/track_sensor.h"
#include "sensing/imu_proc.h"
#include "control/control.h"
#include "control/motor.h"
#define DASHBOARD_REFRESH_MS 100u /* 数值刷新周期 */
/* 停车前台独占刷新节流时间，按 HAL 毫秒 tick 无符号差处理回绕。 */
static uint32_t s_last_refresh;
/**
 * @brief 清屏并绘制板载单页仪表盘标签。
 * @details 按现有调用顺序使用 BSP_LCD_Clear、BSP_LCD_ShowString 完成本函数处理。
 * @note BSP LCD 初始化后前台调用，可阻塞；运行链不调用，与外接菜单编译期互斥。
 */
void Dashboard_Init(void)
{
    BSP_LCD_Clear(BSP_LCD_WHITE);
    BSP_LCD_ShowString(0, 0, "L:", BSP_LCD_BLACK, BSP_LCD_WHITE);      /* 行0：四路电感 */
    BSP_LCD_ShowString(0, 16, "E:", BSP_LCD_BLACK, BSP_LCD_WHITE);     /* 行1：循迹偏差 */
    BSP_LCD_ShowString(72, 16, "T:", BSP_LCD_BLACK, BSP_LCD_WHITE);    /* 方向环输出 */
    BSP_LCD_ShowString(0, 32, "B:", BSP_LCD_BLACK, BSP_LCD_WHITE);     /* 行2：电池电压 */
    BSP_LCD_ShowString(72, 32, "DL:", BSP_LCD_BLACK, BSP_LCD_WHITE);   /* 激光距离 */
    BSP_LCD_ShowString(0, 48, "G:", BSP_LCD_BLACK, BSP_LCD_WHITE);     /* 行3：陀螺仪 xyz */
    BSP_LCD_ShowString(0, 64, "S:", BSP_LCD_BLACK, BSP_LCD_WHITE);     /* 行4：左右轮速 */
    BSP_LCD_ShowString(72, 64, "D:", BSP_LCD_BLACK, BSP_LCD_WHITE);    /* 里程 */
    BSP_LCD_ShowString(128, 64, "O1K0", BSP_LCD_BLACK, BSP_LCD_WHITE); /* 主状态机 */
    s_last_refresh = 0;
}

/**
 * @brief 按 100 ms 间隔刷新板载数值区。
 * @details 按现有调用顺序使用 HAL_GetTick、BSP_LCD_ShowInt、Track_GetState、IMU_GetState
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 仅停车前台调用，可阻塞；无符号减法处理 HAL tick 回绕，保持各状态读取位置。
 */
void Dashboard_Update(void)
{
    uint32_t now = HAL_GetTick();

    if ((uint32_t)(now - s_last_refresh) < DASHBOARD_REFRESH_MS)
    {
        return;
    }
    s_last_refresh = now;

    /* 行0：四路电感（横左/竖左/竖右/横右） */
    BSP_LCD_ShowInt(16, 0, Track_GetState()->filtered[0], 4, BSP_LCD_BLUE, BSP_LCD_WHITE);
    BSP_LCD_ShowInt(56, 0, Track_GetState()->filtered[1], 4, BSP_LCD_BLUE, BSP_LCD_WHITE);
    BSP_LCD_ShowInt(96, 0, Track_GetState()->filtered[2], 4, BSP_LCD_BLUE, BSP_LCD_WHITE);
    BSP_LCD_ShowInt(136, 0, Track_GetState()->filtered[3], 4, BSP_LCD_BLUE, BSP_LCD_WHITE);

    /* 行1：循迹偏差 / 方向环输出 */
    BSP_LCD_ShowInt(16, 16, Track_GetState()->error, 4, BSP_LCD_RED, BSP_LCD_WHITE);
    BSP_LCD_ShowInt(88, 16, g_control_track_output, 4, BSP_LCD_RED, BSP_LCD_WHITE);

    /* 行2：电池（12bit 值）/ 激光距离（mm，8192=无效） */
    BSP_LCD_ShowInt(16, 32, g_app_battery_filtered, 4, BSP_LCD_BLACK, BSP_LCD_WHITE);
    BSP_LCD_ShowInt(96, 32, g_app_dl1b_distance_mm, 4, BSP_LCD_BLACK, BSP_LCD_WHITE);

    /* 行3：陀螺仪 xyz（°/s 取整） */
    BSP_LCD_ShowInt(16, 48, (int32_t)IMU_GetState()->gyro[0], 5, BSP_LCD_MAGENTA, BSP_LCD_WHITE);
    BSP_LCD_ShowInt(64, 48, (int32_t)IMU_GetState()->gyro[1], 5, BSP_LCD_MAGENTA, BSP_LCD_WHITE);
    BSP_LCD_ShowInt(112, 48, (int32_t)IMU_GetState()->gyro[2], 5, BSP_LCD_MAGENTA, BSP_LCD_WHITE);

    /* 行4：左右轮速 / 里程 / 主状态机 */
    BSP_LCD_ShowInt(16, 64, Motor_GetState()->left.real_speed, 4, BSP_LCD_BLACK, BSP_LCD_WHITE);
    BSP_LCD_ShowInt(48, 64, Motor_GetState()->right.real_speed, 4, BSP_LCD_BLACK, BSP_LCD_WHITE);
    BSP_LCD_ShowInt(88, 64, Motor_GetState()->distance, 5, BSP_LCD_BLACK, BSP_LCD_WHITE);
    char status[5] =
    {
        (Tracking_GetState()->mode == TRACKING_MODE_OPTICAL) ? 'O' : 'E',
        (char)('1' + Tracking_GetState()->gear),
        'K',
        (char)('0' + g_control_kernel_state),
        0
    };
    BSP_LCD_ShowString(128, 64, status, BSP_LCD_BLACK, BSP_LCD_WHITE);
}
