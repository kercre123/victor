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
  cti_common
  cti_messaging
  robot_interface
  audio_engine
  audio_multiplexer_engine
  engine_clad
  DAS
  util
  PRIVATE
  ble_cozmo
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
  PUBLIC
  USE_DAS=$<BOOL:${USE_DAS}>
  PRIVATE
  CORETECH_ENGINE
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

  # Placeholder install task for cozmo_engine. We will eventually transition to using
  # install for all libs & binaries in order to assign appropriate permissions.
  install(TARGETS cozmo_engine
    LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/dist/lib
    ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/dist/lib
  )
endif(ANDROID)

