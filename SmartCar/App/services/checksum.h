/**
 * @file checksum.h
 * @brief 可续算CRC32接口。
 * @details 实现见checksum.c；初次previous传0，后续传上次结果。
 */
#ifndef SMARTCAR_APP_SERVICES_CHECKSUM_H
#define SMARTCAR_APP_SERVICES_CHECKSUM_H
#include <stdint.h>
#include <stddef.h>

/**
 * @brief 按反射多项式续算 zlib 兼容CRC。
 * @note 完整契约见 checksum.c::Checksum_UpdateCrc32。
 */
uint32_t Checksum_UpdateCrc32(uint32_t previous, const uint8_t *data, size_t length);

#endif /* SMARTCAR_APP_SERVICES_CHECKSUM_H */
