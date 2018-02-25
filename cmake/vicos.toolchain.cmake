set(vicos_root "/Users/paul/Downloads/vicos-sdk-apq8009_robot_le_3_18_66-r01-x86_64-apple-darwin")
set(target_triple "arm-oe-linux-gnueabi")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_CXX_COMPILER_ID Clang)

set(CMAKE_C_COMPILER "${vicos_root}/prebuilt/bin/arm-oe-linux-gnueabi-clang")
set(CMAKE_CXX_COMPILER "${vicos_root}/prebuilt/bin/arm-oe-linux-gnueabi-clang++")
set(CMAKE_LINKER "${vicos_root}/prebuilt/bin/arm-oe-linux-gnueabi-ld")

set(CMAKE_SYSROOT "${vicos_root}/sysroot")

set(CMAKE_CXX_FLAGS "-I${CMAKE_SYSROOT}/usr/include")

SET(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
