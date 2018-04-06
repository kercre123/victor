cmake_minimum_required(VERSION 3.6)

project(engine)

# Build options
option(USE_DAS "Enable DAS reporting" OFF)

include(opencv)
include(okao)
include(flatbuffers)
include(archive)
include(routing_http_server)
include(android_strip)
include(anki_build_cxx)

#
# cozmo_engine
#
anki_build_cxx_library(cozmo_engine ${ANKI_SRCLIST_DIR})


set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
set(PLATFORM_FLAGS "")
if (ANDROID)
  include(breakpad)
  set(PLATFORM_LIBS android log GLESv2)
elseif (MACOSX)
  find_library(FOUNDATION Foundation)
  include(webots)
  set(PLATFORM_LIBS
    ${FOUNDATION}
    ${OPENCV_LIBS}
    ${ROUTING_HTTP_LIBS}
    ${WEBOTS_LIBS}
  )
  set(PLATFORM_COMPILE_DEFS
    SIMULATOR
  )
endif()


target_link_libraries(cozmo_engine
  PUBLIC
  # anki libs
  clad
  util
  cti_common
  cti_messaging
  robot_interface
  audio_engine
  audio_multiplexer_engine
  engine_clad
  PRIVATE
  ble_cozmo
  DAS
  cameraService
  canned_anim_lib_engine
  victor_web_library
  # cti
  cti_vision
  cti_planning
  cubeBleClient
  osState
  micdata
  # vendor
  ${FLATBUFFERS_LIBS}
  ${LIBARCHIVE_LIBS}
  ${BREAKPAD_LIBS}
  # system
  z
  # platform
  ${PLATFORM_LIBS}
)

target_compile_options(cozmo_engine PRIVATE
  ${SYMBOL_VISIBILITY_FLAGS}
  "${PLATFORM_FLAGS}"
)

android_strip(TARGET cozmo_engine)

target_compile_definitions(cozmo_engine
  PRIVATE
  CORETECH_ENGINE
  USE_DAS=$<BOOL:${USE_DAS}>
  "${PLATFORM_COMPILE_DEFS}"
)

target_include_directories(cozmo_engine
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> # allows reference to engine/
  #PRIVATE
  #$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/anki/cozmo/basestation/cozmoAPI>
)

if (ANDROID)
  copy_opencv_android_libs()
  add_dependencies(cozmo_engine
    copy_opencv_libs
  )
endif()

if (ANDROID)
  # TODO: support tools on mac
  add_subdirectory(tools)
endif(ANDROID)
