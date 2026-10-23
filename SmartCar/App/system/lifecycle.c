/**
 * @file lifecycle.c
 * @brief 应用初始化与起跑事务。
 * @details 持有运行标志、控制拍、测距和看门狗生命周期；前台启动定时链，ISR经显式命令更新状态。
 */
/**
 * @note 移植来源：Sirius20260718/project/user/main.c、user/isr.c、code/config.c。
 * @note STC AI8051U 省赛快照（2026-07-18）；原算法未单列作者或独立许可证。
 * @note 保留源工程出处，关联 Phase 13/14/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include "app.h"
#include "system/lifecycle.h"
#include "system/safety.h"
#include "sensing/battery.h"
#include "services/param.h"
#include "sensing/track_sensor.h"
#include "sensing/imu_proc.h"
#include "control/pid.h"
#include "control/control.h"
#include "control/roundabout.h"
#include "control/motor.h"
#include "services/datalog.h"
#include "hmi/dashboard.h"
#include "sensing/gray_sensor.h"
#include "services/scb_link.h"
#include "services/telemetry.h"
#include "control/tuning.h"

/* 起跑前台复位、TIM6自增；单位为2ms拍。 */
static int32_t s_control_ticks = 0;
/* 前台发布、首次停车清零；每次读取保持volatile访问。 */
static volatile uint8_t s_key_flag = 0;
static volatile uint8_t s_start_flag = 0;
static volatile uint8_t s_run_flag = 0;
/* 控制拍更新的测距毫米值，8192表示无效。 */
static uint16_t s_dl1b_distance_mm = BSP_DL1B_INVALID;
/* 看门狗首次成功起跑后置位，停车期间仍需喂狗。 */
static uint8_t s_wdt_started = 0;
/* 上电参数加载返回值，仅初始化流程写入。 */
static uint8_t s_flash_available = 0;

static void Lifecycle_ResetRunState(void);

/**
 * @brief 按既有顺序初始化 BSP、参数和 App 模块。
 * @details 先经BSP恢复SPI4/5时钟，再初始化串口、Flash并加载参数；失败只打印提示，
 *     随后初始化采集、控制、电池、按键、编码器、PWM、LCD、IMU、测距与采样器，
 *     最后初始化SCB静态链路和独立上传队列。
 *     初始化通信不启动定时器，不改变参数失败门禁或ESP的EN。
 * @note 前台上电调用，可能阻塞；保持 SPI45SEL 补丁、参数失败路径、陀螺校准和定时器不启动。
 */
void Lifecycle_Init(void)
{
    /* SPI4/5 内核时钟 = PLL3Q 80MHz（SPI4÷8=10MHz，IMU660RB 上限）。
     * 实测写于 main 的 SysInit 会被后续 MX 外设初始化覆盖（SPI123SEL 与
     * SPI45SEL 同在 D2CCIP1R 寄存器），故放在所有 MX_* 之后重写一次 */
    BSP_Runtime_ConfigureSpiClock();

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
    Battery_Init(); /* 电池电压低通（源 All_init） */
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
    SCB_Init();
    Tuning_Init();
}

/**
 * @brief 检查停车起跑条件，并在全部通过后启用定时运行链。
 * @details 依次隔离在跑/待收尾状态及导出互斥，切断电机，检查标定与新鲜双传感器结果，
 *     持久化参数并建立日志会话；成功后复位运行历史、分配上传run_id并发布首快照，
 *     再启用标志和定时链。采样时基清零后固定RUN_STARTED，拒绝路径不分配运行身份。
 *     校验不能跳过灰度或电感中的任一套，避免所选模式有效而另一套保护失效时仍起跑。
 * @return 成功启用返回 1；非停车或任一门禁失败返回 0，并输出该门禁已有的拒绝信息
 *     （非停车分支无打印）。失败不启动定时链，不回滚已经完成的采样、参数保存或日志区操作。
 * @pre 由停车主循环的启动事件路径调用，初始化已经完成；ISR 不调用本函数。
 * @note 无显式形参；读取启动/待收尾标志、传感器、参数和日志状态，成功后更新运行历史、
 *     启动标志、按键武装及看门狗状态。含 UART/Flash 阻塞，非停车早退不改变在跑输出；
 *     真正进入门禁后电机保持零。标定配置仍需用户实测填写，本文不提供有效示例值。
 */
