/**
 * @file startup.c
 * @brief 启动依赖协调与密码校验。
 * @details 输入回调表及保护结果，逐步执行并输出启动报告；startup_idf提供实际依赖。
 *     无共享运行期状态；由启动任务串行调用，不访问ISR或业务传输。
 */
#include "startup.h"
#include <stdbool.h>

/**
 * @brief 校验本地热点密码的显式字节范围。
 * @details 逐字节拒绝NUL和非ASCII；不保存、打印或修改密码。
 * @param[in] data 密码字节；不能为空，仅在调用期间借用。
 * @param[in] length 字节数，允许8至63。
 * @return 合法返回ESP_OK，否则ESP_ERR_INVALID_ARG。
 */
esp_err_t bridge_password_validate(const unsigned char *data, size_t length)
{
    if (data == NULL || length < 8 || length > 63)
    {
        return ESP_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i < length; ++i)
    {
        if (data[i] == 0 || data[i] > 127)
        {
            return ESP_ERR_INVALID_ARG;
        }
    }
    return ESP_OK;
}

/**
 * @brief 按依赖执行启动步骤并记录独立服务结果。
 * @details 先初始化报告，再按NVS、netif、事件循环、AP、socket、SPI顺序尝试。池失败阻塞全部步骤，网络依赖失败不阻塞SPI；缺回调记未实现。保留首个实际错误，存在未完成步
 *     骤时不报告全成功。
 * @param[in] ops 不能为空；调用期间借用回调表及context，回调不得使表失效。
 * @param[out] report 不能为空；参数无效时不写，其他路径填充逐步状态和依赖错误。
 * @return 返回首个READY/密码/步骤错误；否则池错误、ESP_ERR_NOT_SUPPORTED或ESP_OK。
 */
esp_err_t bridge_startup_run(const bridge_startup_ops_t *ops, bridge_startup_report_t *report)
{
    if (ops == NULL || report == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *report = (bridge_startup_report_t)
    {
        .ready_error = ops->ready_error,
        .password_error = ops->password_error,
        .pool_error = ops->pool_error
    };
    const bridge_start_step_fn callbacks[BRIDGE_START_COUNT]
        = {ops->nvs, ops->netif, ops->event_loop, ops->ap, ops->sockets, ops->spi};
    const int dependencies[BRIDGE_START_COUNT] =
    {
        BRIDGE_DEP_NONE, BRIDGE_START_NVS,
        BRIDGE_START_NETIF, BRIDGE_START_EVENT_LOOP, BRIDGE_START_AP, BRIDGE_DEP_NONE
    };
    esp_err_t first_error = ops->ready_error != ESP_OK ? ops->ready_error : ops->password_error;
    bool incomplete = false;
    for (int i = 0; i < BRIDGE_START_COUNT; ++i)
    {
        bridge_start_result_t *step = &report->steps[i];
        step->dependency = BRIDGE_DEP_NONE;
        if (ops->pool_error != ESP_OK)
        {
            *step = (bridge_start_result_t){BRIDGE_BLOCKED, ops->pool_error, BRIDGE_DEP_POOL};
        }
        else if (dependencies[i] >= 0 && report->steps[dependencies[i]].state != BRIDGE_SUCCEEDED)
        {
            *step = (bridge_start_result_t)
            {
                BRIDGE_BLOCKED, report->steps[dependencies[i]].error, dependencies[i]
            };
        }
        else if (i == BRIDGE_START_AP && ops->password_error != ESP_OK)
        {
            *step
                = (bridge_start_result_t){BRIDGE_BLOCKED, ops->password_error, BRIDGE_DEP_PASSWORD};
        }
        else if (i == BRIDGE_START_SPI && ops->ready_error != ESP_OK)
        {
            *step = (bridge_start_result_t){BRIDGE_BLOCKED, ops->ready_error, BRIDGE_DEP_READY};
        }
        else if (callbacks[i] == NULL)
        {
            *step = (bridge_start_result_t)
            {
                BRIDGE_UNIMPLEMENTED, ESP_ERR_NOT_SUPPORTED, BRIDGE_DEP_NONE
            };
        }
        else
        {
            step->error = callbacks[i](ops->context);
            step->state = step->error == ESP_OK ? BRIDGE_SUCCEEDED : BRIDGE_FAILED;
            if (first_error == ESP_OK && step->error != ESP_OK)
            {
                first_error = step->error;
            }
        }
        incomplete = incomplete || step->state != BRIDGE_SUCCEEDED;
    }
    if (ops->pool_error != ESP_OK)
    {
        return first_error != ESP_OK ? first_error : ops->pool_error;
    }
    return first_error != ESP_OK ? first_error : (incomplete ? ESP_ERR_NOT_SUPPORTED : ESP_OK);
}
