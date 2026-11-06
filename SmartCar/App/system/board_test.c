/**
 * @file board_test.c
 * @brief 单项上电采集、显示和IMU完整上传。
 * @details 测试模式绕过正常业务入口，ISR独占采集状态，前台复制快照绘图或顺序发送。
 *     队列由TIM15生产、前台消费，失败保留证据；不访问HAL句柄或寄存器。
 */
#include "bsp.h"
#include "bsp_board_test.h"
#include "system/board_test.h"
#include "sensing/battery.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define BOARD_TEST_QUEUE_SIZE 1024u
#define BOARD_TEST_OBSERVATION_CYCLES (480000000ull * 60ull)

typedef struct
{
    uint32_t count; /**< ISR采集次数，按键模式前台写。 */
    uint32_t last_cycles; /**< 上次IRQ入口DWT周期。 */
    uint64_t interval_cycles; /**< 逐拍间隔累计周期，避免DWT多次回绕。 */
    uint32_t interval_min; /**< 最短入口间隔周期，首拍不计。 */
    uint32_t interval_max; /**< 最长入口间隔周期。 */
    uint32_t execution_max; /**< 最长采集执行周期。 */
    uint32_t window_tick; /**< 当前窗口起点ms。 */
    uint32_t invalid_count; /**< DL1B无效轮询次数。 */
    uint32_t valid_tick; /**< 最近有效距离时刻ms。 */
    uint32_t short_count; /**< PC13短按累计。 */
    uint32_t long_count; /**< PC13长按累计。 */
    int64_t pulses; /**< ISR逐拍累加脉冲。 */
    int32_t value; /**< 当前ADC码/脉冲/距离/事件。 */
    int32_t filtered; /**< 既有电池滤波ADC码。 */
    uint16_t minimum; /**< 当前窗口最小ADC码。 */
    uint16_t maximum; /**< 当前窗口最大ADC码。 */
    uint16_t previous_min; /**< 上一完整窗口最小码。 */
    uint16_t previous_max; /**< 上一完整窗口最大码。 */
    uint16_t valid_distance; /**< 最近有效距离mm。 */
    uint8_t window_ready; /**< 上一完整窗口已发布。 */
    uint8_t valid; /**< DL1B当前有效标志。 */
    uint8_t status; /**< 灰度BSP状态码。 */
    bsp_gray_frame_t gray; /**< 上次完整成功帧，失败保留。 */
} board_test_snapshot_t;

typedef struct
{
    int16_t axis[3]; /**< TIM15写、前台按发布索引读取的XYZ原码。 */
} board_test_sample_t;

/* 所选ISR唯一写；前台短临界区复制。按键测试没有采集ISR，由前台写。 */
static volatile board_test_snapshot_t s_state;
/* 静态SPSC槽，发布head前内容完整，释放tail前发送完成。 */
static volatile board_test_sample_t s_queue[BOARD_TEST_QUEUE_SIZE];
/* ISR写head/采集统计，前台写tail/发送统计；单个32bit读取原子。 */
static volatile uint32_t s_head;
static volatile uint32_t s_tail;
static volatile uint32_t s_overflow;
static volatile uint32_t s_sent;
static volatile uint32_t s_send_errors;
static volatile uint32_t s_peak;
static volatile uint32_t s_stopped;
/* 前台发送缓冲及结果全程序寿命，失败冻结；不混入串口统计文本。 */
static uint8_t s_frame[16];
static volatile bsp_board_test_tx_t s_tx;
/* 初始化/启动失败由前台保存，IMU静默，屏幕模式显示。 */
static volatile uint8_t s_init_error;

/**
 * @brief 将本次ADC码加入非重叠一秒极值窗口。
 * @details 跨窗口先发布旧窗口再以本次值初始化新窗口，首次窗口结束前不显示伪极值。
 * @param[in] value 当前12bit ADC码。
 * @note 只在当前唯一采集上下文调用；窗口以毫秒时基自然回绕差判断。
 */
