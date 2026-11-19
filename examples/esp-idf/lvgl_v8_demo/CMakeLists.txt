cmake_minimum_required(VERSION 3.22)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(BMGR_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/components/gen_bmgr_codes/board_manager.defaults")
if(EXISTS "${BMGR_DEFAULTS}")
    list(FIND SDKCONFIG_DEFAULTS "${BMGR_DEFAULTS}" BMGR_DEFAULTS_INDEX)
    if(BMGR_DEFAULTS_INDEX EQUAL -1)
        if(SDKCONFIG_DEFAULTS)
            list(INSERT SDKCONFIG_DEFAULTS 1 "${BMGR_DEFAULTS}")
        else()
            set(SDKCONFIG_DEFAULTS
                "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults;${BMGR_DEFAULTS}"
            )
        endif()
    endif()
endif()

project(lvgl_v8_demo)
