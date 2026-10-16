/**
 * @file app.c
 * @brief App 总装与起停门禁；前台及 TIM15/6/7 保持 1/2/5 ms 契约。
 * @note 移植来源：Sirius20260718/project/user/main.c、user/isr.c、code/config.c。
 * @note STC AI8051U 省赛快照（2026-07-18）；原算法未单列作者或独立许可证。
 * @note 保留源工程出处，关联 Phase 13/14/15 规约。
 */
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
#define APP_LED_RUN_LEVEL GPIO_PIN_SET

/* ==================== 全局变量（源 config.c） ==================== */
/* 前台/ISR 共享原全局，volatile 与链接属性保持；读写上下文不随改名变化。 */
uint8_t g_app_uart_tx_buffer[32]; /* 串口数据发送缓冲区 */
uint8_t g_app_uart_rx_buffer[32]; /* 串口数据接收缓冲区 */

int16_t g_app_battery_raw = 0;      /* 电池电压ADC原始值 */
int16_t g_app_battery_filtered = 0; /* 电池电压滤波值 */
int16_t g_app_low_power_count = 0;  /* 低压持续计数 */

int32_t g_app_control_ticks = 0; /* 控制节拍计数（2ms） */

volatile uint8_t g_app_key_flag = 0;   /* 启动键已按下 */
volatile uint8_t g_app_start_flag = 0; /* 启动标志位 */
volatile uint8_t g_app_run_flag = 0;   /* 运行标志位（赛道检测控制） */

filter_lowpass_t g_app_battery_filter; /* 电池电压低通滤波器（α=0.65） */

uint16_t g_app_dl1b_distance_mm = BSP_DL1B_INVALID; /* DL1B 距离（mm），8192=无效 */

/* 停车请求由 ISR 产生，阻塞式日志收尾在主循环执行。 */
static volatile uint8_t s_stop_pending = 0;
static volatile app_stop_reason_t s_stop_reason = APP_STOP_NONE;
static uint8_t s_wdt_started = 0;
static uint8_t s_flash_available = 0;

static void App_FinalizeStop(void);
static void App_ResetRunState(void);
static uint8_t App_TryStart(void);

uint8_t App_IsStopped(void)
{
    return !g_app_start_flag && !g_app_key_flag && !s_stop_pending;
}

void App_Init(void)
{
    /* SPI4/5 内核时钟 = PLL3Q 80MHz（SPI4÷8=10MHz，IMU660RB 上限）。
     * 实测写于 main 的 SysInit 会被后续 MX 外设初始化覆盖（SPI123SEL 与
     * SPI45SEL 同在 D2CCIP1R 寄存器），故放在所有 MX_* 之后重写一次 */
    MODIFY_REG(RCC->D2CCIP1R, RCC_D2CCIP1R_SPI45SEL, RCC_SPI45CLKSOURCE_PLL3);

    BSP_UART_Init();                  /* USART1 115200：调试+无线调参 */
    BSP_Flash_Init();                 /* W25Q64 JEDEC 自检，失败时服务接口返回错误 */
    s_flash_available = Param_Load(); /* 读/迁移参数；返回0时禁用日志并保留RAM默认/迁移值 */
    if (!s_flash_available)
    {
        BSP_UART_WriteString("W25Q64 unavailable; parameter/log writes disabled\r\n");
    }
    BSP_ADC_Init(); /* ADC 十通道 DMA 循环采集 */
    Gray_Init();
    Control_ResetRunState();
    Filter_InitLowpass(&g_app_battery_filter, 0.65f); /* 电池电压低通（源 All_init） */
    BSP_Key_Init();                                   /* PC13 启动键 */
    Motor_InitEncoder();                              /* 左右轮编码器 + 速度低通滤波器 */
    BSP_PWM_Init();                                   /* 电机+风扇 PWM */
    BSP_PWM_SetDuty(
        BSP_PWM_FAN, (uint32_t)Motor_GetState()->fan_duty_idle); /* 当前速度档空闲风扇占空比 */
    BSP_LCD_Init();                                              /* ST7735 小屏 */
    BSP_LCD_Clear(BSP_LCD_WHITE);
    BSP_LCD_SetBacklight(100);

    if (BSP_IMU660RB_Init() != 0) /* IMU 失败不卡死，串口提示 */
    {
        BSP_UART_WriteString("IMU660RB init fail\r\n");
    }
    IMU_Init();          /* 陀螺高通等滤波器初始化 */
    IMU_CalibrateGyro(); /* 上电静止校准（源工程被注释，本工程恢复） */

    BSP_DL1B_Init(); /* DL1B 激光测距 */

    BSP_Sampler_Init(); /* TIM15 1ms 高速采样器（编码器+IMU），待启动 */

#if !LCD_TARGET_EXTERNAL
    Dashboard_Init(); /* 板载屏：单页仪表盘标签 */
#endif
}