static void BoardTest_UpdateWindow(uint16_t value)
{
    uint32_t now = BSP_Time_GetTick();
    if ((uint32_t)(now - s_state.window_tick) >= 1000u)
    {
        s_state.previous_min = s_state.minimum;
        s_state.previous_max = s_state.maximum;
        s_state.window_ready = s_state.count > 1u;
        s_state.minimum = value;
        s_state.maximum = value;
        s_state.window_tick = now;
    }
    if (value < s_state.minimum)
    {
        s_state.minimum = value;
    }
    if (value > s_state.maximum)
    {
        s_state.maximum = value;
    }
}

/**
 * @brief 记录采集入口及逐拍间隔。
 * @details 首拍不计间隔，后续无符号差处理DWT回绕，并累计到64bit统计。
 * @return 本次入口周期，供出口测量执行耗时。
 * @note 所选ISR调用；不表示定时器挂起事件均被服务，板测须对照时间和计数。
 */
static uint32_t BoardTest_BeginCapture(void)
{
    uint32_t now = BSP_Time_GetCycles();
    if (s_state.count != 0u)
    {
        uint32_t interval = now - s_state.last_cycles;
        s_state.interval_cycles += interval;
        if (interval < s_state.interval_min)
        {
            s_state.interval_min = interval;
        }
        if (interval > s_state.interval_max)
        {
            s_state.interval_max = interval;
        }
    }
    s_state.last_cycles = now;
    s_state.count++;
    return now;
}

/**
 * @brief 更新本次采集最长耗时。
 * @details 以无符号周期差处理DWT回绕，仅在本次更长时替换历史最大值，不重置采集或间隔统计。
 * @param[in] start 本次入口DWT周期。
 * @note 所选ISR出口调用；不包含向量入口和HAL分派开销，板测须外部核对完整IRQ占用。
 */
static void BoardTest_EndCapture(uint32_t start)
{
    uint32_t duration = BSP_Time_GetCycles() - start;
    if (duration > s_state.execution_max)
    {
        s_state.execution_max = duration;
    }
}

/**
 * @brief TIM6独立对象采集分派。
 * @details 电感只读指定通道；灰度成功才替换完整帧；测距Update后读缓存并记录有效历史。
 * @note 唯一HAL回调测试分支调用，不进入控制、日志或姿态链。
 */
void BoardTest_ControlISR(void)
{
    uint32_t start = BoardTest_BeginCapture();
    if (BOARD_TEST_SELECT >= BOARD_TEST_IND_HL && BOARD_TEST_SELECT <= BOARD_TEST_IND_HR)
    {
        s_state.value = BSP_ADC_Read((bsp_adc_channel_t)(BOARD_TEST_SELECT - 2));
        BoardTest_UpdateWindow((uint16_t)s_state.value);
    }
    else if (BOARD_TEST_SELECT == BOARD_TEST_GRAY)
    {
        bsp_gray_frame_t frame;
        s_state.status = BSP_Gray_ReadFrame(&frame);
        if (s_state.status == BSP_GRAY_OK)
        {
            s_state.gray = frame;
        }
    }
    else if (BOARD_TEST_SELECT == BOARD_TEST_DL1B)
    {
        BSP_DL1B_Update();
        s_state.value = BSP_DL1B_GetDistanceMm();
        s_state.valid = s_state.value != BSP_DL1B_INVALID;
        if (s_state.valid)
        {
            s_state.valid_distance = (uint16_t)s_state.value;
            s_state.valid_tick = BSP_Time_GetTick();
        }
        else
        {
            s_state.invalid_count++;
        }
    }
    BoardTest_EndCapture(start);
}

/**
 * @brief TIM7独立电池采集和既有滤波。
 * @details 不执行低压停车或喂狗；读取电池模块本拍缓存并统计原码极值。
 * @note 唯一HAL回调测试分支调用，5ms周期。
 */
void BoardTest_TaskISR(void)
{
    uint32_t start = BoardTest_BeginCapture();
    Battery_Update();
    s_state.value = Battery_GetRaw();
    s_state.filtered = Battery_GetFiltered();
    BoardTest_UpdateWindow((uint16_t)s_state.value);
    BoardTest_EndCapture(start);
}

