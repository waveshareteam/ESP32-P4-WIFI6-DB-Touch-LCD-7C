set(PROJ_SRC ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE PROJ_SRCS_C ${PROJ_SRC}/*.c)
file(GLOB_RECURSE PROJ_SRCS_CPP ${PROJ_SRC}/*.cpp)

# WHOLE_ARCHIVE keeps the static app registrar that main installs from the registry.
idf_component_register(
    SRCS  ${PROJ_SRCS_C} ${PROJ_SRCS_CPP}
    INCLUDE_DIRS ${PROJ_SRC}
    REQUIRES esp_driver_uart esp_driver_gpio nvs_flash esp_board_manager brookesia_core
    WHOLE_ARCHIVE)

set_source_files_properties(
    ${PROJ_SRCS_CPP}
    PROPERTIES
        COMPILE_FLAGS "-Wno-missing-field-initializers"
)
