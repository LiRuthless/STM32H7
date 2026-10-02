// ============================================================
// 文件名: dashboard.c
// 功能说明: 板载屏（ST7735 160x80 横屏）单页仪表盘
// 停车状态主循环周期调用，实时显示各传感器读数，无需按键。
// 与 menu.c 的外接屏完整菜单互斥（LCD_TARGET_EXTERNAL 编译期选择）。
// ============================================================

#include "bsp.h"
#include "app_config.h"
#include "dashboard.h"
#include "app.h"
#include "track_sensor.h"
#include "imu_proc.h"
#include "control.h"
#include "motor.h"

/* 私有定义 -----------------------------------------------------------*/
#define DASH_REFRESH_MS     100u    /* 数值刷新周期 */

/* 私有变量 ---------------------------------------------------------*/
static uint32_t s_last_refresh;

/* 导出函数定义 -------------------------------------------*/

// 函数名: Dashboard_Init
// 功能: 清屏并绘制固定标签（布局：5 行，8x16 字库，160 列向 20 字符）
void Dashboard_Init(void)
{
    BSP_LCD_Clear(LCD_WHITE);
    BSP_LCD_ShowString(0,   0, "L:", LCD_BLACK, LCD_WHITE);     // 行0：四路电感
    BSP_LCD_ShowString(0,  16, "E:", LCD_BLACK, LCD_WHITE);     // 行1：循迹偏差
    BSP_LCD_ShowString(72, 16, "T:", LCD_BLACK, LCD_WHITE);     //      方向环输出
    BSP_LCD_ShowString(0,  32, "B:", LCD_BLACK, LCD_WHITE);     // 行2：电池电压
    BSP_LCD_ShowString(72, 32, "DL:", LCD_BLACK, LCD_WHITE);    //      激光距离
    BSP_LCD_ShowString(0,  48, "G:", LCD_BLACK, LCD_WHITE);     // 行3：陀螺仪 xyz
    BSP_LCD_ShowString(0,  64, "S:", LCD_BLACK, LCD_WHITE);     // 行4：左右轮速
    BSP_LCD_ShowString(72, 64, "D:", LCD_BLACK, LCD_WHITE);     //      里程
    BSP_LCD_ShowString(128, 64, "K:", LCD_BLACK, LCD_WHITE);    //      主状态机
    s_last_refresh = 0;
}

// 函数名: Dashboard_Update
// 功能: 刷新数值区（按 100ms 节流，主循环周期调用）
void Dashboard_Update(void)
{
    uint32_t now = HAL_GetTick();

    if((uint32_t)(now - s_last_refresh) < DASH_REFRESH_MS)
    {
        return;
    }
    s_last_refresh = now;

    /* 行0：四路电感（横左/竖左/竖右/横右） */
    BSP_LCD_ShowInt(16,  0, Track_GetState()->filtered[0], 4, LCD_BLUE, LCD_WHITE);
    BSP_LCD_ShowInt(56,  0, Track_GetState()->filtered[1], 4, LCD_BLUE, LCD_WHITE);
    BSP_LCD_ShowInt(96,  0, Track_GetState()->filtered[2], 4, LCD_BLUE, LCD_WHITE);
    BSP_LCD_ShowInt(136, 0, Track_GetState()->filtered[3], 4, LCD_BLUE, LCD_WHITE);

    /* 行1：循迹偏差 / 方向环输出 */
    BSP_LCD_ShowInt(16, 16, Track_GetState()->error, 4, LCD_RED, LCD_WHITE);
    BSP_LCD_ShowInt(88, 16, track_out,  4, LCD_RED, LCD_WHITE);

    /* 行2：电池（12bit 值）/ 激光距离（mm，8192=无效） */
    BSP_LCD_ShowInt(16, 32, battery_filt,     4, LCD_BLACK, LCD_WHITE);
    BSP_LCD_ShowInt(96, 32, dl1b_distance_mm, 4, LCD_BLACK, LCD_WHITE);

    /* 行3：陀螺仪 xyz（°/s 取整） */
    BSP_LCD_ShowInt(16,  48, (int32_t)IMU_GetState()->gyro[0], 5, LCD_MAGENTA, LCD_WHITE);
    BSP_LCD_ShowInt(64,  48, (int32_t)IMU_GetState()->gyro[1], 5, LCD_MAGENTA, LCD_WHITE);
    BSP_LCD_ShowInt(112, 48, (int32_t)IMU_GetState()->gyro[2], 5, LCD_MAGENTA, LCD_WHITE);

    /* 行4：左右轮速 / 里程 / 主状态机 */
    BSP_LCD_ShowInt(16,  64, Motor_GetState()->left.real_speed, 4, LCD_BLACK, LCD_WHITE);
    BSP_LCD_ShowInt(48,  64, Motor_GetState()->right.real_speed, 4, LCD_BLACK, LCD_WHITE);
    BSP_LCD_ShowInt(88,  64, Motor_GetState()->distance,     5, LCD_BLACK, LCD_WHITE);
    BSP_LCD_ShowInt(144, 64, kernel_state, 1, LCD_BLACK, LCD_WHITE);
}
