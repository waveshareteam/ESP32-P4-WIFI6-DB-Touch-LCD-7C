idf_component_register(SRCS "rs485_example.c"
                    REQUIRES nvs_flash esp_driver_uart
                    PRIV_REQUIRES gen_bmgr_codes
                    INCLUDE_DIRS ".")