void App_Loop(void)
{
    if (s_wdt_started && g_app_key_flag == 0)
    {
        BSP_WDT_Feed(); /* TIM7 停止后的停车态喂狗路径 */
    }

    if (s_stop_pending)
    {
        App_FinalizeStop(); /* 日志尾包刷写只在主循环执行 */
    }

    if (g_app_key_flag)
    {
        Datalog_Service();
        if (s_stop_pending)
        {
            App_FinalizeStop();
        }
        return;
    }

    if (g_app_key_flag == 0)
    {
        /* 顶部检查后可能被停车ISR打断；界面/采样前再次完成收尾。 */
        if (s_stop_pending)
        {
            App_FinalizeStop();
        }
        Track_ReadAdc(); /* 刷新电感值，让菜单页能看实时偏差 */
        Gray_Update();
        (void)Track_UpdateError();
        g_control_track_output = PID_UpdateTrack();

        /* 电池电压监测（启动前 TIM7 未运行，在此采样滤波，
         * 供菜单显示与低压保护；保护动作在 App_TaskISR 中执行） */
        g_app_battery_raw = (int16_t)BSP_ADC_Read(BSP_ADC_VBAT);
        g_app_battery_filtered
            = (int16_t)Filter_UpdateLowpass(&g_app_battery_filter, (float)g_app_battery_raw);

#if LCD_TARGET_EXTERNAL
        Menu_Update(); /* 外接屏：完整按键菜单 */
#else
        Dashboard_Update(); /* 板载屏：单页仪表盘刷新 */
#endif
        Wireless_Adjust();   /* 无线串口调参 */
        Param_ServiceSave(); /* 串口改参静默3秒后保存 */

        /* 状态灯：停车时慢闪（主循环软件定时，TIM7 启动前也生效）。
         * PE3 蓝灯经 NPN 驱动，高电平点亮 */
        {
            static uint32_t s_led_last = 0;
            uint32_t now_ms = HAL_GetTick();
            if ((uint32_t)(now_ms - s_led_last) >= 250u)
            {
                s_led_last = now_ms;
                HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
            }
        }

        bsp_key_event_t event = BSP_Key_PollEvent();
        if (event == BSP_KEY_TOGGLE)
        {
            (void)Tracking_TrySetMode((uint8_t)(1u - Tracking_GetState()->mode));
        }
        if (event == BSP_KEY_START)
        {
            (void)App_TryStart();
        }
    }
}

/* 门禁期间始终PWM零；任何失败都不置启动标志、不启动三线定时器。 */
/**
 * @brief 按原门禁逐项尝试起跑。
 * @return 成功返回 1；任一既有门禁失败返回 0，不启动定时器。
 * @note 仅停车前台调用，可阻塞；失败保留原打印与副作用，不抽取合并分支或改变采样次数。
 */
static uint8_t App_TryStart(void)
{
    /* 1. 必须先确认停车且无待收尾；随后保持 PWM 为零再进入阻塞门禁。 */
    if (!App_IsStopped())
    {
        return 0;
    }
    Motor_EmergencyStop();

    /* 2. 真实标定、双采样、采集状态、灰度有效线和电感和按原次序检查。 */
    if (!Gray_CheckCalibration(1))
    {
        BSP_UART_WriteString("start rejected: calibration\r\n");
        return 0;
    }
    Track_ReadAdc();
    Gray_Update();
    if (Gray_GetState()->acquisition_status)
    {
        BSP_UART_WriteString("start rejected: gray acquisition\r\n");
        return 0;
    }
    if (!Gray_GetState()->line_valid)
    {
        BSP_UART_WriteString("start rejected: gray lost\r\n");
        return 0;
    }
    if (Track_GetState()->filtered[0] + Track_GetState()->filtered[1]
            + Track_GetState()->filtered[2] + Track_GetState()->filtered[3]
        <= 300)
    {
        BSP_UART_WriteString("start rejected: em lost\r\n");
        return 0;
    }

    /* 3. 先完成待保存参数，再建立完整日志会话；每个失败保留独立打印和返回。 */
    if (!Param_TryFlushPending())
    {
        BSP_UART_WriteString("start rejected: parameters\r\n");
        return 0;
    }
    if (Datalog_Start())
    {
        BSP_UART_WriteString("start rejected: log unavailable\r\n");
        return 0;
    }

    /* 4. 成功后才复位运行历史并发布首快照，保持参数和传感器零偏。 */
    App_ResetRunState();
    Datalog_PublishControl();

    /* 5. 灯、连锁标志、按键释放、TIM6/TIM7/采样器和看门狗次序保持。 */
    HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, APP_LED_RUN_LEVEL);
    g_app_key_flag = g_app_start_flag = 1;
    BSP_Key_RequireRelease();
    HAL_TIM_Base_Start_IT(&htim6);
    HAL_TIM_Base_Start_IT(&htim7);
    BSP_Sampler_Start();
    if (!s_wdt_started)
    {
        BSP_WDT_Init();
        s_wdt_started = 1;
    }
    else
    {
        BSP_WDT_Feed();
    }
    return 1;
}

