set(SRCS "")
list(APPEND SRCS
    "src/board_camera.c"
    "src/board_codec.c"
    "src/board_audio.c"
    "src/board_display.c"
    "src/board_storage.c"
    "src/board_wifi.c"
    "src/board_peripherals.c"
    "src/board_leds.c"
    "src/board_rtc.c"
    "src/board_ble.c"
    "src/board_button.c"
    "src/board_usb.c"
)

set(INCLUDE_DIRS "include")

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES
        esp_board_manager
        esp_driver_gpio
        esp_driver_i2s
        esp_driver_twai
        esp_driver_uart
        esp_lcd
        esp_lcd_touch
        esp_lvgl_adapter
    PRIV_REQUIRES
        bt
        esp_driver_i2c
        esp_driver_ledc
        esp_mm
        esp_netif
        esp_psram
        esp_timer
        esp_wifi
        fatfs
        nvs_flash
        usb
        waveshare__pcf85063a
)

target_compile_options(
    ${COMPONENT_LIB} PRIVATE
        -Wno-format
        -Wno-int-conversion
        -Wno-incompatible-pointer-types
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-overflow
        -Wno-unused-but-set-variable
        -Wno-discarded-qualifiers)
