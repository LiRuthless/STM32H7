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
#include "gray_sensor.h"
#include <stdio.h>

/* 状态灯：板载蓝色 LED（PE3），原理图为 NPN 三极管驱动，高电平点亮 */
#define LED_RUN_LEVEL       GPIO_PIN_SET

// ==================== 全局变量（源 config.c） ====================
uint8_t  uart[32];              // 串口数据发送缓冲区
uint8_t  dat[32];               // 串口数据接收缓冲区

int16_t  battery = 0;           // 电池电压ADC原始值
int16_t  battery_filt = 0;      // 电池电压滤波值
int16_t  low_power_num = 0;     // 低压持续计数

int32_t  time = 0;              // 控制节拍计数（2ms）

volatile uint8_t  key_flag = 0;          // 启动键已按下
volatile uint8_t  Start_flag = 0;        // 启动标志位
volatile uint8_t  Run_flag = 0;          // 运行标志位（赛道检测控制）

LowPassFilter filt_battery;     // 电池电压低通滤波器（α=0.65）

uint16_t dl1b_distance_mm = BSP_DL1B_INVALID;   // DL1B 距离（mm），8192=无效

/* 停车请求由 ISR 产生，阻塞式日志收尾在主循环执行。 */
static volatile uint8_t s_stop_pending = 0;
static volatile app_stop_reason_t s_stop_reason = APP_STOP_NONE;
static uint8_t s_wdt_started = 0;
static uint8_t s_flash_available = 0;

static void App_FinalizeStop(void);
static void App_ResetRunState(void);
static uint8_t App_TryStart(void);
uint8_t App_IsStopped(void) { return !Start_flag && !key_flag && !s_stop_pending; }

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
    BSP_Flash_Init();                               // W25Q64 JEDEC 自检，失败时服务接口返回错误
    s_flash_available = Param_Load();               // 读/迁移参数；返回0时禁用日志并保留RAM默认/迁移值
    if(!s_flash_available)
    {
        BSP_UART_WriteString("W25Q64 unavailable; parameter/log writes disabled\r\n");
    }
    BSP_ADC_Init();                                 // ADC 十通道 DMA 循环采集
    Gray_Init();
    Control_ResetRunState();
    lowpass_init(&filt_battery, 0.65f);             // 电池电压低通（源 All_init）
    BSP_Key_Init();                                 // PC13 启动键
    encoder_init();                                 // 左右轮编码器 + 速度低通滤波器
    BSP_PWM_Init();                                 // 电机+风扇 PWM
    BSP_PWM_SetDuty(BSP_PWM_FAN, (uint32_t)Motor_GetState()->fan_duty_idle); // 当前速度档空闲风扇占空比
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

    if(key_flag)
    {
        Datalog_Service();
        if(s_stop_pending) App_FinalizeStop();
        return;
    }

    if(key_flag == 0)
    {
        /* 顶部检查后可能被停车ISR打断；界面/采样前再次完成收尾。 */
        if(s_stop_pending) App_FinalizeStop();
        read_adc();                         // 刷新电感值，让菜单页能看实时偏差
        Gray_Update();
        (void)get_track_error();
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
        Param_ServiceSave();                // 串口改参静默3秒后保存

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

        bsp_key_event_t event=BSP_Key_PollEvent();
        if(event==BSP_KEY_TOGGLE)
        {
            (void)Tracking_TrySetMode((uint8_t)(1u-Tracking_GetState()->mode));
        }
        if(event==BSP_KEY_START) (void)App_TryStart();
    }
}

/* 门禁期间始终PWM零；任何失败都不置启动标志、不启动三线定时器。 */
static uint8_t App_TryStart(void)
{
    if(!App_IsStopped()) return 0;
    Motor_EmergencyStop();
    if(!Gray_CheckCalibration(1)) {BSP_UART_WriteString("start rejected: calibration\r\n");return 0;}
    read_adc();Gray_Update();
    if(Gray_GetState()->acquisition_status) {BSP_UART_WriteString("start rejected: gray acquisition\r\n");return 0;}
    if(!Gray_GetState()->line_valid) {BSP_UART_WriteString("start rejected: gray lost\r\n");return 0;}
    if(Track_GetState()->filtered[0]+Track_GetState()->filtered[1]+Track_GetState()->filtered[2]+Track_GetState()->filtered[3]<=300)
    {BSP_UART_WriteString("start rejected: em lost\r\n");return 0;}
    if(!Param_TryFlushPending()) {BSP_UART_WriteString("start rejected: parameters\r\n");return 0;}
    if(Datalog_Start()) {BSP_UART_WriteString("start rejected: log unavailable\r\n");return 0;}
    App_ResetRunState();
    Datalog_PublishControl();
    HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,LED_RUN_LEVEL);
    key_flag=Start_flag=1;
    BSP_Key_RequireRelease();
    HAL_TIM_Base_Start_IT(&htim6);HAL_TIM_Base_Start_IT(&htim7);BSP_Sampler_Start();
    if(!s_wdt_started) {BSP_WDT_Init();s_wdt_started=1;} else BSP_WDT_Feed();
    return 1;
}

