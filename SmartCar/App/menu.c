// ============================================================
// 文件名: menu.c
// 功能说明: LCD 菜单与按键处理模块（移植自源 menu.c）
// 实现 ADC 分压按键扫描、页面导航、参数调参及屏幕刷新。
// 显示由 ips114_show_* 换为 BSP_LCD_*，布局适配 160×80 小屏
// （8x16 字库：20 列 × 5 行，行 y = 0/16/32/48/64）。
// 屏幕只在停车菜单状态刷新（运行中 main 循环不调用 menu()，与源一致）。
//
// ==================== 操作方法 ====================
//  UP/DOWN    : 上下移动光标
//  OK         : 主页进入子页面（ADC_ERR / SPD_DIS / GYRO）
//  BACK       : 返回主页（调参页退出时保存参数到 Flash）
//  LEFT/RIGHT : 调参页增减参数（PID 步进 ±0.001，速度/风扇 ±100）
//  RST        : 清屏刷新
//  ADJUST1/2  : 一键进入慢速/快速调参页（加载对应档位参数）
// ============================================================

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
#define LCD_COL_LABEL   0       /* 标签列 */
#define LCD_COL_VAL     72      /* 数值列 */
#define LCD_COL_CURSOR  144     /* 光标 "<<" 列 */
#define LCD_ROW(n)      (16 * (n))

#define TXT_FG          LCD_BLACK
#define TXT_BG          LCD_WHITE
#define CUR_FG          LCD_RED

// ==================== 菜单状态变量 ====================
uint8_t  page  = PAGE_HOME;     // 当前页面编号
uint8_t  arrow = 1;             // 当前光标位置（从1开始计数）
static uint8_t  mode = 1;       // 菜单模式（1=浏览，2=调参）
uint16_t key_adc1 = 0;          // 按键ADC通道1采样值
uint16_t key_adc2 = 0;          // 按键ADC通道2采样值

static uint8_t param_dirty = 0; // 参数被修改待保存

/* 显示状态（减少不必要刷新与闪烁） */
static uint16_t last_displayed_page  = 0xFFFF;
static uint8_t  last_displayed_arrow = 0xFF;

static LowPassFilter filt_key1;
static LowPassFilter filt_key2;
static uint8_t filt_inited = 0;

// ==================== 静态函数声明 ====================
static void menu_draw_content(void);
static void menu_draw_cursor(void);

// 一路按键 ADC 三次平均（对应源 adc_mean_filter_convert(ch, 3)）
static uint16_t key_read_avg(bsp_adc_channel_t ch)
{
    uint32_t sum = 0;
    for(uint8_t i = 0; i < 3; i++)
    {
        sum += BSP_ADC_Read(ch);
    }
    return (uint16_t)(sum / 3);
}

// ==================== 按键扫描 ====================

// ADC按键扫描：两路分压键盘，/100 区间判定（与源一致）
// 返回值: UP/DOWN/OK/LEFT/RIGHT/RST/ADJUST1/ADJUST2/FOUR/BACK 或 0（无按键）
uint8_t key_scan(void)
{
    uint8_t key_status = 0;

    if(!filt_inited)        // 按键低通滤波器（源 key_init，α=0.7）
    {
        lowpass_init(&filt_key1, 0.7f);
        lowpass_init(&filt_key2, 0.7f);
        filt_inited = 1;
    }

    key_adc1 = key_read_avg(BSP_ADC_KEY1);      // 方向键+确定
    key_adc2 = key_read_avg(BSP_ADC_KEY2);      // 复位+返回+调参

    if(key_adc1 / 100 > 3 && key_adc1 / 100 < 9)
    {
        key_status = UP;        // 约542
    }
    else if(key_adc1 / 100 > 9 && key_adc1 / 100 < 15)
    {
        key_status = DOWN;      // 约1262
    }
    else if(key_adc1 / 100 > 15 && key_adc1 / 100 < 20)
    {
        key_status = OK;        // 约1840
    }
    else if(key_adc1 / 100 > 20 && key_adc1 / 100 < 25)
    {
        key_status = LEFT;      // 约2467
    }
    else if(key_adc1 / 100 > 25 && key_adc1 / 100 < 31)
    {
        key_status = RIGHT;     // 约3082
    }

    if(key_adc2 / 100 > 3 && key_adc2 / 100 < 9)
    {
        key_status = RST;       // 约542
    }
    else if(key_adc2 / 100 > 9 && key_adc2 / 100 < 15)
    {
        key_status = ADJUST1;   // 约1262
    }
    else if(key_adc2 / 100 > 15 && key_adc2 / 100 < 20)
    {
        key_status = ADJUST2;   // 约1840
    }
    else if(key_adc2 / 100 > 20 && key_adc2 / 100 < 25)
    {
        key_status = FOUR;      // 约2467（预留）
    }
    else if(key_adc2 / 100 > 25 && key_adc2 / 100 < 31)
    {
        key_status = BACK;      // 约3082
    }

    key_adc1 = (uint16_t)lowpass_update(&filt_key1, (float)key_adc1);
    key_adc2 = (uint16_t)lowpass_update(&filt_key2, (float)key_adc2);
    return key_status;
}

