# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# WIN32 describes the target platform. This cross-compiled ESP-IDF target is
# not Windows, even when CMake itself runs on Windows.
if(CMAKE_HOST_WIN32 AND CMAKE_GENERATOR MATCHES "Ninja")
    set(CMAKE_NINJA_FORCE_RESPONSE_FILE ON CACHE BOOL
        "Force Ninja to pass compiler arguments through response files" FORCE)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
# Reported to the Xiaozhi server as the firmware version. The console gates custom themes on
# a semantic version (>= 2.0.0), so do not let it fall back to the git describe string.
# TODO(theme): themes disabled; the version is left unset. Restore PROJECT_VER "2.0.0" to re-enable.
# set(PROJECT_VER "2.0.0")
project(esp-brookesia)
