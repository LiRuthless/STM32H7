idf_component_register(
    SRCS "app_main.c" "buffer_pool.c" "startup.c" "startup_idf.c" "protocol.c" "json_guard.c"
         "reliable_link.c" "link_session.c"
         "spi_bridge.c" "spi_protocol.c" "net_bridge.c" "bridge_state.c" "bridge_stats.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES log esp_system nvs_flash esp_netif esp_event esp_wifi
                  lwip esp_driver_spi esp_driver_gpio freertos heap esp_hw_support esp_timer
)
