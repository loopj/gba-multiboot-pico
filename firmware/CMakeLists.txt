# set(PICO_BOARD seeed_xiao_rp2040)

cmake_minimum_required(VERSION 3.20)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(app C CXX ASM)
pico_sdk_init()

add_executable(app main.c)

target_link_libraries(app pico_stdlib hardware_spi)

pico_enable_stdio_usb(app 1)
pico_enable_stdio_uart(app 0)

pico_add_extra_outputs(app)
