cmake_minimum_required(VERSION 3.6)

if (VICOS)

project(vic-faultCodeDisplay)

include(anki_build_cxx)
include(robot_core)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES ${OPENCV_INCLUDE_PATHS})
set(PLATFORM_COMPILE_DEFS "")

set(PLATFORM_LIBS
  robot_core
)

add_executable(vic-faultCodeDisplay
  ${CMAKE_SOURCE_DIR}/platform/faultCodeDisplay/faultCodeDisplay.cpp)

target_link_libraries(vic-faultCodeDisplay
  PUBLIC
  robot_interface
  PRIVATE
  cti_common
  cti_vision
  ${PLATFORM_LIBS}
)

anki_build_strip(TARGET vic-faultCodeDisplay)

target_compile_definitions(vic-faultCodeDisplay
  PRIVATE
  ${PLATFORM_COMPILE_DEFS}
)

endif(VICOS)