uint8_t Lifecycle_TryStart(void)
{
    /* 先隔离在跑/待收尾状态；真正进入门禁后立即切断电机输出，检查期间不驱动车轮。 */
    if (!App_IsStopped())
    {
        return 0;
    }
    if (Datalog_IsExporting())
    {
        return 0;
    }
    Motor_EmergencyStop();

    /* 标定必须覆盖全部通道，再刷新两类传感器；用本次结果检查采集成功与双信号有效。 */
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

    /* 先持久化待保存参数，再建立可记录的日志会话；任何一步失败都不能进入定时控制。 */
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

    /* 全部门禁通过后清除上次运行历史，再发布首个控制快照，避免首条日志混入旧状态。 */
    Lifecycle_ResetRunState();
    Telemetry_BeginRun();
    Datalog_PublishControl();

    /* 发布启动标志并要求按键释放，然后启动控制/辅助/采样链；首次启用 IWDG，以后只喂狗。 */
    BSP_Runtime_SetRunLed();
    s_key_flag = s_start_flag = 1;
    BSP_Key_RequireRelease();
    BSP_Runtime_StartTimers();
    BSP_Sampler_Start();
    /* 实际定时链启动后固定运行事件；采样tick已经由Sampler_Start清零。 */
    {
        telemetry_status_t upload;
        scb_event_t event = { 0 };
        Telemetry_GetStatus(&upload);
        event.kind = SCB_EVENT_RUN_STARTED;
        event.run_id = upload.run_id;
        event.revision = upload.revision;
        event.tick = BSP_Sampler_GetTick();
        event.control_sequence = Tracking_GetState()->control_sequence;
        (void)SCB_QueueEvent(&event);
    }

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

/**
 * @brief 起跑前复位运行历史，保留参数和零偏。
 * @details 先清控制拍数，再依次清低压计数、停车首因和pending，随后复位灰度序号、
 *     电机历史、PID、双模式控制、旧环岛和IMU运行历史；不清参数、滤波电池或陀螺零偏。
 * @note 仅既有起跑准备路径调用，不重复校准或改风扇参数。
 */
static void Lifecycle_ResetRunState(void)
{
    s_control_ticks = 0;
    Safety_Reset();
    Gray_ResetSequence();
    Motor_ResetRunState();
    PID_ResetAll();
    Control_ResetRunState();
    Roundabout_ResetRunState();
    IMU_ResetRunState();
}

/**
 * @brief 检查启动、按键及待收尾门禁。
 * @return 三个条件都为零返回1，否则0。
 * @note 前台/ISR可读；保留短路顺序与每次volatile读取。
 */
uint8_t Lifecycle_IsStopped(void)
{
    return !s_start_flag && !s_key_flag && !Safety_IsStopPending();
}

/**
 * @brief 读取定时控制启动标志。
 * @return 当前标志值，0或1。
 * @note 单次读值，不加锁或合并调用方重复读取。
 */
uint8_t Lifecycle_IsStarted(void)
{
    return s_start_flag;
}

/**
 * @brief 读取前台运行分支标志。
 * @return 当前标志值，0或1。
 * @note 单次读值，不加锁或合并调用方重复读取。
 */
uint8_t Lifecycle_IsKeyActive(void)
{
    return s_key_flag;
}

/**
 * @brief 读取等待期结束后的运行标志。
 * @return 当前标志值，0或1。
 * @note 单次读值，不加锁或合并调用方重复读取。
 */
uint8_t Lifecycle_IsRunning(void)
{
    return s_run_flag;
}

/**
 * @brief 读取本次运行2ms拍数。
 * @return 原有int32_t控制拍计数。
 * @note 单次读值，不加锁或合并调用方重复读取。
 */
int32_t Lifecycle_GetControlTicks(void)
{
    return s_control_ticks;
}

/**
 * @brief 读取最近控制拍的测距毫米值。
 * @return 距离毫米值，8192表示无效。
 * @note 单次读值，不加锁或合并调用方重复读取。
 */
uint16_t Lifecycle_GetDistanceMm(void)
{
    return s_dl1b_distance_mm;
}

/**
 * @brief 读取看门狗已启动标志。
 * @return 当前标志值，0或1。
 * @note 单次读值，不加锁或合并调用方重复读取。
 */
uint8_t Lifecycle_IsWatchdogStarted(void)
{
    return s_wdt_started;
}

/**
 * @brief 推进一个2ms控制拍。
 * @note 仅TIM6原自增位置调用，不改计数宽度。
 */
void Lifecycle_AdvanceControlTick(void)
{
    ++s_control_ticks;
}

/**
 * @brief 保存本拍的测距结果。
 * @param[in] distance_mm BSP测距毫米值，8192表示无效。
 * @note 仅TIM6原赋值位置调用，不夹紧输入。
 */
void Lifecycle_SetDistanceMm(uint16_t distance_mm)
{
    s_dl1b_distance_mm = distance_mm;
}

/**
 * @brief 发布等待期已结束的运行标志。
 * @note 仅原TIM6及旧控制入口赋值点调用。
 */
void Lifecycle_MarkRunning(void)
{
    s_run_flag = 1;
}

/**
 * @brief 按原链式顺序清除运行标志。
 * @pre 调用者已进入首次停车的PRIMASK临界区。
 * @note 不自行开关中断，保持key、run、start写入顺序。
 */
void Lifecycle_ClearRunFlags(void)
{
    s_start_flag = s_run_flag = s_key_flag = 0;
}

#if defined(SMARTCAR_HOST_TEST)
/**
 * @brief 在宿主夹具中布置运行状态。
 * @param[in] ticks 控制拍计数。
 * @param[in] key 前台运行标志，0或1。
 * @param[in] start 启动标志，0或1。
 * @param[in] run 运行标志，0或1。
 * @note 仅宿主测试编译；不操作硬件，不复位安全或算法状态。
 */
void Lifecycle_TestSetRunState(int32_t ticks, uint8_t key, uint8_t start, uint8_t run)
{
    s_control_ticks = ticks;
    s_key_flag = key;
    s_start_flag = start;
    s_run_flag = run;
}
#endif