void App_ControlISR(void)
{
    uint32_t start;
    uint16_t flags = 0;
    if (!g_app_start_flag)
    {
        return;
    }
    start = BSP_Time_GetCycles();
    ++g_app_control_ticks;
    Track_ReadAdc();
    Gray_Update();
    if (Gray_GetState()->acquisition_status)
    {
        flags |= TRACKING_PROTECT_ACQUIRE;
    }
    if (!Gray_GetState()->calibration_valid)
    {
        flags |= TRACKING_PROTECT_CALIBRATION;
    }
    if (!Gray_GetState()->acquisition_status && Gray_GetState()->calibration_valid
        && !Gray_GetState()->line_valid)
    {
        flags |= TRACKING_PROTECT_GRAY_LOST;
    }
    if (Track_GetState()->filtered[0] + Track_GetState()->filtered[1]
            + Track_GetState()->filtered[2] + Track_GetState()->filtered[3]
        <= 300)
    {
        flags |= TRACKING_PROTECT_EM_LOST;
    }
    Tracking_AddProtection(flags);
    /* 全部条件先记位；同拍首次原因严格按采集→灰度→电感排序。 */
    if (flags & (TRACKING_PROTECT_ACQUIRE | TRACKING_PROTECT_CALIBRATION))
    {
        App_RequestStop(APP_STOP_GRAY_ACQUIRE);
    }
    if (flags & TRACKING_PROTECT_GRAY_LOST)
    {
        App_RequestStop(APP_STOP_GRAY_LOST);
    }
    if (flags & TRACKING_PROTECT_EM_LOST)
    {
        App_RequestStop(APP_STOP_OFF_TRACK);
    }
    if (flags)
    {
        goto complete;
    }
    BSP_DL1B_Update();
    g_app_dl1b_distance_mm = BSP_DL1B_GetDistanceMm();
    IMU_UpdateGyroAngle();
    BSP_PWM_SetDuty(BSP_PWM_FAN, (uint32_t)Motor_GetState()->fan_duty);
    Tracking_Observe(1, 0);
    if (g_app_control_ticks > APP_RUN_DELAY_COUNT)
    {
        g_app_run_flag = 1;
        Tracking_Execute();
    }
    else
    {
        Motor_EmergencyStop();
    }
    if (!g_app_start_flag)
    {
        goto complete;
    }
    Motor_ReadEncoder();
    Tracking_AccumulateMotion();
    if (g_app_control_ticks > APP_RUN_DELAY_COUNT)
    {
        Motor_UpdateControl();
    }
complete:
    Tracking_CompleteControl();
    Datalog_PublishControl();
    /* 包含快照发布及错误路径；故障后再发布停车摘要但绝不继续控制。 */
    if ((uint32_t)(BSP_Time_GetCycles() - start) >= BSP_CONTROL_BUDGET_CYCLES)
    {
        App_RequestStop(APP_STOP_CONTROL_OVERRUN);
        Datalog_PublishControl();
    }
}

void App_TaskISR(void)
{
    /* 电池电压监测（源 key_start 的低压保护，5ms×200≈1s 后强制停车） */
    g_app_battery_raw = (int16_t)BSP_ADC_Read(BSP_ADC_VBAT);
    g_app_battery_filtered
        = (int16_t)Filter_UpdateLowpass(&g_app_battery_filter, (float)g_app_battery_raw);

    if (g_app_battery_filtered < APP_BATTERY_LOW_THRESHOLD
        && g_app_battery_filtered > APP_BATTERY_MIN_VALID)
    {
        if (++g_app_low_power_count > APP_BATTERY_LOW_COUNT)
        {
            App_RequestStop(APP_STOP_LOW_BATTERY);
        }
    }
    else
    {
        g_app_low_power_count = 0;
    }

    BSP_WDT_Feed(); /* 喂狗 */

    /* 状态灯由主循环负责（停车慢闪），此处仅在运行时保持常亮 */
    if (g_app_start_flag)
    {
        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, APP_LED_RUN_LEVEL);
    }
}

