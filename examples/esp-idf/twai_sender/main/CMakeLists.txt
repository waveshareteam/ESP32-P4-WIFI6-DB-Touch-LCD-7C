idf_component_register(
    SRCS "twai_sender.c"
    REQUIRES esp_driver_twai esp_timer
    PRIV_REQUIRES gen_bmgr_codes
    INCLUDE_DIRS "."
)
