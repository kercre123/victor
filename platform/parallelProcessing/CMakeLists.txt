cmake_minimum_required(VERSION 3.6)

project(parallel_processing)

include(opencv)
include(anki_build_cxx)

anki_build_cxx_executable(parallel_processing ${ANKI_SRCLIST_DIR})

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
        webotsCtrlCommon
        ${WEBOTS_LIBS}
    )
    set(PLATFORM_COMPILE_DEFS "-DSIMULATOR")
endif()

target_link_libraries(parallel_processing
PRIVATE
  cameraService
  cti_vision
  engine_clad # imageTypes.h
  robot_interface # cozmoConfig.h
  ${PLATFORM_LIBS}
)

anki_build_strip(TARGET parallel_processing)

target_compile_definitions(parallel_processing
  PRIVATE
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(parallel_processing
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> # allow "vicos/" prefix
  ${PLATFORM_INCLUDES}
)

anki_build_target_license(parallel_processing "ANKI")
