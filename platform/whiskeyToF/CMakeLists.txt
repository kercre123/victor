cmake_minimum_required(VERSION 3.6)

project(whiskeyToF)

include(anki_build_cxx)

anki_build_cxx_library(whiskeyToF ${ANKI_SRCLIST_DIR})

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
if (VICOS)
    set(PLATFORM_LIBS
        log
    )
elseif (MACOSX)
    include(webots)
    set(PLATFORM_LIBS
        ${OPENCV_LIBS}
        ${WEBOTS_LIBS}
    )
    set(PLATFORM_COMPILE_DEFS "-DSIMULATOR")
endif()

target_link_libraries(whiskeyToF
PRIVATE
  util
  cti_common
  cti_vision
  engine_clad # imageTypes.h
  robot_interface # cozmoConfig.h
  ${PLATFORM_LIBS}
)

anki_build_strip(TARGET whiskeyToF)

target_compile_definitions(whiskeyToF
  PRIVATE
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(whiskeyToF
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> # allow "vicos/" prefix
  ${PLATFORM_INCLUDES}
)
