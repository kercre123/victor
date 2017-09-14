cmake_minimum_required(VERSION 3.6)

project(cozmo)

list(APPEND CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

include(import)

# These are used by some android cmake toolchains
# They appear to be ignored by the android-sdk toolchain that works with gradle/Android Studio
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 99)

set(SYMBOL_VISIBILITY_FLAGS "")

set(ANKI_PLATFORM_NAME "unknown")
if (ANDROID)
  set(ANKI_PLATFORM_NAME "android")
  set(SYMBOL_VISIBILITY_FLAGS "-fvisibility=hidden")
elseif(MACOSX)
  set(ANKI_PLATFORM_NAME "mac")
else()
  message(FATAL_ERROR "unsupported platform: ${ANKI_PLATFORM_NAME}")
endif()

add_compile_options(
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Release>:-Os>
)

if (ANDROID)
  # Build for Android system without ART (*NO* JNI available)
  add_definitions(
    -DDISABLE_JNI=1
  )
endif()

set(LINK_FLAGS "${LINK_FLAGS} -Wl,--threads")

if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

set(ANKI_EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/EXTERNALS")
set(CORETECH_EXTERNAL_DIR "${ANKI_EXTERNAL_DIR}/coretech_external")

set(ANKI_SRCLIST_DIR "${CMAKE_SOURCE_DIR}/generated/cmake")

#if (ANDROID)
#  add_subdirectory("lib/crash-reporting-android/Breakpad")
#endif()

#if (NOT breakpad_client)
#    message(FATAL_ERROR "breakpad_client not defined")
#endif()

import(clad "tools/message-buffers")
import(util "lib/util")
import(audio "lib/audio")
import(ble_cozmo "lib/BLECozmo")
import(DAS "lib/das-client")

# BRC-TODO: Make this top-level include into an "interface"
# lib so that other target can depend on it, instead of manually injecting
# include dirs
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

import(coretech "coretech")
add_subdirectory("clad")
add_subdirectory("androidHAL")
add_subdirectory("animProcess")
target_include_directories(victor_anim PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/anki/cozmo>
)
add_subdirectory("robot")

if (ANDROID)
    add_subdirectory("robot2")
endif()

import(cozmo_engine "engine")
target_include_directories(cozmo_engine PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/anki/cozmo>
)

add_subdirectory("resources")

if (ANDROID)
    add_subdirectory("project/victor/tools")
    include(android-ndk-stl-config)
endif()

if (MACOSX)
    add_subdirectory("test")
    add_subdirectory("simulator") # webots
endif()