// ==================== 按键动作处理 ====================

void key_action(uint8_t key)
{
    switch(key)
    {
    case UP:
        if(page == PAGE_ADJUST1 || page == PAGE_ADJUST2)
            arrow = (arrow == 1) ? 5 : arrow - 1;       // 调参页 5 项
        else if(page == PAGE_HOME)
            arrow = (arrow == 1) ? 3 : arrow - 1;       // 主页 3 项
        else
            arrow = (arrow == 1) ? 4 : arrow - 1;
        break;

    case DOWN:
        if(page == PAGE_ADJUST1 || page == PAGE_ADJUST2)
            arrow = (arrow == 5) ? 1 : arrow + 1;
        else if(page == PAGE_HOME)
            arrow = (arrow == 3) ? 1 : arrow + 1;
        else
            arrow = (arrow == 4) ? 1 : arrow + 1;
        break;

    case OK:
        if(page == PAGE_HOME)
        {
            page  = 20 + arrow;     // 主页 → ADC_ERR/SPD_DIS/GYRO
            arrow = 1;
        }
        break;

    case BACK:
        if(page == PAGE_ADJUST1 || page == PAGE_ADJUST2)
        {
            Param_Save();           // 退出调参页保存参数（对应源 write_speed_low/high）
            page  = PAGE_HOME;
            arrow = 1;
            mode  = 1;
        }
        else if(page == PAGE_ADC_ERR || page == PAGE_SPD_DIS || page == PAGE_GYRO)
        {
            page  = PAGE_HOME;
            arrow = 1;
        }
        break;

    case LEFT:
        if((page == PAGE_ADJUST1 || page == PAGE_ADJUST2) && mode == 2)
        {
            if(arrow == 1) KP_x       -= 0.001f;
            if(arrow == 2) K2P_x      -= 0.001f;
            if(arrow == 3) KD_x       -= 0.001f;
            if(arrow == 4) base_speed -= 100;
            if(arrow == 5) fan_duty   -= 100;
            param_dirty = 1;
        }
        break;

    case RIGHT:
        if((page == PAGE_ADJUST1 || page == PAGE_ADJUST2) && mode == 2)
        {
            if(arrow == 1) KP_x       += 0.001f;
            if(arrow == 2) K2P_x      += 0.001f;
            if(arrow == 3) KD_x       += 0.001f;
            if(arrow == 4) base_speed += 100;
            if(arrow == 5) fan_duty   += 100;
            param_dirty = 1;
        }
        break;

    case ADJUST1:
        page = PAGE_ADJUST1;
        Param_SelectGear(PARAM_GEAR_LOW);   // 对应源 read_speed_low()
        mode = 2;
        break;

    case ADJUST2:
        page = PAGE_ADJUST2;
        Param_SelectGear(PARAM_GEAR_HIGH);  // 对应源 read_speed_high()
        mode = 2;
        break;

    default:
        break;
    }
}

// ==================== 显示绘制函数 ====================

