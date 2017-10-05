cmake_minimum_required(VERSION 3.6)
project(test_engine)

set(BUILT_RESOURCES_DIR "${CMAKE_BINARY_DIR}/assets/cozmo_resources")

set(GTEST_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/util/libs/framework/gtest.framework")

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
set(PLATFORM_FLAGS "")
if (ANDROID)
  set(PLATFORM_LIBS log)
elseif (MACOSX)
  include(webots)
  find_library(FOUNDATION Foundation)
  set(PLATFORM_LIBS
    ${GTEST_LIBS}
    ${FOUNDATION}
  )
  set(PLATFORM_FLAGS -fobjc-arc)
endif()

include(anki_build_cxx_library)
include(symlink_target)

anki_build_cxx_executable(test_engine ${ANKI_SRCLIST_DIR})

target_link_libraries(test_engine 
  PRIVATE
  # anki libs
  cozmo_engine
  audio_engine
  cti_common
  cti_planning
  cti_vision
  cti_messaging
  engine_clad
  androidHAL
  util
  util_audio
  # platform
  ${PLATFORM_LIBS}
)

target_include_directories(test_engine 
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine>
)

#
# Set up symlink to resources
#
symlink_target(TARGET test_engine
  SRC ${BUILT_RESOURCES_DIR}
  DST resources#
)

target_compile_options(test_engine
  PRIVATE
  -Wno-undef
)

target_compile_definitions(test_engine 
  PRIVATE
  SIMULATOR
  CORETECH_BASESTATION
)

#
# Set up symlinks to satisfy webots
#
symlink_target(TARGET test_engine
  SRC ${WEBOTS_HOME}/lib/libController.dylib
  DST ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib/
)

symlink_target(TARGET test_engine
  SRC ${WEBOTS_HOME}/lib/libCppController.dylib
  DST ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib/
)

symlink_target(TARGET test_engine
  SRC ${WEBOTS_HOME}/lib/libode.dylib
  DST ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib/
)

symlink_target(TARGET test_engine
  SRC ${BUILT_RESOURCES_DIR}
  DST resources
)

if (MACOSX)
  enable_testing()
  add_test(NAME test_engine COMMAND test_engine)
endif(MACOSX)
