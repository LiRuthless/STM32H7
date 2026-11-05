/**
 * @file startup.h
 * @brief 启动依赖协调与密码校验。
 * @details 声明启动依赖协调与密码校验接口；实现及完整参数/错误语义位于startup.c与startup_idf.c。
 *     调用方按接口约束串行访问owner状态，头文件不定义线图像或运行期服务。
 */
#pragma once
#include <stddef.h>
#include "esp_err.h"

typedef enum
{
    BRIDGE_START_NVS, /**< NVS启动步骤索引。 */
    BRIDGE_START_NETIF, /**< 网络接口启动步骤索引。 */
    BRIDGE_START_EVENT_LOOP, /**< 事件循环启动步骤索引。 */
    BRIDGE_START_AP, /**< 热点启动步骤索引。 */
    BRIDGE_START_SOCKETS, /**< 网络任务启动步骤索引。 */
    BRIDGE_START_SPI, /**< SPI任务启动步骤索引。 */
    BRIDGE_START_COUNT /**< 步骤数哨兵。 */
} bridge_start_step_t;

typedef enum
{
    BRIDGE_NOT_ATTEMPTED, /**< 尚未尝试该步骤。 */
    BRIDGE_SUCCEEDED, /**< 步骤回调成功。 */
    BRIDGE_FAILED, /**< 步骤回调返回错误。 */
    BRIDGE_BLOCKED, /**< 前置条件阻塞，未调用步骤。 */
    BRIDGE_UNIMPLEMENTED /**< 缺失步骤回调，不能记成功。 */
} bridge_start_state_t;

enum
{
    BRIDGE_DEP_NONE = -1, /**< 无阻塞来源。 */
    BRIDGE_DEP_POOL = -2, /**< 固定池失败来源。 */
    BRIDGE_DEP_PASSWORD = -3, /**< 密码校验失败来源。 */
    BRIDGE_DEP_READY = -4 /**< READY保护失败来源。 */
};

typedef struct
{
    bridge_start_state_t state; /**< 该步骤的尝试/成功/失败/阻塞/未实现状态。 */
    esp_err_t error; /**< 实际步骤错误或阻塞来源错误。 */
    int dependency; /**< 阻塞步骤索引或BRIDGE_DEP_*负哨兵。 */
} bridge_start_result_t;

typedef esp_err_t (*bridge_start_step_fn)(void *context);

typedef struct
{
    void *context; /**< 原样借给各步骤的上下文，可为空，须保持至返回。 */
    bridge_start_step_fn nvs; /**< NVS步骤，可为空；空记未实现。 */
    bridge_start_step_fn netif; /**< 网络接口步骤，依赖NVS。 */
    bridge_start_step_fn event_loop; /**< 事件循环步骤，依赖netif。 */
    bridge_start_step_fn ap; /**< 热点步骤，依赖事件循环及密码校验。 */
    bridge_start_step_fn sockets; /**< socket服务步骤，依赖热点。 */
    bridge_start_step_fn spi; /**< SPI步骤，独立于网络链，依赖池及READY保护。 */
    esp_err_t ready_error; /**< 启动前READY保护结果。 */
    esp_err_t password_error; /**< 启动前本地密码校验结果。 */
    esp_err_t pool_error; /**< 启动前固定池初始化结果。 */
} bridge_startup_ops_t;

typedef struct
{
    bridge_start_result_t steps[BRIDGE_START_COUNT]; /**< 按bridge_start_step_t顺序的逐步结果。 */
    esp_err_t ready_error; /**< 启动前READY保护的原结果。 */
    esp_err_t password_error; /**< 启动前密码校验的原结果，不携带密码。 */
    esp_err_t pool_error; /**< 启动前池初始化的原结果。 */
} bridge_startup_report_t;

/**
 * @brief 校验本地热点密码的显式字节范围。
 * @note startup.c::bridge_password_validate；纯密码校验只读输入；启动协调器回调在本次调用期间有效。
 */
esp_err_t bridge_password_validate(const unsigned char *data, size_t length);
/**
 * @brief 按依赖执行启动步骤并记录独立服务结果。
 * @note startup.c::bridge_startup_run；纯密码校验只读输入；启动协调器回调在本次调用期间有效。
 */
esp_err_t bridge_startup_run(const bridge_startup_ops_t *ops, bridge_startup_report_t *report);
/**
 * @brief 将启动协调器绑定到真实IDF资源与服务。
 * @note startup_idf.c::bridge_startup_idf_run；纯密码校验只读输入；启动协调器回调在本次调用期间有效。
 */
esp_err_t bridge_startup_idf_run(bridge_startup_report_t *report);
