# Phase 15 注释专项：固定模板与完整成品

> 2026-10-16 用户确认维护级“C 完整、H 摘要”。本文件仅为 Markdown 规约。
> 三个成品均以 `1eca21c` 的真实函数为基线，只换注释，不改任何非注释 token。
> 这里的完整函数不是已应用的源码补丁；本轮不回写 C/H，也不声称新的构建通过。

## 六类固定模板

所有多行块的每个正文行以 ` * ` 开始，标签后一个空格；续行使用 ` *     `。
整个块随代码层级缩进，注释与定义/阶段之间无空白行。普通行宽 <=100；版权/来源例外
在 comment-review 的 E 表登记。以下占位模板不是关于实际硬件的事实。

### T-01 文件说明

```c
/**
 * @file module.c
 * @brief 一句话说明模块职责。
 * @details 说明职责边界、输入到输出的数据流及主要调用关系。
 *     说明状态归属、调度上下文和并发限制；配置/聚合头写自己的配置或依赖职责。
 */
```

### T-02 C 函数完整说明与 H 摘要

C 使用 brief→details→param→return→pre→note；每个参数按 C 定义的实际形参名和顺序说明，
不为匹配旧 H 注释而改形参名。省略仅限没有参数、void 返回、没有额外前置条件，或经人工
确认的简单类别无需 details；省略原因在清单中记录，不填“同上”“无”。

```c
/**
 * @brief 一句话说明函数职责。
 * @details 说明算法、处理阶段及关键设计原因。
 * @param[in] input 输入含义、单位和合法范围。
 * @param[out] output 输出含义和写入条件。
 * @return 返回值及失败语义。
 * @pre 调用前必须满足的条件。
 * @note 调用上下文、阻塞性及共享状态影响。
 */
```

H 只保留 brief/note。以下为真实公共接口的摘要成品，不复制参数长表：

```c
/**
 * @brief 计算 16 路灰度强度的带有效性标志质心偏差。
 * @note 两指针非空；零偏差不代表有线，须读取 line_valid。
 *     完整参数、算法和边界见 gray_sensor.c::Gray_ComputeError。
 */
int16_t Gray_ComputeError(const uint16_t *strength, uint8_t *line_valid);
```

### T-03 处理阶段

阶段说明放在相应代码前，解释本阶段为什么存在及失败/顺序边界，不逐句翻译赋值。

```c
    /*
     * 解释本阶段目标及与下一阶段的关系。
     * 解释必须保持的状态或时序约束。
     */
```

短阶段可用 `/* 完整说明。 */`；不使用装饰线或“步骤已按原逻辑保留”作为内容。

### T-04 字段与枚举成员

```c
    uint32_t sequence; /**< 成功完整帧序号，自然回绕，失败时保持。 */
    OPTICAL_WAIT = 0, /**< 等待有效起跑；状态观察器负责推进。 */
```

两个示例分别属于结构体和枚举，不能把它们拼入同一个声明。超长说明在成员前使用
`/** ... */` 加 `@brief`，不用尾随标记 `/**<` 前置；字段顺序、类型、枚举值和逗号不变。

### T-05 全局/静态状态

```c
/* 主循环起跑准备时清零；主循环或 TIM6 完整采集成功后递增，单位为帧。 */
static uint32_t s_sequence;
```

同样适用于静态局部量。多个对象仅在真实共享用途/上下文时合并说明，不能用一条
“全部变量保持原样”覆盖用途不同的状态。

### T-06 来源和休眠说明

来源/版权块原文独立保留；位置和指纹见逐模块清单。不能推断新作者/许可，不能把普通旧注释
全部作为格式例外。休眠函数采用同一函数模板，在 note 写明未接入状态、出处及真实保留原因；
空壳可写“未接入的既有保留入口，处理策略仍由元素规约决定”，不擅自启用或删除。

## 三个真实函数完整成品

### S-01 App_TryStart：起跑事务与失败副作用