/**
 * @brief TIM15所选单轮或IMU采集及队列发布。
 * @details 单轮仅读清本轮。IMU每拍读取三轴，满时记录本样本溢出并停采；
 *     完整写槽后DMB发布head，前台尚未释放的tail槽不可覆盖。累计入口间隔达到60秒后停采排空。
 * @note 不等待串口、不操作LCD；驱动阻塞和坏读数不可识别限制保持。
 */
void BoardTest_SampleISR(void)
{
    uint32_t start = BoardTest_BeginCapture();
    if (BOARD_TEST_SELECT == BOARD_TEST_ENCODER_LEFT
        || BOARD_TEST_SELECT == BOARD_TEST_ENCODER_RIGHT)
    {
        s_state.value = BOARD_TEST_SELECT == BOARD_TEST_ENCODER_LEFT
            ? BSP_Encoder_GetLeft() : BSP_Encoder_GetRight();
        s_state.pulses += s_state.value;
    }
    else
    {
        int16_t raw[3];
        uint32_t head = s_head;
        uint32_t tail;
        uint32_t axis;
        if (BOARD_TEST_SELECT == BOARD_TEST_GYRO)
        {
            BSP_IMU660RB_GetGyro(&raw[0], &raw[1], &raw[2]);
        }
        else
        {
            BSP_IMU660RB_GetAcc(&raw[0], &raw[1], &raw[2]);
        }
        tail = s_tail;
        BSP_Time_MemoryBarrier();
        if ((uint32_t)(head - tail) == BOARD_TEST_QUEUE_SIZE)
        {
            s_overflow++;
            s_stopped = 1;
            BSP_BoardTest_StopTimer();
        }
        else
        {
            for (axis = 0; axis < 3u; axis++)
            {
                s_queue[head & (BOARD_TEST_QUEUE_SIZE - 1u)].axis[axis] = raw[axis];
            }
            BSP_Time_MemoryBarrier();
            s_head = head + 1u;
            if ((uint32_t)(head + 1u - tail) > s_peak)
            {
                s_peak = head + 1u - tail;
            }
        }
        if (s_state.interval_cycles >= BOARD_TEST_OBSERVATION_CYCLES)
        {
            s_stopped = 1;
            BSP_BoardTest_StopTimer();
        }
    }
    BoardTest_EndCapture(start);
}

/**
 * @brief 前台打包发送队首样本。
 * @details 明确float宽度并按整数位模式拆成小端字节；发送完整才释放槽。
 *     异常立即停采停发，当前帧和队列保持；线路完成不证明PC收到。
 * @note SPSC消费者；head读取后屏障再复制，ISR不改占用槽，不屏蔽采集中断。
 */
static void BoardTest_ServiceUpload(void)
{
    uint32_t tail = s_tail;
    uint32_t axis;
    typedef char board_test_float_width_t[sizeof(float) == 4 ? 1 : -1];
    (void)sizeof(board_test_float_width_t);
    if (s_send_errors != 0u || tail == s_head)
    {
        return;
    }
    BSP_Time_MemoryBarrier();
    for (axis = 0; axis < 3u; axis++)
    {
        float value = (float)s_queue[tail & (BOARD_TEST_QUEUE_SIZE - 1u)].axis[axis];
        uint32_t bits;
        uint32_t byte;
        memcpy(&bits, &value, sizeof(bits));
        for (byte = 0; byte < 4u; byte++)
        {
            s_frame[axis * 4u + byte] = (uint8_t)(bits >> (8u * byte));
        }
    }
    s_frame[12] = 0;
    s_frame[13] = 0;
    s_frame[14] = 0x80;
    s_frame[15] = 0x7f;
    s_tx = BSP_BoardTest_SendFrame(s_frame);
    if (s_tx.status == 0)
    {
        s_sent++;
        BSP_Time_MemoryBarrier();
        s_tail = tail + 1u;
    }
    else
    {
        BSP_BoardTest_StopTimer();
        s_stopped = 1;
        s_send_errors++;
    }
}

/**
 * @brief 绘制一行并清除旧字符尾部。
 * @details 先取得文本长度，再逐列覆盖20个字符；超出文本的列写空格，避免数字缩短留下残影。
 * @param[in] row 行号0～4。
 * @param[in] text 非空ASCII字符串，最多显示20字符。
 * @param[in] color 前景RGB565。
 * @note 仅前台调用；逐字符覆盖20列，不关采集中断。
 */
