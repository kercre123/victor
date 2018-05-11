cmake_minimum_required(VERSION 3.6)

project(cameraService)

if (VICOS)

add_library(camera_client STATIC
    vicos/camera_client/camera_client.h
    vicos/camera_client/camera_client.c
    vicos/camera_client/log.h
    vicos/camera_client/log.c
)

target_include_directories(camera_client
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
  SYSTEM PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vicos/camera_client/inc>
)

target_compile_definitions(camera_client
  PRIVATE
  USE_ION
  USE_ANDROID_LOGGING=1
)

target_compile_options(camera_client
PRIVATE
  -std=c11 # needed for stdatomic.h
)

set_target_properties(camera_client PROPERTIES LINKER_LANGUAGE C)

endif()

include(anki_build_cxx)

anki_build_cxx_library(cameraService ${ANKI_SRCLIST_DIR})

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
if (VICOS)
    set(PLATFORM_LIBS
        log
        camera_client)
elseif (MACOSX)
    include(webots)
    set(PLATFORM_LIBS
        ${OPENCV_LIBS}
        ${WEBOTS_LIBS}
    )
    set(PLATFORM_COMPILE_DEFS "-DSIMULATOR")
endif()

target_link_libraries(cameraService
PRIVATE
  util
  cti_common
  cti_vision
  engine_clad # imageTypes.h
  robot_interface # cozmoConfig.h
  ${PLATFORM_LIBS}
)

anki_build_strip(TARGET cameraService)

target_compile_definitions(cameraService
  PRIVATE
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(cameraService
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> # allow "vicos/" prefix
  ${PLATFORM_INCLUDES}
)