static void menu_draw_content(void)
{
    switch(page)
    {
    case PAGE_HOME:
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(0), "ADC_ERR", TXT_FG, TXT_BG);
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(1), "SPD_DIS", TXT_FG, TXT_BG);
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(2), "GYRO",    TXT_FG, TXT_BG);
        /* 底部两栏辅助信息：按键ADC（校准用）与电池电压 */
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(3), "K1:", TXT_FG, TXT_BG);
        BSP_LCD_ShowInt(24, LCD_ROW(3), key_adc1, 4, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(80, LCD_ROW(3), "K2:", TXT_FG, TXT_BG);
        BSP_LCD_ShowInt(104, LCD_ROW(3), key_adc2, 4, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(4), "BAT:", TXT_FG, TXT_BG);
        BSP_LCD_ShowInt(32, LCD_ROW(4), battery_filt, 4, TXT_FG, TXT_BG);
        break;

    case PAGE_ADC_ERR:
        BSP_LCD_ShowString(0,  LCD_ROW(0), "A1:", TXT_FG, TXT_BG);
        BSP_LCD_ShowInt(24,    LCD_ROW(0), adc_filted[0], 4, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(80, LCD_ROW(0), "A2:", TXT_FG, TXT_BG);
        BSP_LCD_ShowInt(104,   LCD_ROW(0), adc_filted[1], 4, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(0,  LCD_ROW(1), "A3:", TXT_FG, TXT_BG);
        BSP_LCD_ShowInt(24,    LCD_ROW(1), adc_filted[2], 4, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(80, LCD_ROW(1), "A4:", TXT_FG, TXT_BG);
        BSP_LCD_ShowInt(104,   LCD_ROW(1), adc_filted[3], 4, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(0,  LCD_ROW(2), "ERR:", TXT_FG, TXT_BG);
        BSP_LCD_ShowInt(32,    LCD_ROW(2), track_error, 6, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(0,  LCD_ROW(3), "SX:", TXT_FG, TXT_BG);
        BSP_LCD_ShowInt(24,    LCD_ROW(3), symmetry_x, 4, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(80, LCD_ROW(3), "SY:", TXT_FG, TXT_BG);
        BSP_LCD_ShowInt(104,   LCD_ROW(3), symmetry_y, 4, TXT_FG, TXT_BG);
        break;

    case PAGE_SPD_DIS:
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(0), "SpL:", TXT_FG, TXT_BG);
        BSP_LCD_ShowInt(40, LCD_ROW(0), real_speed_L, 6, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(1), "SpR:", TXT_FG, TXT_BG);
        BSP_LCD_ShowInt(40, LCD_ROW(1), real_speed_R, 6, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(2), "Dis:", TXT_FG, TXT_BG);
        BSP_LCD_ShowInt(40, LCD_ROW(2), Distance, 8, TXT_FG, TXT_BG);
        break;

    case PAGE_GYRO:
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(0), "GX:", TXT_FG, TXT_BG);
        BSP_LCD_ShowFloat(40, LCD_ROW(0), gyro_x, 4, 1, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(1), "GY:", TXT_FG, TXT_BG);
        BSP_LCD_ShowFloat(40, LCD_ROW(1), gyro_y, 4, 1, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(2), "GZ:", TXT_FG, TXT_BG);
        BSP_LCD_ShowFloat(40, LCD_ROW(2), gyro_z, 4, 1, TXT_FG, TXT_BG);
        break;

    case PAGE_ADJUST1:
    case PAGE_ADJUST2:
        BSP_LCD_ShowString(152, LCD_ROW(0), (page == PAGE_ADJUST1) ? "1" : "2", CUR_FG, TXT_BG);
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(0), "KP_x",  TXT_FG, TXT_BG);
        BSP_LCD_ShowFloat(LCD_COL_VAL, LCD_ROW(0), KP_x,  2, 3, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(1), "K2P_x", TXT_FG, TXT_BG);
        BSP_LCD_ShowFloat(LCD_COL_VAL, LCD_ROW(1), K2P_x, 2, 3, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(2), "KD_x",  TXT_FG, TXT_BG);
        BSP_LCD_ShowFloat(LCD_COL_VAL, LCD_ROW(2), KD_x,  2, 3, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(3), "SPD",   TXT_FG, TXT_BG);
        BSP_LCD_ShowInt(LCD_COL_VAL, LCD_ROW(3), base_speed, 5, TXT_FG, TXT_BG);
        BSP_LCD_ShowString(LCD_COL_LABEL, LCD_ROW(4), "FAN",   TXT_FG, TXT_BG);
        BSP_LCD_ShowInt(LCD_COL_VAL, LCD_ROW(4), fan_duty, 5, TXT_FG, TXT_BG);
        break;

    default:
        break;
    }
}

// 绘制光标指示器（仅位置变化时更新）
static void menu_draw_cursor(void)
{
    if(arrow != last_displayed_arrow || page != last_displayed_page)
    {
        if(last_displayed_arrow >= 1 && last_displayed_arrow <= 5)
        {
            BSP_LCD_ShowString(LCD_COL_CURSOR, LCD_ROW(last_displayed_arrow - 1), "  ", TXT_FG, TXT_BG);
        }
        BSP_LCD_ShowString(LCD_COL_CURSOR, LCD_ROW(arrow - 1), "<<", CUR_FG, TXT_BG);
        last_displayed_arrow = arrow;
    }
}

// ==================== 主菜单函数 ====================

// 主菜单显示与处理：主循环周期调用，完成按键响应和屏幕刷新（仅停车时运行）
void menu(void)
{
    static uint8_t last_key = 0;

    uint8_t key = key_scan();

    if(key == 0)                    // 无键清零（消抖沿检测）
    {
        last_key = 0;
    }

    /* RST键：清屏重绘 */
    if(key == RST && key != last_key)
    {
        BSP_LCD_Clear(LCD_WHITE);
        last_displayed_page  = 0xFFFF;
        last_displayed_arrow = 0xFF;
        last_key = key;
        return;
    }

    /* 按键按下沿检测 */
    if(key != last_key && key != 0)
    {
        key_action(key);
        last_key = key;
    }

    /* 页面切换时清屏，避免每帧闪烁 */
    if(page != last_displayed_page)
    {
        BSP_LCD_Clear(LCD_WHITE);
        last_displayed_page  = page;
        last_displayed_arrow = 0xFF;
    }

    /* 参数修改后、按键松开时写一次 Flash
     *（源工程 ADJUST1 页每帧写 Flash，此处改为空闲时单次写入，减少擦除损耗） */
    if(param_dirty && key == 0)
    {
        Param_Save();
        param_dirty = 0;
    }

    menu_draw_content();
    menu_draw_cursor();
}
