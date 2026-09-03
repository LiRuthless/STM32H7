// ============================================================
// 文件名: app.c
// 功能说明: 应用总装（吸收源 user\main.c、user\isr.c、code\config.c 的职责）
// 系统初始化、主循环调度、TIM6 2ms 控制中断、TIM7 5ms 辅助中断。
// ============================================================

#include "bsp.h"
#include "app_config.h"
#include "app.h"
#include "param.h"
#include "menu.h"
#include "wireless.h"
#include "track_sensor.h"
#include "imu_proc.h"
#include "pid.h"
#include "control.h"
#include "motor.h"
#include "filter.h"

/* 状态灯：板载蓝色 LED（PE3），按低电平点亮处理；若实际相反改此处即可 */
#define LED_RUN_LEVEL       GPIO_PIN_RESET

// ==================== 全局变量（源 config.c） ====================
uint8_t  uart[32];              // 串口数据发送缓冲区
uint8_t  dat[32];               // 串口数据接收缓冲区

int16_t  battery = 0;           // 电池电压ADC原始值
int16_t  battery_filt = 0;      // 电池电压滤波值
int16_t  low_power_num = 0;     // 低压持续计数

int32_t  time = 0;              // 控制节拍计数（2ms）

uint8_t  key_flag = 0;          // 启动键已按下
uint8_t  Start_flag = 0;        // 启动标志位
uint8_t  Run_flag = 0;          // 运行标志位（赛道检测控制）

LowPassFilter filt_battery;     // 电池电压低通滤波器（α=0.65）

uint16_t dl1b_distance_mm = BSP_DL1B_INVALID;   // DL1B 距离（mm），8192=无效

// 函数名: App_Init
// 功能: 系统初始化（对应源 All_init + main 前半段）
// 说明: TIM6/TIM7 此时不启动，待启动键按下后再开中断。
void App_Init(void)
{
    BSP_UART_Init();                                // USART1 115200：调试+无线调参
    Param_Load();                                   // 读 Flash 参数，失败用默认并回写
    BSP_ADC_Init();                                 // ADC 十通道 DMA 循环采集
    lowpass_init(&filt_battery, 0.65f);             // 电池电压低通（源 All_init）
    BSP_Key_Init();                                 // PC13 启动键
    BSP_Encoder_Init();                             // 左右轮编码器
    BSP_PWM_Init();                                 // 电机+风扇 PWM
    BSP_PWM_SetDuty(BSP_PWM_FAN, FAN_DUTY_IDLE);    // 风扇空闲占空比
    BSP_LCD_Init();                                 // ST7735 小屏
    BSP_LCD_Clear(LCD_WHITE);
    BSP_LCD_SetBacklight(100);

    if(BSP_IMU660RB_Init() != 0)                    // IMU 失败不卡死，串口提示
    {
        BSP_UART_WriteString("IMU660RB init fail\r\n");
    }
    imu_proc_init();                                // 陀螺高通等滤波器初始化
    gyro_calibrate();                               // 上电静止校准（源工程被注释，本工程恢复）

    BSP_DL1B_Init();                                // DL1B 激光测距
}

// 函数名: App_Loop
// 功能: 主循环（对应源 main 的 while 结构）
// 说明: 停车状态下跑菜单/调参；检测到启动键按下后开启定时中断与看门狗。
void App_Loop(void)
{
    if(key_flag == 0)
    {
        read_adc();                         // 刷新电感值，让菜单页能看实时偏差
        track_error = get_track_error();
        track_out   = PID_track();

        /* 电池电压监测（启动前 TIM7 未运行，在此采样滤波，
         * 供菜单显示与低压保护；保护动作在 App_TaskISR 中执行） */
        battery      = (int16_t)BSP_ADC_Read(BSP_ADC_VBAT);
        battery_filt = (int16_t)lowpass_update(&filt_battery, (float)battery);

        menu();                             // 菜单/按键/屏幕刷新
        wireless_adjust();                  // 无线串口调参

        if(BSP_Key_StartPressed())          // 启动键按下
        {
            key_flag   = 1;
            Start_flag = 1;
            time = 0;                       // 清节拍计数，重新计时起跑延时
            Distance   = 0;                 // 清编码器累计距离
            distance_L = 0;
            distance_R = 0;
            HAL_TIM_Base_Start_IT(&htim6);  // 2ms 控制中断
            HAL_TIM_Base_Start_IT(&htim7);  // 5ms 辅助中断
            BSP_WDT_Init();                 // 使能独立看门狗
        }
    }
}

// 函数名: App_ControlISR
// 功能: TIM6 2ms 控制中断（对应源 pit_track）
void App_ControlISR(void)
{
    time++;

    if(Start_flag)
    {
        read_gyro_angle();                      // 陀螺仪读取与角度积分（仅运行时读取：
                                                // 停车菜单刷屏期间避免与 LCD 争用 SPI4 总线）
        BSP_PWM_SetDuty(BSP_PWM_FAN, FAN_DUTY_RUN);     // 负压电机运行占空比

        if(time > RUN_DELAY_COUNT)          // 起跑延时 1000×2ms = 2s
        {
            whole_test();                   // 循迹+元素控制主流程
        }
    }

    if(!Run_flag || !Start_flag)            // 未启动或出赛道：目标速度清零
    {
        target_speed_L = 0;
        target_speed_R = 0;
        if(!key_flag)
        {
            BSP_PWM_SetDuty(BSP_PWM_FAN, FAN_DUTY_IDLE);    // 风扇回空闲占空比
        }
    }

    read_encoder();                         // 编码器测速
    motor_control();                        // 速度闭环+电机输出
}

// 函数名: App_TaskISR
// 功能: TIM7 5ms 辅助中断（DL1B 测距、电池低压保护、喂狗、状态灯）
void App_TaskISR(void)
{
    BSP_DL1B_Update();                                  // DL1B 非阻塞轮询
    dl1b_distance_mm = BSP_DL1B_GetDistanceMm();

    /* 电池电压监测（源 key_start 的低压保护，5ms×200≈1s 后强制停车） */
    battery      = (int16_t)BSP_ADC_Read(BSP_ADC_VBAT);
    battery_filt = (int16_t)lowpass_update(&filt_battery, (float)battery);

    if(battery_filt < BATTERY_LOW_THRESHOLD && battery_filt > BATTERY_MIN_VALID)
    {
        if(++low_power_num > BATTERY_LOW_COUNT)
        {
            Start_flag = 0;
            key_flag   = 0;
        }
    }
    else
    {
        low_power_num = 0;
    }

    BSP_WDT_Feed();                                     // 喂狗

    /* 状态灯：运行时常亮，停车时慢闪（500ms 周期） */
    if(Start_flag)
    {
        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, LED_RUN_LEVEL);
    }
    else
    {
        static uint8_t led_cnt = 0;
        if(++led_cnt >= 50)                             // 50×5ms=250ms 翻转一次
        {
            led_cnt = 0;
            HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
        }
    }
}

// 函数名: HAL_TIM_PeriodElapsedCallback
// 功能: HAL 定时器周期回调（全工程唯一）：TIM6→控制环，TIM7→辅助任务
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM6)
    {
        App_ControlISR();
    }
    else if(htim->Instance == TIM7)
    {
        App_TaskISR();
    }
}

// 函数名: float_abs
// 功能: 计算浮点数的绝对值
float float_abs(float a)
{
    if(a < 0)
    {
        a = -a;
    }
    return a;
}