/**
 * @brief 保留 HAL 强符号回调分派 TIM6/TIM7。
 * @param[in] htim HAL 提供的非空定时器句柄；其他实例不操作。
 * @note 固定名称，不新增声明或回调注册；周期仍 2/5 ms。
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        App_ControlISR();
    }
    else if (htim->Instance == TIM7)
    {
        App_TaskISR();
    }
}

void App_SampleISR(void)
{
    if (g_app_start_flag)
    {
        Datalog_Push();
    }
}

void App_RequestStop(app_stop_reason_t reason)
{
    uint16_t flag = 0;
    uint8_t first;
    uint32_t mask;
    if (reason == APP_STOP_NONE)
    {
        return;
    }
    switch (reason)
    {
        case APP_STOP_OFF_TRACK:
            flag = TRACKING_PROTECT_EM_LOST;
            break;
        case APP_STOP_LOW_BATTERY:
            flag = TRACKING_PROTECT_LOW_BATTERY;
            break;
        case APP_STOP_GRAY_LOST:
            flag = TRACKING_PROTECT_GRAY_LOST;
            break;
        case APP_STOP_GRAY_ACQUIRE:
            flag = TRACKING_PROTECT_ACQUIRE;
            break;
        case APP_STOP_CONTROL_OVERRUN:
            flag = TRACKING_PROTECT_OVERRUN;
            break;
        case APP_STOP_LOG_OVERFLOW:
            flag = TRACKING_PROTECT_LOG_OVERFLOW;
            break;
        case APP_STOP_LOG_WRITE:
            flag = TRACKING_PROTECT_LOG_WRITE;
            break;
        case APP_STOP_LOG_FULL:
            flag = TRACKING_PROTECT_LOG_FULL;
            break;
        default:
            break;
    }
    Tracking_AddProtection(flag);
    /* 保护位先累加；原临界区只锁存第一次原因和标志，不吞掉后续保护位。 */
    mask = BSP_Time_EnterCritical();
    first = (s_stop_reason == APP_STOP_NONE);
    if (first)
    {
        s_stop_reason = reason;
        s_stop_pending = 1;
        g_app_start_flag = g_app_run_flag = g_app_key_flag = 0;
    }
    BSP_Time_ExitCritical(mask);
    Datalog_SetStopSummary((uint8_t)s_stop_reason, Tracking_GetState()->protection_flags);
    if (!first)
    {
        return;
    }

    /* 只对首次请求切断输出和三条调度线；阻塞式日志收尾留给前台。 */
    Motor_EmergencyStop();
    BSP_PWM_SetDuty(BSP_PWM_FAN, (uint32_t)Motor_GetState()->fan_duty_idle);
    BSP_Sampler_Stop();
    (void)HAL_TIM_Base_Stop_IT(&htim6);
    (void)HAL_TIM_Base_Stop_IT(&htim7);
    Tracking_Stopped((uint8_t)s_stop_reason);
}

/**
 * @brief 在前台完成日志尾包并报告首次停车。
 * @note 可能阻塞，禁止 ISR 调用；保留前后喂狗、一次报告、清 pending 和按键释放次序。
 */
static void App_FinalizeStop(void)
{
    app_stop_reason_t reason = s_stop_reason;

    if (s_wdt_started)
    {
        BSP_WDT_Feed();
    }
#if DATALOG_ENABLE
    Datalog_Stop();
#endif
    if (s_wdt_started)
    {
        BSP_WDT_Feed();
    }

    {
        const datalog_status_t *log = Datalog_GetStatus();
        char message[192];
        snprintf(message, sizeof(message),
            "stop: reason=%u protect=%u fault=%u accepted=%lu written=%lu rejected=%lu pending=%lu\r\n",
            (unsigned)reason, (unsigned)Tracking_GetState()->protection_flags, (unsigned)log->fault,
            (unsigned long)log->accepted, (unsigned long)log->written, (unsigned long)log->rejected,
            (unsigned long)log->pending);
        BSP_UART_WriteString(message);
    }
    s_stop_pending = 0;
    BSP_Key_RequireRelease();
}

/**
 * @brief 起跑前复位运行历史，保留参数和零偏。
 * @note 仅既有起跑准备路径调用，不重复校准或改风扇参数。
 */
static void App_ResetRunState(void)
{
    g_app_control_ticks = 0;
    g_app_low_power_count = 0;
    s_stop_reason = APP_STOP_NONE;
    s_stop_pending = 0;
    Gray_ResetSequence();
    Motor_ResetRunState();
    PID_ResetAll();
    Control_ResetRunState();
    Roundabout_ResetRunState();
    IMU_ResetRunState();
}

float App_AbsFloat(float a)
{
    if (a < 0)
    {
        a = -a;
    }
    return a;
}
