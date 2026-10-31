/**
 * @file checksum.c
 * @brief 通用可续算反射CRC32。
 * @details 按字节更新zlib兼容CRC32，不依赖协议、日志、硬件或共享状态。
 */
#include "services/checksum.h"

/**
 * @brief 按反射多项式续算 zlib 兼容CRC。
 * @details 每次先撤销前次最终异或，逐位计算后再次异或；初次previous为零。
 * @param[in] previous 前段已完成的CRC，首次为0。
 * @param[in] data 非空数据，length为零时可为空。
 * @param[in] length 字节数。
 * @return 可继续输入下一段的最终CRC。
 */
uint32_t Checksum_UpdateCrc32(uint32_t previous, const uint8_t *data, size_t length)
{
    uint32_t crc = previous ^ 0xffffffffu;
    size_t i;
    uint8_t bit;
    for (i = 0; i < length; ++i)
    {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit)
        {
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
        }
    }
    return crc ^ 0xffffffffu;
}
