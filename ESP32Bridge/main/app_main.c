#include <stddef.h>

#include "esp_log.h"
#include "sdkconfig.h"

#if !CONFIG_IDF_TARGET_ESP32S3
#error "ESP32Bridge requires esp32s3"
#endif
#if defined(CONFIG_SPIRAM) || defined(CONFIG_FREERTOS_UNICORE)
#error "ESP32Bridge requires internal memory and both CPU cores"
#endif

/* Group 1 performs a real, fail-closed configuration check. Service startup and
 * resource ownership are delivered separately by Phase 01 Group 2. */
void app_main(void)
{
    static const char tag[] = "foundation";
    static const unsigned char password[] = CONFIG_BRIDGE_AP_PASSWORD;
    const size_t length = sizeof(password) - 1U;

    if (length < 8U || length > 63U) {
        ESP_LOGE(tag, "Invalid local AP configuration; services remain unavailable");
        return;
    }
    for (size_t i = 0; i < length; ++i) {
        if (password[i] == 0U || password[i] > 127U) {
            ESP_LOGE(tag, "Invalid local AP configuration; services remain unavailable");
            return;
        }
    }

    ESP_LOGI(tag, "Configuration validated; Phase 01 Group 2 startup/resources pending");
    ESP_LOGW(tag, "AP, sockets and SPI services are unavailable in this Group 1 image");
}
