from pathlib import Path
import re

root = Path(__file__).resolve().parents[3]

def edit(name, replacements):
    p = root / name
    data = p.read_bytes().decode('utf-8')
    for old, new in replacements:
        assert old in data, (name, old)
        data = data.replace(old, new.replace('\n', '\r\n'))
    p.write_bytes(data.encode('utf-8'))

edit('BSP/bsp_dl1b.c', [
('通过 GPIO 时序驱动 DL1B 灯条，保存显示缓存并提供单灯、整屏和清屏操作。',
 '前台初始化通过 XS 复位及 I2C2 配置测距器；TIM6 控制链阻塞读取状态和距离。\n *     模块缓存毫米值或 BSP_DL1B_INVALID，读取缓存不发起总线访问。'),
('寄存器地址，使用调用方指定的原始字节或 16 位地址。', '16 位寄存器地址。'),
('数据缓冲，必须非空且具有传输所需容量；HAL 调用期间读取或写入。', '数据缓冲，必须非空且至少具有 len 字节容量。'),
('@param[in,out] buf', '@param[out] buf'),
('传输字节数，保持原参数类型范围。', '传输字节数，由模块内调用方提供。'),
('保持原调度入口与阻塞传输顺序。', '使用阻塞 HAL I2C 接口，传入 10 ms 超时；实际退出依赖 HAL 时基推进。'),
('按现有调用顺序使用 HAL_GPIO_WritePin、HAL_Delay、BSP_DL1B_ReadRegister、BSP_DL1B_WriteRegister\r\n *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。',
 'XS 高 50 ms、低 10 ms、高 50 ms 后检查固件状态 bit0，写入 135 字节配置，\n *     轮询状态 bit0 清零；I2C 失败或超过轮询计数返回失败。成功才置初始化标志并清缓存。'),
('按现有调用顺序使用 BSP_DL1B_ReadRegister、BSP_DL1B_WriteRegister 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。',
 '未初始化时保留缓存；状态字节非零时尝试清中断（忽略写入结果），再要求量程状态\n *     等于 0x89。两字节距离组合后转 int16_t，仅 0～4000 保存为毫米缓存，其余发布无效值。'),
('TIM7 调用且有 I2C 阻塞；未初始化直接返回，未就绪或读取失败发布 INVALID。',
 'App_ControlISR 的 TIM6 控制链调用，存在 I2C 阻塞；HAL 时基不推进时不能保证超时退出。'),
('读取最近一次 DL1B 测距结果并换算为毫米返回；本函数不触发新的总线测量。',
 '直接返回已按毫米保存的缓存；不进行单位换算，也不触发新的总线测量。'),
])
edit('BSP/bsp_dl1b.h', [
('更新由 TIM7 调用。', '更新由 TIM6 控制链调用。'),
('声明 DL1B 初始化与显示接口；完整位时序和缓存行为位于 bsp_dl1b.c。',
 '声明 I2C 测距初始化、更新及缓存读取接口；完整契约位于 bsp_dl1b.c。'),
('TIM7 调用且有 I2C 阻塞；未初始化直接返回，未就绪或读取失败发布 INVALID。 完整契约见 bsp_dl1b.c::BSP_DL1B_Update。',
 'TIM6 控制链中执行阻塞 I2C 访问；完整契约见 bsp_dl1b.c::BSP_DL1B_Update。'),
])
edit('BSP/bsp_adc.c', [
('接管 ADC1 DMA 扫描，提供电感缓存读取和灰度 ADC3 单次转换；保持 D2 SRAM 与超时契约。',
 'ADC1 的十个扫描结果由循环 DMA 写入 D2 SRAM1，读取时右移四位到 12 位标度。\n *     PC4 灰度位于 rank5；前台或 TIM6 可观察 NDTR 等待新样本，失败不写输出。'),
('按现有调用顺序使用 HAL_ADCEx_Calibration_Start、HAL_ADC_Start_DMA 完成本函数处理。',
 '先执行单端偏移校准，再把模块缓冲交给 ADC1 循环 DMA；两个 HAL 返回值均未检查。'),
('保持原有 HAL 返回值忽略行为。', '不返回校准或 DMA 启动失败状态。'),
('TIM6 电感采样读取 DMA 正在循环更新的单个半字；不锁定整组快照。',
 '前台菜单、电池读取及 TIM6 电感采样可调用；读取单个半字，不锁定整组快照。'),
('按现有调用顺序使用\r\n *     BSP_Time_GetCycles、__HAL_DMA_GET_FLAG、__HAL_DMA_GET_TE_FLAG_INDEX、\r\n *     __HAL_DMA_GET_DME_FLAG_INDEX\r\n *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。',
 '先检查输出指针和 DMA 句柄，再在整帧 DWT 期限内轮询。NDTR 大于 5 时记录\n *     rank5 写入前窗口，随后小于等于 5 才读取缓存；无 DMA 使能、错误标志或异常计数均失败。'),
])
edit('BSP/bsp_adc.h', [
('灰度单次转换接口', '灰度新 DMA 样本读取接口'),
('rank0=INP10(PC0) rank1=INP11(PC1) rank2=INP18(PA4) rank3=INP8(PC5)',
 'rank1=INP10(PC0) rank2=INP11(PC1) rank3=INP18(PA4) rank4=INP8(PC5)'),
('rank4=INP4(PC4)  rank5=INP14(PA2) rank6=INP15(PA3) rank7=INP9(PB0)',
 'rank5=INP4(PC4)  rank6=INP14(PA2) rank7=INP15(PA3) rank8=INP9(PB0)'),
('rank8=INP5(PB1)  rank9=INP19(PA5)', 'rank9=INP5(PB1)  rank10=INP19(PA5)'),
('灰度整帧采集完成，输出缓冲区已更新。', '本次样本或整帧读取成功，输出已写入。'),
('输出指针为空，未启动扫描。', '输出指针为空，本次调用未写输出。'),
('等待 DMA 完成超过单次采集时限。', '达到灰度整帧共用的 DWT 预算。'),
('HAL ADC DMA 启动或停止失败。', 'DMA 句柄、使能、错误标志或计数异常。'),
('保持原有 HAL 返回值忽略行为。', '不返回校准或 DMA 启动失败状态。'),
('value 成功时写入 0～4095；空指针返回 BSP_GRAY_ARG，失败不写输出。 主循环或 TIM6 有界等待；先观察 rank5 写入前窗口，再等写入后窗口。',
 '主循环或 TIM6 使用整帧 DWT 预算等待；仅成功时写 value。'),
])
edit('BSP/bsp_key.c', [
('采样两路 ADC 分压按键，执行去抖、释放武装和单次事件发布；仅供停车前台轮询。',
 '轮询 PC13 高有效 GPIO，在前台执行双沿消抖、释放武装及短按/长按事件识别。\n *     状态由本模块持有；ADC 分压菜单按键不经过本状态机。'),
('按现有调用顺序使用 HAL_GPIO_ReadPin、HAL_GetTick 完成本函数处理。',
 '记录当前原始电平和时刻，把稳定电平置为按下并清武装、按下及长按标志；\n *     后续必须观察到稳定释放才允许新事件。'),
('前台起停流程调用；保留当前电平读取和标志写入次序。', '前台起停流程调用；不立即产生按键事件。'),
('按现有调用顺序使用 HAL_GetTick、HAL_GPIO_ReadPin 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。',
 '原始电平保持 30 ms 才更新稳定状态；稳定释放后武装，已武装的稳定按下开始计时。\n *     短按在释放时报告 START；持续按下满 1000 ms 报告一次 TOGGLE，随后释放不报告 START。'),
('先根据释放门禁和当前 ADC 电平完成去抖；只有稳定按下且已武装时发布一次事件，持续按住不重复触发。',
 '先对 GPIO 电平消抖并推进释放门禁；短按松开产生 START，长按达到阈值产生一次 TOGGLE。'),
])
edit('BSP/bsp_key.h', [('前台起停流程调用；保留当前电平读取和标志写入次序。', '前台起停流程调用；必须稳定释放后才允许新事件。')])
edit('BSP/bsp_pwm.c', [
('把万分比占空比和方向转换为定时器比较值与方向 GPIO，并提供风扇输出。',
 '启动三个 PWM 通道，将无符号万分比占空比转换为 CCR；不保存应用控制状态。\n *     电机方向 GPIO 由 App 的 Motor_UpdateControl 处理，本模块不设置方向。'),
('启动左右电机和风扇 PWM 通道，并把方向与比较值置为既定初始状态；不启动应用控制环。',
 '依次启动 TIM13_CH1、TIM4_CH3、TIM14_CH1，忽略 HAL 启动返回值；\n *     使用定时器既有比较值，不写方向 GPIO，不启动应用控制环。'),
('上电前台调用；写定时器和方向 GPIO，不改变应用运行标志。', '上电前台调用；不返回 PWM 启动失败状态。'),
('按现有调用顺序使用 __HAL_TIM_GET_AUTORELOAD、__HAL_TIM_SET_COMPARE 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。',
 '拒绝达到通道数的索引，将 duty 截至 10000，再以 duty*(ARR+1)/10000\n *     计算比较值并写入所选通道；不判断正反转。'),
('控制 ISR、停车路径及前台可调用；乘除与寄存器写入顺序保持。',
 '控制 ISR、停车路径及前台可调用；调用方提供有效通道，方向由电机控制层设置。'),
('先把万分比限制到 0..10000，再按通道查表换算 CCR；电机通道同时依据符号设置方向，风扇只写占空比。',
 '检查通道并限制占空比，再按所选定时器 ARR 换算 CCR；三个通道均只更新占空比。'),
])
edit('BSP/bsp_pwm.h', [
('完整限幅、方向及 CCR 换算位于 bsp_pwm.c。', '完整限幅及 CCR 换算位于 bsp_pwm.c，方向由电机控制层设置。'),
('17kHz（ARR=7059）', '约 17kHz（ARR=7058）'),
('TIM1 左电机 PWM 通道。', 'TIM13_CH1 左电机 PWM 通道。'),
('TIM8 右电机 PWM 通道。', 'TIM4_CH3 右电机 PWM 通道。'),
('duty 占空比 0～10000，超出上限时截为 10000。 控制 ISR、停车路径及前台可调用；乘除与寄存器写入顺序保持。 完整契约见',
 'duty 为无符号万分比，超过 10000 截断；不设置方向。完整契约见'),
])
edit('App/pid.c', [
('控制状态由 TIM6 独占更新。', '活动控制在 TIM6 更新，前台负责参数设置和起跑/停车复位。'),
('按原顺序更新四项方向增益。', '更新四项方向增益，保留控制历史。'),
('按原顺序更新三项角度增益。', '更新三项角度增益，保留控制历史。'),
('先计算当前与上一拍偏差的差分，再叠加比例、微分和立方非线性项；结果按 int16_t 返回，并在末尾保存本拍偏差。',
 '依次计算 kp*e、k2p*e*abs(e)、kd*(e-前拍偏差) 和 k2d*accel。\n *     非线性项为带符号二次项；先保存本拍偏差，再将前三项之和减阻尼项转换为 int16_t 并限幅。'),
('方向输出，-1000..1000。', '转换有效时的方向输出，-1000..1000；越界浮点转换不保证数学饱和结果。'),
('浮点求和先转换 int16_t，再限幅 ±1000，不调整顺序。',
 '浮点求和先转换 int16_t，再限幅 ±1000；大值存在转换风险，尚未实施 R01 修复。'),
('按现有调用顺序使用 PID_DirectionStep、Track_GetState、IMU_GetState 完成本函数处理。',
 '从模块参数构造方向增益，读取电磁偏差和 IMU X 轴加速度，调用方向计算并更新共享历史。'),
('以当前角度误差和上一拍误差计算比例、微分输出，更新历史后把结果限制到既有方向输出范围。',
 '按目标角度减当前 X 轴角度得到误差，计算比例加拍间差分再减角速度阻尼。\n *     保存误差历史后，把浮点输出取负传给 Motor_DistributeSpeed；本函数没有方向限幅。'),
('保持角误差、浮点输出、历史再差速顺序；旧入口保留。', '保留的角度控制入口；浮点输出向差速接口整数形参转换存在范围约束。'),
('按现有调用顺序使用 Motor_GetState 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。',
 '读取该轮目标和反馈，累计 kp*(本拍误差-前拍误差)+ki*本拍误差；\n *     更新前拍误差后把累计值限制到 ±7500，微分占位保持零。'),
('按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。',
 '目标跨零换向时清积分，先保存目标历史，再计算误差并累加积分；积分限制为\n *     ±(7500-dead_zone)。比例加积分后按目标符号加入死区，输出限制到 ±7500。'),
('先按目标与反馈计算误差并累加积分，再对积分和比例积分和分别限幅；最后更新历史和输出。',
 '先处理目标换向并更新历史，再计算 PI、限制积分、补偿死区，最后限幅并返回输出。'),
('按现有调用顺序使用 Motor_GetState、PID_UpdateSpeed 完成本函数处理。',
 '读取该轮目标及反馈，以该轮独立积分/目标历史执行位置式 PI，并写入对应输出缓存。'),
('按现有调用顺序使用 PID_ResetSpeed 完成本函数处理。',
 '清共享电磁方向历史、角度历史及角度输出，再清双轮活动 PI 的积分、目标历史和输出。'),
])
# ResetSpeed is not a PI update: replace its duplicated old template separately.
p = root/'App/pid.c'
data = p.read_bytes().decode('utf-8')
start = data.index(' * @brief 清两个活动 PI')
end = data.index(' * @note', start)
data = data[:start] + (' * @brief 清两个活动 PI 的积分、目标历史和输出。\r\n'
    ' * @details 分别清左右积分、前拍目标和 out_l/out_r，不修改增益、方向或角度历史。\r\n') + data[end:]
