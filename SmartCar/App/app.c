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
#include "roundabout.h"
#include "motor.h"
#include "filter.h"
#include "datalog.h"
#include "dashboard.h"

/* 状态灯：板载蓝色 LED（PE3），原理图为 NPN 三极管驱动，高电平点亮 */
#define LED_RUN_LEVEL       GPIO_PIN_SET

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

/* 停车请求由 ISR 产生，阻塞式日志收尾在主循环执行。 */
static volatile uint8_t s_stop_pending = 0;
static volatile app_stop_reason_t s_stop_reason = APP_STOP_NONE;
static uint8_t s_wdt_started = 0;

static void App_FinalizeStop(void);
static void App_ResetRunState(void);

// 函数名: App_Init
// 功能: 系统初始化（对应源 All_init + main 前半段）
// 说明: TIM6/TIM7 此时不启动，待启动键按下后再开中断。
void App_Init(void)
{
    /* SPI4/5 内核时钟 = PLL3Q 80MHz（SPI4÷8=10MHz，IMU660RB 上限）。
     * 实测写于 main 的 SysInit 会被后续 MX 外设初始化覆盖（SPI123SEL 与
     * SPI45SEL 同在 D2CCIP1R 寄存器），故放在所有 MX_* 之后重写一次 */
    MODIFY_REG(RCC->D2CCIP1R, RCC_D2CCIP1R_SPI45SEL, RCC_SPI45CLKSOURCE_PLL3);

    BSP_UART_Init();                                // USART1 115200：调试+无线调参
    Param_Load();                                   // 读 Flash 参数，失败用默认并回写
    BSP_ADC_Init();                                 // ADC 十通道 DMA 循环采集
    lowpass_init(&filt_battery, 0.65f);             // 电池电压低通（源 All_init）
    BSP_Key_Init();                                 // PC13 启动键
    encoder_init();                                 // 左右轮编码器 + 速度低通滤波器
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

    BSP_Sampler_Init();                             // TIM15 1ms 高速采样器（编码器+IMU），待启动

#if !LCD_TARGET_EXTERNAL
    Dashboard_Init();                               // 板载屏：单页仪表盘标签
#endif
}

// 函数名: App_Loop
// 功能: 主循环（对应源 main 的 while 结构）
// 说明: 停车状态下跑菜单/调参；检测到启动键按下后开启定时中断与看门狗。
void App_Loop(void)
{
    if(s_wdt_started && key_flag == 0)
    {
        BSP_WDT_Feed();                     // TIM7 停止后的停车态喂狗路径
    }

    if(s_stop_pending)
    {
        App_FinalizeStop();                 // 日志尾包刷写只在主循环执行
    }

    if(key_flag == 0)
    {
        read_adc();                         // 刷新电感值，让菜单页能看实时偏差
        track_error = get_track_error();
        track_out   = PID_track();

        /* 电池电压监测（启动前 TIM7 未运行，在此采样滤波，
         * 供菜单显示与低压保护；保护动作在 App_TaskISR 中执行） */
        battery      = (int16_t)BSP_ADC_Read(BSP_ADC_VBAT);
        battery_filt = (int16_t)lowpass_update(&filt_battery, (float)battery);

#if LCD_TARGET_EXTERNAL
        menu();                             // 外接屏：完整按键菜单
#else
        Dashboard_Update();                 // 板载屏：单页仪表盘刷新
#endif
        wireless_adjust();                  // 无线串口调参

        /* 状态灯：停车时慢闪（主循环软件定时，TIM7 启动前也生效）。
         * PE3 蓝灯经 NPN 驱动，高电平点亮 */
        {
            static uint32_t led_last = 0;
            uint32_t now_ms = HAL_GetTick();
            if((uint32_t)(now_ms - led_last) >= 250u)
            {
                led_last = now_ms;
                HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
            }
        }

        if(BSP_Key_StartPressed())          // 启动键按下
        {
#if DATALOG_ENABLE
            Datalog_Start();                // 复位日志区（W25Q64 边写边擦，仅几十 ms）
#endif
            App_ResetRunState();
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, LED_RUN_LEVEL); // 运行常亮
            key_flag   = 1;
            Start_flag = 1;
            HAL_TIM_Base_Start_IT(&htim6);  // 2ms 控制中断
            HAL_TIM_Base_Start_IT(&htim7);  // 5ms 辅助中断
            BSP_Sampler_Start();            // 1ms 高速采样器（编码器+IMU，供控制环与数据记录）
            if(!s_wdt_started)
            {
                BSP_WDT_Init();             // IWDG 一旦启动不可关闭，只初始化一次
                s_wdt_started = 1;
            }
            else
            {
                BSP_WDT_Feed();
            }
        }
    }
}

