cmake_minimum_required(VERSION 3.6)
project(engine_tests)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
set(PLATFORM_FLAGS "")
if (ANDROID)
  set(PLATFORM_LIBS log)
elseif (MACOSX)
  find_library(FOUNDATION Foundation)
  set(PLATFORM_LIBS
    ${FOUNDATION}
    ${WEBOTS_LIBS}
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/util/libs/framework/gtest.framework"
  )
  set(PLATFORM_FLAGS -fobjc-arc)
endif()

message(STATUS "test_engine:PLATFORM_LIBS=${PLATFORM_LIBS}")

include(anki_build_cxx_library)
anki_build_cxx_executable(test_engine ${ANKI_SRCLIST_DIR})

target_link_libraries(test_engine
PRIVATE
# anki libs
  util
  util_audio
  cozmo_engine
  audio_engine
  cti_common
  cti_planning
  cti_vision
  cti_messaging
  engine_clad
# platform
  ${PLATFORM_LIBS}
)

target_compile_definitions(test_engine PRIVATE                                                       
    -DSIMULATOR
    -DCORETECH_BASESTATION                                                                          
    -DANKICORETECH_USE_OPENCV=1                                                                     
    -DANKICORETECH_EMBEDDED_USE_OPENCV=1)

target_include_directories(test_engine
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine>
)
