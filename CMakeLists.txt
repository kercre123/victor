cmake_minimum_required(VERSION 3.6)

list(APPEND CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_SOURCE_DIR}/victor-sdk-clad/cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/util/cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/tools/build/cmake
)

project(cozmo)

include(ccache) # Use ccache if it is installed

include(import)
include(clean_clad_targets)

# These are used by some android cmake toolchains
# They appear to be ignored by the android-sdk toolchain that works with gradle/Android Studio
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 99)

if (CMAKE_GENERATOR MATCHES "Xcode")
  set(CMAKE_XCODE_GENERATE_SCHEME ON)
endif()

# Set up custom property for file locations for custom targets
define_property(TARGET PROPERTY ANKI_OUT_PATH
                BRIEF_DOCS "The output path for this custom target"
                FULL_DOCS "The output path for this custom target")

set(SYMBOL_VISIBILITY_FLAGS "")

set(ANKI_PLATFORM_NAME "unknown")
if (ANDROID)
  set(ANKI_PLATFORM_NAME "android")
  set(SYMBOL_VISIBILITY_FLAGS "-fvisibility=hidden")
elseif(MACOSX)
  set(ANKI_PLATFORM_NAME "mac")
elseif(VICOS)
  set(ANKI_PLATFORM_NAME "vicos")
else()
  message(FATAL_ERROR "unsupported platform: ${ANKI_PLATFORM_NAME}")
endif()

add_compile_options(
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Release>:-O2>
)

#
# Factory build options
#
if (FACTORY_TEST)
  add_definitions(-DFACTORY_TEST=1)
else()
  add_definitions(-DFACTORY_TEST=0)
endif()

if (FACTORY_TEST_DEV)
  add_definitions(-DFACTORY_TEST_DEV=1)
else()
  add_definitions(-DFACTORY_TEST_DEV=0)
endif()

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

if (ANDROID)
  set(ANDROID_TOOLCHAIN_DIR "${CMAKE_BINARY_DIR}/toolchain")
  set(ANDROID_TOOLCHAIN_CC "${ANDROID_TOOLCHAIN_DIR}/bin/arm-linux-androideabi-gcc")
  set(ANDROID_TOOLCHAIN_CXX "${ANDROID_TOOLCHAIN_DIR}/bin/arm-linux-androideabi-g++")
  add_custom_command(
    OUTPUT "${ANDROID_TOOLCHAIN_CC}" "${ANDROID_TOOLCHAIN_CXX}"
    COMMAND "${ANDROID_NDK}/build/tools/make_standalone_toolchain.py" --install-dir=${ANDROID_TOOLCHAIN_DIR}
            --arch=arm --api=24 --stl libc++ --force --unified-headers
  )
  add_custom_target(android_toolchain DEPENDS "${ANDROID_TOOLCHAIN_CXX}" "${ANDROID_TOOLCHAIN_CC}")

endif()

#if (ANDROID)
#  add_subdirectory("lib/crash-reporting-android/Breakpad")
#endif()

#if (NOT breakpad_client)
#    message(FATAL_ERROR "breakpad_client not defined")
#endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

set(CLAD_VICTOR_EMITTER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/victor-sdk-clad/victorEmitters")
set(CLAD_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/victor-sdk-clad/tools/message-buffers")

if(VICOS)
  add_definitions(-DVICOS)
  set(DISABLE_JSON true)
  import(cti_messaging "coretech/messaging")
  add_subdirectory("clad")
  add_subdirectory("lib/util/source/anki/clad")
  add_subdirectory("coretech/common/clad")
  add_subdirectory("coretech/vision/clad")
  add_subdirectory("robot/clad")
  add_subdirectory("victor-sdk-clad")
  add_subdirectory("platform/switchboard")
else()
import(clad "tools/message-buffers")
import(util "lib/util")
include(audio) # See ./cmake/audio.cmake for definitions and lib
import(ble_cozmo "lib/BLECozmo")
import(DAS "lib/das-client")

# BRC-TODO: Make this top-level include into an "interface"
# lib so that other target can depend on it, instead of manually injecting
# include dirs
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

import(coretech "coretech")
add_subdirectory("clad")
add_subdirectory("cloud")
add_subdirectory("cubeBleClient")
add_subdirectory("osState")
add_subdirectory("cannedAnimLib")
add_subdirectory("animProcess")
#add_subdirectory("webServerProcess")
add_subdirectory("lib/signalEssence")
add_subdirectory("platform")

target_include_directories(victor_anim PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/anki/cozmo>
)

#target_include_directories(victor_web_library PRIVATE
#  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/anki/cozmo>
#)

add_subdirectory("robot")

import(cozmo_engine "engine")

target_include_directories(cozmo_engine PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/anki/cozmo>
)

add_subdirectory("resources")
endif(VICOS)

if (ANDROID)
    add_subdirectory("project/victor/tools")
    include(android-ndk-stl-config)
endif()

if (MACOSX)
  add_subdirectory("test")
  add_subdirectory("simulator") # webots

  enable_testing()

endif()

clean_clad_targets()