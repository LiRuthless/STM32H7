/**
 * @file log_store.h
 * @brief 日志Flash字节后端接口。
 * @details 实现见log_store.c；调用者负责会话状态、地址公式及错误处理。
 */
#ifndef SMARTCAR_APP_SERVICES_LOG_STORE_H
#define SMARTCAR_APP_SERVICES_LOG_STORE_H
#include "services/datalog.h"

/**
 * @brief 擦除日志首扇区并复位底层懒擦前沿。
 * @note 完整契约见 log_store.c::LogStore_Erase。
 */
uint8_t LogStore_Erase(void);

/**
 * @brief 写头后回读并逐字节核对。
 * @note 完整契约见 log_store.c::LogStore_WriteHeader。
 */
uint8_t LogStore_WriteHeader(const datalog_header_t *header);

/**
 * @brief 按字节写入日志区。
 * @note 完整契约见 log_store.c::LogStore_Write。
 */
uint8_t LogStore_Write(uint32_t offset, const uint8_t *data, uint32_t length);

/**
 * @brief 按字节读取日志区。
 * @note 完整契约见 log_store.c::LogStore_Read。
 */
uint8_t LogStore_Read(uint32_t offset, uint8_t *data, uint32_t length);

#endif /* SMARTCAR_APP_SERVICES_LOG_STORE_H */