私有函数不新增 H 声明。进入门禁后 PWM 为零不代表所有失败均无副作用；采样、已完成的参数
保存和日志区操作不能在注释中虚构为自动回滚。

```c
/**
 * @brief 检查停车起跑条件，并在全部通过后启用定时运行链。
 * @details 依次隔离在跑/待收尾状态，切断电机，检查标定与新鲜双传感器结果，
 *     持久化参数并建立日志会话；成功后复位运行历史、发布首快照，再启用标志和定时链。
 *     校验不能跳过灰度或电感中的任一套，避免所选模式有效而另一套保护失效时仍起跑。
 * @return 成功启用返回 1；非停车或任一门禁失败返回 0，并输出该门禁已有的拒绝信息
 *     （非停车分支无打印）。失败不启动定时链，不回滚已经完成的采样、参数保存或日志区操作。
 * @pre 由停车主循环的启动事件路径调用，初始化已经完成；ISR 不调用本函数。
 * @note 无显式形参；读取启动/待收尾标志、传感器、参数和日志状态，成功后更新运行历史、
 *     启动标志、按键武装及看门狗状态。含 UART/Flash 阻塞，非停车早退不改变在跑输出；
 *     真正进入门禁后电机保持零。标定配置仍需用户实测填写，本文不提供有效示例值。
 */
static uint8_t App_TryStart(void)
{
    /* 先隔离在跑/待收尾状态；真正进入门禁后立即切断电机输出，检查期间不驱动车轮。 */
    if (!App_IsStopped())
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
    App_ResetRunState();
    Datalog_PublishControl();

    /* 发布启动标志并要求按键释放，然后启动控制/辅助/采样链；首次启用 IWDG，以后只喂狗。 */
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
```

### S-02 Gray_ComputeError：公式、符号和有效性

说明质心的物理方向约定和整数标度，不虚构门限的实测来源，也不把 error=0 当成有线。

```c
/**
 * @brief 将 16 路归一化强度转换为带有效标志的循迹偏差。
 * @details 通道 0 对应最左探头，通道 15 对应最右探头，位置权重为 15-2*i。
 *     对强度做质心加权后归一化为有符号百分标度：左侧信号偏强为正，右侧偏强为负。
 *     有线判断单独使用最大强度与总和，防止无信号时返回的零偏差被误认为居中。
 * @param[in] strength 非空 16 元素数组，每路为 0..1000 的归一化强度，无物理量单位。
 * @param[out] line_valid 非空指针，每次调用写入；最大强度 >=200 且总和非零时为 1，否则 0。
 * @return 偏差 -100..100；总和为零返回 0。结果必须与 line_valid 一起使用。
 * @pre 调用方提供有效数组和输出指针；本函数不校验空指针或额外钳位越界强度。
 * @note 只做本地计算并写 line_valid，不修改模块状态、不阻塞；归一化流程的主循环或
 *     TIM6 调用路径均可使用。200 是既有判定门限，本轮不声称它来自新的实测整定。
 */
int16_t Gray_ComputeError(const uint16_t *strength, uint8_t *line_valid)
{
    int32_t sum = 0;
    int32_t weighted = 0;
    uint16_t maximum = 0;
    uint8_t i;

    /* 同时累计总强度、位置加权和及最大单路强度，后者独立决定是否达到有线门限。 */
    for (i = 0; i < 16; ++i)
    {
        sum += strength[i];
        weighted += (15 - 2 * (int32_t)i) * strength[i];
        if (strength[i] > maximum)
        {
            maximum = strength[i];
        }
    }

    /* 最大值达到 200 且总强度非零才判有线；偏差为零本身不能区分居中与完全无信号。 */
    *line_valid = (maximum >= 200 && sum != 0);

    /* 权重从左 +15 到右 -15；除以 15 倍总强度归一化到 ±100，整数除法向零截断。 */
    return sum ? (int16_t)(100 * weighted / (15 * sum)) : 0;
}
```

