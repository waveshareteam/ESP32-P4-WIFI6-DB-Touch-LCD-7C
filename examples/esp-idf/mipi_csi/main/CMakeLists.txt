idf_component_register(SRCS "main.c"
                       PRIV_INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_board_manager esp_video_capture_service
                                     esp_video_render gmf_video esp_timer esp_lvgl_port esp_lcd)
