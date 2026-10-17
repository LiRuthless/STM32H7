idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES log esp_system nvs_flash esp_netif esp_event esp_wifi
                  lwip esp_driver_spi esp_driver_gpio freertos heap
)