static void BoardTest_DrawRow(uint32_t row, const char *text, uint16_t color)
{
    uint32_t col;
    uint32_t length = strlen(text);
    for (col = 0; col < 20u; col++)
    {
        BSP_LCD_ShowChar((uint16_t)(col * 8u), (uint16_t)(row * 16u),
            col < length ? text[col] : ' ', color, BSP_LCD_BLACK);
    }
}

/**
 * @brief 将累计脉冲转换为十进制文本。
 * @details 无符号幅值处理INT64_MIN，再倒序展开数字，避免依赖nano库的long long格式支持。
 * @param[in] value 累计有符号脉冲。
 * @param[out] text 非空且至少21字节的文本缓冲。
 * @note 前台快照格式化调用，不读取或修改采集状态。
 */
static void BoardTest_FormatPulses(int64_t value, char *text)
{
    char digits[20];
    uint32_t count = 0;
    uint32_t offset = 0;
    uint64_t magnitude = value < 0 ? 0u - (uint64_t)value : (uint64_t)value;
    if (value < 0)
    {
        text[offset++] = '-';
    }
    do
    {
        digits[count++] = (char)('0' + magnitude % 10u);
        magnitude /= 10u;
    }
    while (magnitude != 0u);
    while (count != 0u)
    {
        text[offset++] = digits[--count];
    }
    text[offset] = 0;
}

/**
 * @brief 从一致快照生成五行显示。
 * @details 仅复制阶段短暂屏蔽中断，格式化及绘图在外；灰度失败保留帧且红色，首帧用占位。
 *     各模式按契约固定字段，脉冲累计使用64bit文本，不做未经核实物理量换算。
 * @note 前台100ms节流调用，不读取传感器硬件。
 */
