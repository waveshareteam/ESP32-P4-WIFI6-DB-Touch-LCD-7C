idf_component_register(
    SRCS main.c
    INCLUDE_DIRS .
    REQUIRES gpio_monitor esp_board_manager esp_lvgl_adapter esp_driver_gpio esp_lcd_touch)
