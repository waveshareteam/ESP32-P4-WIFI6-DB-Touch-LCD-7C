idf_component_register(
    SRCS "twai_listen_only.c"
    REQUIRES esp_driver_twai
    PRIV_REQUIRES gen_bmgr_codes
    INCLUDE_DIRS "."
)