static void BoardTest_Display(void)
{
    board_test_snapshot_t state;
    char lines[5][48] = {{0}};
    uint32_t row;
    uint32_t mask = BSP_Time_EnterCritical();
    state = s_state;
    BSP_Time_ExitCritical(mask);
    if (BOARD_TEST_SELECT == BOARD_TEST_GRAY)
    {
        for (row = 0; row < 4u; row++)
        {
            uint32_t col;
            for (col = 0; col < 4u; col++)
            {
                char value[8];
                if (state.gray.sequence == 0)
                {
                    strcpy(value, "----");
                }
                else
                {
                    snprintf(value, sizeof(value), "%04u", state.gray.raw[row * 4u + col]);
                }
                BSP_LCD_ShowString((uint16_t)(col * 40u), (uint16_t)(row * 16u),
                    value, state.status ? BSP_LCD_RED : BSP_LCD_WHITE, BSP_LCD_BLACK);
            }
        }
        snprintf(lines[4], sizeof(lines[4]), "S%u F%lu %luus", state.status,
            (unsigned long)state.gray.sequence, (unsigned long)(state.gray.scan_cycles / 480u));
        BoardTest_DrawRow(4, lines[4], BSP_LCD_WHITE);
        return;
    }
    if (BOARD_TEST_SELECT == BOARD_TEST_LCD)
    {
        snprintf(lines[0], sizeof(lines[0]), "01234567890123456789");
        snprintf(lines[1], sizeof(lines[1]), "NEG -123 POS +456");
        snprintf(lines[2], sizeof(lines[2]), "COUNT %lu", (unsigned long)BSP_Time_GetTick());
        snprintf(lines[3], sizeof(lines[3]), "%s", (BSP_Time_GetTick() / 1000u) & 1u
            ? "12345678901234567890" : "1");
        snprintf(lines[4], sizeof(lines[4]), "BOTTOM 0123456789012");
    }
    else if (BOARD_TEST_SELECT == BOARD_TEST_DL1B)
    {
        snprintf(lines[0], 48, "MM %ld", (long)state.value);
        snprintf(lines[1], 48, "VALID %u", state.valid);
        snprintf(lines[2], 48, "LAST %u mm", state.valid_distance);
        if (state.valid_distance == BSP_DL1B_INVALID)
        {
            snprintf(lines[3], 48, "AGE ----");
        }
        else
        {
            snprintf(lines[3], 48, "AGE %lu ms",
                (unsigned long)(BSP_Time_GetTick() - state.valid_tick));
        }
        snprintf(lines[4], 48, "INVALID %lu", (unsigned long)state.invalid_count);
    }
    else if (BOARD_TEST_SELECT == BOARD_TEST_PC13)
    {
        snprintf(lines[0], 48, "EVENT %ld", (long)state.value);
        snprintf(lines[1], 48, "SHORT %lu", (unsigned long)state.short_count);
        snprintf(lines[2], 48, "LONG %lu", (unsigned long)state.long_count);
    }
    else if (BOARD_TEST_SELECT == BOARD_TEST_ENCODER_LEFT
        || BOARD_TEST_SELECT == BOARD_TEST_ENCODER_RIGHT)
    {
        snprintf(lines[0], 48, "PULSE %ld", (long)state.value);
        char pulses[21];
        BoardTest_FormatPulses(state.pulses, pulses);
        snprintf(lines[1], 48, "SUM %s", pulses);
        snprintf(lines[2], 48, "N %lu", (unsigned long)state.count);
        snprintf(lines[3], 48, "DT %lu/%lu us", (unsigned long)(state.interval_min / 480u),
            (unsigned long)(state.interval_max / 480u));
        snprintf(lines[4], 48, "MAX %lu us", (unsigned long)(state.execution_max / 480u));
    }
    else
    {
        snprintf(lines[0], 48, "RAW %ld", (long)state.value);
        if (BOARD_TEST_SELECT == BOARD_TEST_BATTERY)
        {
            snprintf(lines[1], 48, "FILTER %ld", (long)state.filtered);
            snprintf(lines[2], 48, state.window_ready ? "MIN %u" : "MIN ----", state.previous_min);
            snprintf(lines[3], 48, state.window_ready ? "MAX %u" : "MAX ----", state.previous_max);
        }
        else
        {
            snprintf(lines[1], 48, state.window_ready ? "MIN %u" : "MIN ----", state.previous_min);
            snprintf(lines[2], 48, state.window_ready ? "MAX %u" : "MAX ----", state.previous_max);
            snprintf(lines[3], 48, state.window_ready ? "PP %u" : "PP ----",
                state.previous_max - state.previous_min);
        }
        snprintf(lines[4], 48, "N %lu", (unsigned long)state.count);
    }
    for (row = 0; row < 5u; row++)
    {
        BoardTest_DrawRow(row, lines[row], BSP_LCD_WHITE);
    }
}

/**
 * @brief 执行当前选择的公共上电与前台服务。
 * @details 全部状态先于BSP初始化及IRQ启用复位；失败不进入正常业务。
 *     IMU只发二进制，其他模式100ms显示；按键每次前台循环消费一次，不进入定时ISR。
 * @note 独立入口调用，永不返回；不可在正常业务启动后调用。
 */
static void BoardTest_RunSelected(void)
{
    uint32_t display_tick;
    board_test_snapshot_t initial = {0};
    s_state = initial;
    s_state.minimum = 4095;
    s_state.interval_min = UINT32_MAX;
    s_state.valid_distance = BSP_DL1B_INVALID;
    s_head = 0;
    s_tail = 0;
    s_overflow = 0;
    s_sent = 0;
    s_send_errors = 0;
    s_peak = 0;
    s_stopped = 0;
    s_init_error = BSP_BoardTest_Init();
    Battery_Init();
    s_state.window_tick = BSP_Time_GetTick();
    display_tick = s_state.window_tick - 100u;
    if (s_init_error == 0)
    {
        s_init_error = BSP_BoardTest_StartTimer();
    }
    while (1)
    {
        if (s_init_error != 0)
        {
            if (BOARD_TEST_SELECT != BOARD_TEST_GYRO && BOARD_TEST_SELECT != BOARD_TEST_ACCEL)
            {
                BoardTest_DrawRow(0, "INIT/START ERROR", BSP_LCD_RED);
            }
            continue;
        }
        if (BOARD_TEST_SELECT == BOARD_TEST_GYRO || BOARD_TEST_SELECT == BOARD_TEST_ACCEL)
        {
            BoardTest_ServiceUpload();
            continue;
        }
        if (BOARD_TEST_SELECT == BOARD_TEST_ADC_KEY1 || BOARD_TEST_SELECT == BOARD_TEST_ADC_KEY2)
        {
            s_state.value = BSP_ADC_Read(BOARD_TEST_SELECT == BOARD_TEST_ADC_KEY1
                ? BSP_ADC_KEY1 : BSP_ADC_KEY2);
            s_state.count++;
            BoardTest_UpdateWindow((uint16_t)s_state.value);
        }
        if (BOARD_TEST_SELECT == BOARD_TEST_PC13)
        {
            bsp_key_event_t event = BSP_Key_PollEvent();
            if (event != BSP_KEY_NONE)
            {
                s_state.value = event;
                s_state.short_count += event == BSP_KEY_START;
                s_state.long_count += event == BSP_KEY_TOGGLE;
            }
        }
        if ((uint32_t)(BSP_Time_GetTick() - display_tick) >= 100u)
        {
            display_tick = BSP_Time_GetTick();
            BoardTest_Display();
        }
    }
}

