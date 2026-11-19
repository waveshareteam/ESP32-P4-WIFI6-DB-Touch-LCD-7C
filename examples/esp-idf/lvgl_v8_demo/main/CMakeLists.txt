idf_component_register(SRCS "lv_adapter_project.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        gen_bmgr_codes
        esp_board_manager
        esp_lvgl_adapter
        lvgl)