// 函数名: App_ControlISR
// 功能: TIM6 2ms 控制中断（对应源 pit_track）
void App_ControlISR(void)
{
    time++;

    BSP_DL1B_Update();                                  // DL1B 激光测距轮询（2ms，原在 TIM7 5ms）
    dl1b_distance_mm = BSP_DL1B_GetDistanceMm();

    if(Start_flag)
    {
        read_gyro_angle();                      // 陀螺仪读取与角度积分（仅运行时读取：
                                                // 停车菜单刷屏期间避免与 LCD 争用 SPI4 总线）
        BSP_PWM_SetDuty(BSP_PWM_FAN, FAN_DUTY_RUN);     // 负压电机运行占空比

        if(time > RUN_DELAY_COUNT)          // 起跑延时 1000×2ms = 2s
        {
            whole_test();                   // 循迹+元素控制主流程
            if(!Start_flag)                 // whole_test 可能已发出安全停车请求
            {
                return;
            }
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
// 功能: TIM7 5ms 辅助中断（日志刷写、电池低压保护、喂狗、状态灯）
void App_TaskISR(void)
{
    Datalog_Flush();                                    // 运行数据记录刷入 Flash

    /* 电池电压监测（源 key_start 的低压保护，5ms×200≈1s 后强制停车） */
    battery      = (int16_t)BSP_ADC_Read(BSP_ADC_VBAT);
    battery_filt = (int16_t)lowpass_update(&filt_battery, (float)battery);

    if(battery_filt < BATTERY_LOW_THRESHOLD && battery_filt > BATTERY_MIN_VALID)
    {
        if(++low_power_num > BATTERY_LOW_COUNT)
        {
            App_RequestStop(APP_STOP_LOW_BATTERY);
        }
    }
    else
    {
        low_power_num = 0;
    }

    BSP_WDT_Feed();                                     // 喂狗

    /* 状态灯由主循环负责（停车慢闪），此处仅在运行时保持常亮 */
    if(Start_flag)
    {
        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, LED_RUN_LEVEL);
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

// 函数名: App_SampleISR
// 功能: 1ms 高速采样钩子（由 TIM15 采样器中断调用）：运行中记录一条数据
void App_SampleISR(void)
{
    if(Start_flag)
    {
        Datalog_Push();
    }
}

// 函数名: App_RequestStop
// 功能: ISR 可调用的有界安全停车入口；阻塞式收尾延后到主循环
void App_RequestStop(app_stop_reason_t reason)
{
    if(reason == APP_STOP_NONE || s_stop_pending)
    {
        return;
    }

    s_stop_reason = reason;                 // 首次原因优先，随后锁定请求
    s_stop_pending = 1;
    Start_flag = 0;
    Run_flag = 0;
    key_flag = 0;

    Motor_EmergencyStop();                  // 必须先于停止调度切断两路电机 PWM
    BSP_PWM_SetDuty(BSP_PWM_FAN, FAN_DUTY_IDLE);
    BSP_Sampler_Stop();                     // 先释放 SPI4 的 IMU 周期访问
    (void)HAL_TIM_Base_Stop_IT(&htim6);
    (void)HAL_TIM_Base_Stop_IT(&htim7);
}

// 函数名: App_FinalizeStop
// 功能: 主循环完成可能阻塞的日志尾包刷写并报告首次停车原因
static void App_FinalizeStop(void)
{
    app_stop_reason_t reason = s_stop_reason;

    if(s_wdt_started)
    {
        BSP_WDT_Feed();
    }
#if DATALOG_ENABLE
    Datalog_Stop();
#endif
    if(s_wdt_started)
    {
        BSP_WDT_Feed();
    }

    if(reason == APP_STOP_OFF_TRACK)
    {
        BSP_UART_WriteString("stop: off track\r\n");
    }
    else if(reason == APP_STOP_LOW_BATTERY)
    {
        BSP_UART_WriteString("stop: low battery\r\n");
    }

    s_stop_reason = APP_STOP_NONE;
    s_stop_pending = 0;
}

// 函数名: App_ResetRunState
// 功能: 再次起跑前集中恢复所有运行态历史，不改变参数与传感器零偏
static void App_ResetRunState(void)
{
    time = 0;
    low_power_num = 0;
    Motor_ResetRunState();
    PID_ResetAll();
    Control_ResetRunState();
    Roundabout_ResetRunState();
    IMU_ResetRunState();
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