/**
 * @brief 上电直接运行Lcd独立测试。
 * @details 委托公共服务，采集对象由编译期唯一选择确定，不能运行时混选。
 * @pre BOARD_TEST_SELECT对应本入口，正常业务尚未启动。
 * @note 仅前台上电调用，永不返回。
 */
void BoardTest_RunLcd(void)
{
    BoardTest_RunSelected();
}

/**
 * @brief 上电直接运行InductanceHL独立测试。
 * @details 委托公共服务，采集对象由编译期唯一选择确定，不能运行时混选。
 * @pre BOARD_TEST_SELECT对应本入口，正常业务尚未启动。
 * @note 仅前台上电调用，永不返回。
 */
void BoardTest_RunInductanceHL(void)
{
    BoardTest_RunSelected();
}

/**
 * @brief 上电直接运行InductanceVL独立测试。
 * @details 委托公共服务，采集对象由编译期唯一选择确定，不能运行时混选。
 * @pre BOARD_TEST_SELECT对应本入口，正常业务尚未启动。
 * @note 仅前台上电调用，永不返回。
 */
void BoardTest_RunInductanceVL(void)
{
    BoardTest_RunSelected();
}

/**
 * @brief 上电直接运行InductanceVR独立测试。
 * @details 委托公共服务，采集对象由编译期唯一选择确定，不能运行时混选。
 * @pre BOARD_TEST_SELECT对应本入口，正常业务尚未启动。
 * @note 仅前台上电调用，永不返回。
 */
void BoardTest_RunInductanceVR(void)
{
    BoardTest_RunSelected();
}

/**
 * @brief 上电直接运行InductanceHR独立测试。
 * @details 委托公共服务，采集对象由编译期唯一选择确定，不能运行时混选。
 * @pre BOARD_TEST_SELECT对应本入口，正常业务尚未启动。
 * @note 仅前台上电调用，永不返回。
 */
void BoardTest_RunInductanceHR(void)
{
    BoardTest_RunSelected();
}

/**
 * @brief 上电直接运行Gray独立测试。
 * @details 委托公共服务，采集对象由编译期唯一选择确定，不能运行时混选。
 * @pre BOARD_TEST_SELECT对应本入口，正常业务尚未启动。
 * @note 仅前台上电调用，永不返回。
 */
void BoardTest_RunGray(void)
{
    BoardTest_RunSelected();
}

/**
 * @brief 上电直接运行Gyro独立测试。
 * @details 委托公共服务，采集对象由编译期唯一选择确定，不能运行时混选。
 * @pre BOARD_TEST_SELECT对应本入口，正常业务尚未启动。
 * @note 仅前台上电调用，永不返回。
 */
void BoardTest_RunGyro(void)
{
    BoardTest_RunSelected();
}

/**
 * @brief 上电直接运行Accel独立测试。
 * @details 委托公共服务，采集对象由编译期唯一选择确定，不能运行时混选。
 * @pre BOARD_TEST_SELECT对应本入口，正常业务尚未启动。
 * @note 仅前台上电调用，永不返回。
 */
void BoardTest_RunAccel(void)
{
    BoardTest_RunSelected();
}