### S-03 Datalog_Service：生产/消费并发与写失败

此函数既服务运行记录，也被停产后的收尾路径调用；不足八条是否可写取决于生产状态，
不能仅用车辆“停车”二字代替 active 的真实含义。H 摘要应注明前台唯一消费者、可阻塞及实现位置。

```c
/**
 * @brief 将环形队列中的至多八条记录写入当前日志会话。
 * @details 前台作为唯一消费者，先固定 tail，检查会话有效标志和写失败锁存，再计算待写数量；
 *     记录仍启用时不足八条暂不写，停止生产后允许短尾包。复制记录期间和 Flash 阻塞期间
 *     都不推进 tail，保证 ISR 生产者不会复用本批槽；成功后才发布消费完成。
 * @pre 由主循环单消费者路径调用，不与另一个消费者或日志初始化/导出并发；ISR 不调用。
 * @note 无显式形参和返回值；读取 s_head、s_tail、s_buf、会话有效/生产/故障状态，
 *     写 s_batch、s_status.written 和 s_tail。Flash 可阻塞，并可被定时中断打断。
 *     写失败时置 s_write_failed，锁存故障并请求安全停车，不消费本批且后续不重试；
 *     不保证已发出的 Flash 写入可回滚，也不把计数与 tail 声称为原子整包更新。
 */
void Datalog_Service(void)
{
    uint32_t pending;
    uint32_t n;

    /* 固定本批的消费起点；Flash 完成前不释放这些槽，生产者只能继续使用其他空槽。 */
    uint16_t tail = s_tail;

    /* 没有有效会话或此前写失败时不再发起写入，避免无头记录和失败重试。 */
    if (s_write_failed || !s_header_valid)
    {
        return;
    }

    /* 运行中累计到八条再写，降低服务频度；停止生产后允许不足八条的尾包。 */
    pending = ((uint32_t)s_head + DATALOG_BUFFER_RECORDS - tail) % DATALOG_BUFFER_RECORDS;
    if (!pending || (s_status.active && pending < DATALOG_BATCH_RECORDS))
    {
        return;
    }
    n = pending < DATALOG_BATCH_RECORDS ? pending : DATALOG_BATCH_RECORDS;
    BSP_Time_MemoryBarrier();

    /* 复制为连续批次，一次最多八条；不移动 tail，因此被中断时本批记录仍被占用。 */
    for (uint32_t i = 0; i < n; ++i)
    {
        memcpy(s_batch + i * 96, &s_buf[(tail + i) % DATALOG_BUFFER_RECORDS], 96);
    }

    /* 会话头占前 256 字节，每条 96 字节；页/扇区拆写由 BSP 完成，失败锁存并请求停车。 */
    if (BSP_Flash_LogWrite(256 + s_status.written * 96, s_batch, n * 96))
    {
        s_write_failed = 1;
        Datalog_RequestFaultStop(
            DATALOG_FAULT_WRITE, APP_STOP_LOG_WRITE, TRACKING_PROTECT_LOG_WRITE);
        return;
    }

    /* 写成功才累计落盘数，并在屏障后发布 tail；失败可能已部分写入，不能承诺 Flash 回滚。 */
    s_status.written += n;
    BSP_Time_MemoryBarrier();
    s_tail = (uint16_t)((tail + n) % DATALOG_BUFFER_RECORDS);
}
```

## 成品验收边界

- 三段 C 成品的非注释 token 必须与基线定义一致，字符串原样，不能借示例改变流程。
- 文件/函数模板格式、字段顺序和行宽可自动检查，原因是否准确必须结合源码和已确认规约审查。
- 头文件只摘要不等于可以遗漏调用者必须知道的非空、ISR、阻塞、停机或所有权约束。
- 已知未知事实保持诚实：真实标定仍 -1，灰度/控制时序和实车验收未重新执行。