// 函数名: App_ControlISR
// 功能: TIM6 2ms 控制中断（对应源 pit_track）
void App_ControlISR(void)
{
    uint32_t start;
    uint16_t flags=0;
    if(!Start_flag) return;
    start=BSP_Time_GetCycles();
    ++time;
    read_adc();Gray_Update();
    if(Gray_GetState()->acquisition_status) flags|=PROTECT_ACQUIRE;
    if(!Gray_GetState()->calibration_valid) flags|=PROTECT_CALIBRATION;
    if(!Gray_GetState()->acquisition_status && Gray_GetState()->calibration_valid && !Gray_GetState()->line_valid)
        flags|=PROTECT_GRAY_LOST;
    if(Track_GetState()->filtered[0]+Track_GetState()->filtered[1]+Track_GetState()->filtered[2]+Track_GetState()->filtered[3]<=300)
        flags|=PROTECT_EM_LOST;
    Tracking_AddProtection(flags);
    /* 全部条件先记位；同拍首次原因严格按采集→灰度→电感排序。 */
    if(flags&(PROTECT_ACQUIRE|PROTECT_CALIBRATION)) App_RequestStop(APP_STOP_GRAY_ACQUIRE);
    if(flags&PROTECT_GRAY_LOST) App_RequestStop(APP_STOP_GRAY_LOST);
    if(flags&PROTECT_EM_LOST) App_RequestStop(APP_STOP_OFF_TRACK);
    if(flags) goto complete;
    BSP_DL1B_Update();dl1b_distance_mm=BSP_DL1B_GetDistanceMm();
    read_gyro_angle();
    BSP_PWM_SetDuty(BSP_PWM_FAN,(uint32_t)Motor_GetState()->fan_duty);
    Tracking_Observe(1,0);
    if(time>RUN_DELAY_COUNT) {Run_flag=1;Tracking_Execute();}
    else Motor_EmergencyStop();
    if(!Start_flag) goto complete;
    read_encoder();Tracking_AccumulateMotion();
    if(time>RUN_DELAY_COUNT) motor_control();
complete:
    Tracking_CompleteControl();Datalog_PublishControl();
    /* 包含快照发布及错误路径；故障后再发布停车摘要但绝不继续控制。 */
    if((uint32_t)(BSP_Time_GetCycles()-start)>=BSP_CONTROL_BUDGET_CYCLES)
    {App_RequestStop(APP_STOP_CONTROL_OVERRUN);Datalog_PublishControl();}
}

// 函数名: App_TaskISR
// 功能: TIM7 5ms 辅助中断（电池低压保护、喂狗、状态灯）
void App_TaskISR(void)
{
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
    uint16_t flag=0;
    uint8_t first;
    uint32_t mask;
    if(reason==APP_STOP_NONE) return;
    switch(reason) {
    case APP_STOP_OFF_TRACK:flag=PROTECT_EM_LOST;break;
    case APP_STOP_LOW_BATTERY:flag=PROTECT_LOW_BATTERY;break;
    case APP_STOP_GRAY_LOST:flag=PROTECT_GRAY_LOST;break;
    case APP_STOP_GRAY_ACQUIRE:flag=PROTECT_ACQUIRE;break;
    case APP_STOP_CONTROL_OVERRUN:flag=PROTECT_OVERRUN;break;
    case APP_STOP_LOG_OVERFLOW:flag=PROTECT_LOG_OVERFLOW;break;
    case APP_STOP_LOG_WRITE:flag=PROTECT_LOG_WRITE;break;
    case APP_STOP_LOG_FULL:flag=PROTECT_LOG_FULL;break;
    default:break;
    }
    Tracking_AddProtection(flag);
    mask=BSP_EnterCritical();
    first=(s_stop_reason==APP_STOP_NONE);
    if(first) {s_stop_reason=reason;s_stop_pending=1;Start_flag=Run_flag=key_flag=0;}
    BSP_ExitCritical(mask);
    Datalog_SetStopSummary((uint8_t)s_stop_reason,Tracking_GetState()->protection_flags);
    if(!first) return;
    Motor_EmergencyStop();
    BSP_PWM_SetDuty(BSP_PWM_FAN,(uint32_t)Motor_GetState()->fan_duty_idle);
    BSP_Sampler_Stop();
    (void)HAL_TIM_Base_Stop_IT(&htim6);(void)HAL_TIM_Base_Stop_IT(&htim7);
    Tracking_Stopped((uint8_t)s_stop_reason);
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

    {
        const datalog_status_t *log=Datalog_GetStatus();
        char message[192];
        snprintf(message,sizeof message,"stop: reason=%u protect=%u fault=%u accepted=%lu written=%lu rejected=%lu pending=%lu\r\n",
            (unsigned)reason,(unsigned)Tracking_GetState()->protection_flags,(unsigned)log->fault,
            (unsigned long)log->accepted,(unsigned long)log->written,(unsigned long)log->rejected,(unsigned long)log->pending);
        BSP_UART_WriteString(message);
    }
    s_stop_pending=0;
    BSP_Key_RequireRelease();
}

// 函数名: App_ResetRunState
// 功能: 再次起跑前集中恢复所有运行态历史，不改变参数与传感器零偏
static void App_ResetRunState(void)
{
    time = 0;
    low_power_num = 0;
    s_stop_reason=APP_STOP_NONE;s_stop_pending=0;
    Gray_ResetSequence();
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
