/**
 * @file gray_sensor.h
 * @brief App 标定、归一化与质心；采集仍由 BSP 完整帧接口提供。
 */
#ifndef SMARTCAR_APP_GRAY_SENSOR_H
#define SMARTCAR_APP_GRAY_SENSOR_H
#include <stdint.h>

typedef struct
{
    uint16_t raw[16];           /**< 十六路完整帧均值，ADC 0..4095，ch0 最左。 */
    uint16_t percent[16];       /**< raw*100/4095 的整数百分比，0..100。 */
    uint16_t strength[16];      /**< 手工黑白标定后强度，0..1000。 */
    int16_t error;              /**< 质心偏差，通常 -100..100；总强度为零时为 0。 */
    uint32_t sequence;          /**< 成功完整帧序号；采集失败保留上一帧。 */
    uint32_t scan_cycles;       /**< 成功帧的 DWT 扫描周期数，失败时保留。 */
    uint8_t calibration_valid;  /**< 全部十六路标定有效为 1，未标定为 0。 */
    uint8_t line_valid;         /**< 最大强度 >=200 且总强度非零、标定有效时为 1。 */
    uint8_t acquisition_status; /**< 最近 BSP 采集状态值，失败也更新；其他帧数据不发布。 */
} gray_state_t;

/**
 * @brief 清模块状态，初始化 BSP 并报告标定。
 * @note 启动前前台调用；保留 memset、BSP 初始化、标定报告顺序。
 */
void Gray_Init(void);

/**
 * @brief 复位 BSP 和 App 完整帧序号。
 * @note 停车起跑准备调用，不清其他帧字段。
 */
void Gray_ResetSequence(void);

/**
 * @brief 读取 BSP 整帧并发布标定和质心。
 * @note TIM6 调用；失败仅更新采集状态和标定标志，保留上一整帧及有效线标志。
 */
void Gray_Update(void);

/**
 * @brief 读取模块持有的只读运行状态。
 * @return 非空只读指针，不转移所有权。
 * @note 静态对象寿命为整个程序；控制拍可更新，前台一致性读取须遵守既有停车约定。
 */
const gray_state_t *Gray_GetState(void);

/**
 * @brief 逐路检查黑白标定范围和绝对差。
 * @param[in] black 非空十六元素黑底数组，ADC 0..4095。
 * @param[in] white 非空十六元素白线数组，ADC 0..4095。
 * @return 位 i 为 1 表示该路无效，0 表示全部有效。
 * @note 纯计算，无阻塞；差绝对值必须 >=200。
 */
uint16_t Gray_ValidateCalibration(const int16_t *black, const int16_t *white);

/**
 * @brief 更新全部标定有效标志，可逐路报告错误。
 * @param[in] report 0 静默，非零通过 UART 报告每条无效通道。
 * @return 全部有效为 1，否则为 0。
 * @note 报告路径为启动前前台阻塞输出；控制拍仅传 0。
 */
uint8_t Gray_CheckCalibration(uint8_t report);

/**
 * @brief 按原整数顺序将原始值归一化。
 * @param[in] raw ADC 原始均值，0..4095。
 * @param[in] black 本路黑底实测 ADC 值。
 * @param[in] white 本路白线实测 ADC 值。
 * @return 强度 0..1000。
 * @note 标定无效返回 0；q<50 清零，q>1000 截顶，保留极性和整数截断。
 */
uint16_t Gray_Normalize(uint16_t raw, int16_t black, int16_t white);

/**
 * @brief 按十六路强度计算整数质心。
 * @param[in] strength 非空十六元素数组，强度 0..1000。
 * @param[out] line_valid 非空指针，最大值 >=200 且总和非零时写 1。
 * @return 通常 -100..100；总和为零返回 0。
 * @note 权重依次 15,13,...,-15；不加入额外滤波或迟滞。
 */
int16_t Gray_ComputeError(const uint16_t *strength, uint8_t *line_valid);

/**
 * @brief 读取十六路黑底实测标定表。
 * @return 非空十六元素指针，ADC 0..4095 或未标定 -1。
 * @note 静态只读数组，ch0 最左、ch15 最右；-1 表示未填写，不提供示例值。
 */
const int16_t *Gray_GetBlack(void);

/**
 * @brief 读取十六路白线实测标定表。
 * @return 非空十六元素指针，ADC 0..4095 或未标定 -1。
 * @note 静态只读数组，ch0 最左、ch15 最右；-1 表示未填写，不提供示例值。
 */
const int16_t *Gray_GetWhite(void);

#endif
