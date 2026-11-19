# TODO(theme): online theme package and custom PNG/GIF emoji; set to 1 to re-enable.
set(XIAOZHI_THEME_ENABLED 0)

set(SRCS_C
    "assets/img_app_xiaozhi.c"
)
set(SRCS_CPP
    "XiaozhiActivationClient.cpp"
    "XiaozhiActivationPrompt.cpp"
    "XiaozhiApp.cpp"
    "XiaozhiAudioProcessor.cpp"
    "XiaozhiClient.cpp"
    "XiaozhiDeveloperTools.cpp"
    "XiaozhiLocalization.cpp"
    "XiaozhiUi.cpp"
    "XiaozhiWebsocketStack.cpp"
    "third_party/xiaozhi_esp32/ogg_demuxer.cc"
)
if(XIAOZHI_THEME_ENABLED)
    list(APPEND SRCS_C "third_party/xiaozhi_esp32/gif/gifdec.c")
    list(APPEND SRCS_CPP "XiaozhiAssets.cpp" "third_party/xiaozhi_esp32/gif/lvgl_gif.cc")
endif()

# WHOLE_ARCHIVE keeps the static app registrar that main installs from the registry.
idf_component_register(
    SRCS ${SRCS_C} ${SRCS_CPP}
    INCLUDE_DIRS "."
    REQUIRES
        brookesia_core
        board
        esp-sr
        esp_audio_codec
        esp_audio_effects
        esp_event
        esp_netif
        esp_xiaozhi
        nvs_flash
        xiaozhi-fonts
    PRIV_REQUIRES
        app_update
        cjson
        efuse
        esp_app_format
        esp_http_client
        esp_hw_support
        esp_partition
        esp_timer
        esp_websocket_client
        esp_wifi
        esp_wifi_remote
        Camera
        Settings
        mbedtls
        mcp-c-sdk
        spi_flash
    WHOLE_ARCHIVE
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC XIAOZHI_THEME_ENABLED=${XIAOZHI_THEME_ENABLED})

set_source_files_properties(${SRCS_CPP} PROPERTIES COMPILE_FLAGS "-Wno-missing-field-initializers")

# Copy the Xiaozhi UI font into the SPIFFS image source directory.
idf_component_get_property(xiaozhi_fonts_dir 78__xiaozhi-fonts COMPONENT_DIR)
set(xiaozhi_font_src "${xiaozhi_fonts_dir}/cbin/font_puhui_common_30_4.bin")
set(xiaozhi_font_dst "${CMAKE_CURRENT_LIST_DIR}/../../spiffs/xiaozhi/font.bin")

if(xiaozhi_fonts_dir AND EXISTS "${xiaozhi_font_src}")
    file(COPY_FILE "${xiaozhi_font_src}" "${xiaozhi_font_dst}" ONLY_IF_DIFFERENT)
else()
    message(STATUS "xiaozhi-fonts common font not found; skipping SPIFFS font copy")
endif()

# esp_xiaozhi leaves the WebSocket task at the client's 4 KB default, which MCP
# tools/list overflows; XiaozhiWebsocketStack.cpp raises it through this wrap.
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=esp_websocket_client_init")
