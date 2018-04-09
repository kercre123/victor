cmake_minimum_required(VERSION 3.6)

list(APPEND CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/victor-clad/cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/util/cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/tools/build/cmake
)

project(victor)

# Build options
option(USE_DAS "Enable DAS reporting" OFF)

# Use ccache if it is installed
include(ccache)

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
  set(SYMBOL_VISIBILITY_FLAGS "-fvisibility=hidden")
elseif(VICOS_STAGING)
  set(ANKI_PLATFORM_NAME "vicos")
  set(SYMBOL_VISIBILITY_FLAGS "-fvisibility=hidden")
else()
  message(FATAL_ERROR "unsupported platform: ${ANKI_PLATFORM_NAME}")
endif()

message(STATUS "ANKI_PLATFORM_NAME: ${ANKI_PLATFORM_NAME}")

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

add_definitions(-DANKI_BUILD_SHA="${ANKI_BUILD_SHA}")

if (ANDROID OR VICOS OR VICOS_STAGING)
  # Build for Android system without ART (*NO* JNI available)
  add_definitions(
    -DDISABLE_JNI=1
  )
else()
  add_definitions(
    -DDISABLE_JNI=0
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

set(CLAD_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/victor-clad/tools/message-buffers")
set(CLAD_VICTOR_EMITTER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/victor-clad/victorEmitters")
add_subdirectory("victor-clad") # includes clad via message-buffers

if (VICOS_STAGING)
  # VICOS_STAGING is a temp transition phase for converting build targets
  # to use an embedded linux toolchain.
  #
  # Targets listed below build successfully with the vicos-sdk toolchain
  #

  import(util "lib/util")
  #include(audio) # See ./cmake/audio.cmake for definitions and lib
  import(ble_cozmo "lib/BLECozmo")

  # Treat VICOS like a standard "unix" target for DAS
  import(DAS "lib/das-client")

elseif(VICOS)
  # Until _all_ targets build for VICOS (instead of android), this should only
  # contain targets that require / only build with the vicos toolchain.
  import(cti_messaging "coretech/messaging")
  add_subdirectory("clad")

  # include only required deps from util
  add_subdirectory("lib/util/source/3rd/jsoncpp")
  add_subdirectory("lib/util/source/anki/clad")

  add_subdirectory("coretech/common/clad_src/clad")
  add_subdirectory("coretech/vision/clad_src")
  add_subdirectory("robot/clad")
  add_subdirectory("platform/switchboard")

  include(vicos-sdk-stl-config)
else()
  # Current DEFAULT (android or mac platforms)
  set(CLEAN_CLAD_TARGETS TRUE)

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
  add_subdirectory("webServerProcess")
  add_subdirectory("lib/micData")
  add_subdirectory("lib/signalEssence")
  add_subdirectory("platform")

  target_include_directories(victor_anim PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/anki/cozmo>
  )

  target_include_directories(victor_web_library PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/anki/cozmo>
  )

  add_subdirectory("robot")

  import(cozmo_engine "engine")

  target_include_directories(cozmo_engine PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/anki/cozmo>
  )

  if (ANDROID)
      add_subdirectory("project/victor/tools")
      include(android-ndk-stl-config)
  endif()

  add_subdirectory("resources")
endif() # VICOS, VICOS_STAGING

if (MACOSX)
  add_subdirectory("test")
  add_subdirectory("simulator") # webots

  enable_testing()
endif()

# VICOS/STAGING note: Platforms that generate clad should set this variable
# This avoids nuking CLAD output when no targets were built for a platform
if (CLEAN_CLAD_TARGETS)
  clean_clad_targets()
endif()
