# ESP-IDF CMakeLists.txt for PGPemu with LVGL UI
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    components/lvgl
    components/lvgl_esp32_drivers
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(pgpemu-display)
