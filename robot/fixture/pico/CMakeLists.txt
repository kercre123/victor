cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(enc_test C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(enc_test
   enc_test.c
)

pico_enable_stdio_usb(enc_test 1)
pico_enable_stdio_uart(enc_test 1)

pico_add_extra_outputs(enc_test)

target_link_libraries(enc_test pico_stdlib hardware_adc hardware_dma)
