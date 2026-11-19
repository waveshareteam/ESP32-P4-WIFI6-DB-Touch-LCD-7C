set(example_sources "main.c")
if(CONFIG_EXAMPLE_MODE_ECHO)
    list(APPEND example_sources "audio_echo.c")
else()
    list(APPEND example_sources "audio_playback.c")
endif()

idf_component_register(
    SRCS ${example_sources}
    PRIV_REQUIRES esp_board_manager
    EMBED_FILES "canon.pcm"
)
