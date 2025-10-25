idf_component_register(
    SRCS 
        "main.c"
        "display_ui.c"
        "cst816_touch.c"
    INCLUDE_DIRS "."
    REQUIRES 
        nvs_flash
        bt
        driver
        spi_flash
        esp_lcd
        lvgl
        lvgl_esp32_drivers
)