/**
 * @brief 上电直接运行Dl1b独立测试。
 * @details 委托公共服务，采集对象由编译期唯一选择确定，不能运行时混选。
 * @pre BOARD_TEST_SELECT对应本入口，正常业务尚未启动。
 * @note 仅前台上电调用，永不返回。
 */
void BoardTest_RunDl1b(void)
{
    BoardTest_RunSelected();
}

/**
 * @brief 上电直接运行EncoderLeft独立测试。
 * @details 委托公共服务，采集对象由编译期唯一选择确定，不能运行时混选。
 * @pre BOARD_TEST_SELECT对应本入口，正常业务尚未启动。
 * @note 仅前台上电调用，永不返回。
 */
void BoardTest_RunEncoderLeft(void)
{
    BoardTest_RunSelected();
}

/**
 * @brief 上电直接运行EncoderRight独立测试。
 * @details 委托公共服务，采集对象由编译期唯一选择确定，不能运行时混选。
 * @pre BOARD_TEST_SELECT对应本入口，正常业务尚未启动。
 * @note 仅前台上电调用，永不返回。
 */
void BoardTest_RunEncoderRight(void)
{
    BoardTest_RunSelected();
}

/**
 * @brief 上电直接运行Battery独立测试。
 * @details 委托公共服务，采集对象由编译期唯一选择确定，不能运行时混选。
 * @pre BOARD_TEST_SELECT对应本入口，正常业务尚未启动。
 * @note 仅前台上电调用，永不返回。
 */
void BoardTest_RunBattery(void)
{
    BoardTest_RunSelected();
}

/**
 * @brief 上电直接运行AdcKey1独立测试。
 * @details 委托公共服务，采集对象由编译期唯一选择确定，不能运行时混选。
 * @pre BOARD_TEST_SELECT对应本入口，正常业务尚未启动。
 * @note 仅前台上电调用，永不返回。
 */
void BoardTest_RunAdcKey1(void)
{
    BoardTest_RunSelected();
}

/**
 * @brief 上电直接运行AdcKey2独立测试。
 * @details 委托公共服务，采集对象由编译期唯一选择确定，不能运行时混选。
 * @pre BOARD_TEST_SELECT对应本入口，正常业务尚未启动。
 * @note 仅前台上电调用，永不返回。
 */
void BoardTest_RunAdcKey2(void)
{
    BoardTest_RunSelected();
}

/**
 * @brief 上电直接运行Pc13独立测试。
 * @details 委托公共服务，采集对象由编译期唯一选择确定，不能运行时混选。
 * @pre BOARD_TEST_SELECT对应本入口，正常业务尚未启动。
 * @note 仅前台上电调用，永不返回。
 */
void BoardTest_RunPc13(void)
{
    BoardTest_RunSelected();
}

/**
 * @brief 分派编译选择的唯一独立上电入口。
 * @details NONE不调用任何测试；测试入口返回后永久停留，禁止落入正常业务。
 * @note main的SysInit补丁后调用，仅启动时使用。
 */
void BoardTest_Run(void)
{
    switch (BOARD_TEST_SELECT)
    {
        case 1:
            BoardTest_RunLcd();
            break;
        case 2:
            BoardTest_RunInductanceHL();
            break;
        case 3:
            BoardTest_RunInductanceVL();
            break;
        case 4:
            BoardTest_RunInductanceVR();
            break;
        case 5:
            BoardTest_RunInductanceHR();
            break;
        case 6:
            BoardTest_RunGray();
            break;
        case 7:
            BoardTest_RunGyro();
            break;
        case 8:
            BoardTest_RunAccel();
            break;
        case 9:
            BoardTest_RunDl1b();
            break;
        case 10:
            BoardTest_RunEncoderLeft();
            break;
        case 11:
            BoardTest_RunEncoderRight();
            break;
        case 12:
            BoardTest_RunBattery();
            break;
        case 13:
            BoardTest_RunAdcKey1();
            break;
        case 14:
            BoardTest_RunAdcKey2();
            break;
        case 15:
            BoardTest_RunPc13();
            break;
        default:
            break;
    }
    while (1)
    {
        /* 测试返回也保持启动隔离。 */
    }
}