p.write_bytes(data.encode('utf-8'))
edit('App/pid.h', [
('按原顺序更新四项方向增益。', '更新四项方向增益，保留控制历史。'),
('按原顺序更新三项角度增益。', '更新三项角度增益，保留控制历史。'),
('gains 非空四项增益。 history 非空独立前拍偏差，计算差分后更新。 TIM6 调用；浮点求和先转换 int16_t，再限幅 ±1000，不调整顺序。 完整契约见',
 'gains、history 必须非空；先转换 int16_t 再限幅，越界转换风险尚存。完整契约见'),
('保持角误差、浮点输出、历史再差速顺序；旧入口保留。', '保留角度入口，本函数不作方向限幅。'),
])

# Wrap only comment text, retaining all non-comment bytes and source line endings.
for name in ['BSP/bsp_'+x+e for x in ('adc','dl1b','key','pwm') for e in ('.c','.h')] + ['App/pid.c','App/pid.h']:
    p = root/name
    text = p.read_bytes().decode('utf-8')
    lines = []
    for line in text.split('\r\n'):
        if line.startswith(' * ') and len(line) > 100:
            while len(line) > 100:
                cut = line.rfind(' ', 30, 96)
                if cut < 30: cut = 94
                lines.append(line[:cut])
                line = ' *     ' + line[cut:].lstrip()
        lines.append(line)
    p.write_bytes('\r\n'.join(lines).encode('utf-8'))
print('updated ten C/H files; no implementation edits intended')
