cmake_minimum_required(VERSION 3.6)
project(androidHAL)

include(anki_build_cxx_library)
anki_build_cxx_library(androidHAL ${ANKI_SRCLIST_DIR})

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
if (ANDROID)
    set(PLATFORM_LIBS
        android
        log
        GLESv2
        mediandk
        camera2ndk)
elseif (MACOSX)
    include(webots)
    set(PLATFORM_LIBS
        ${OPENCV_LIBS}
        ${WEBOTS_LIBS}
    )
    set(PLATFORM_COMPILE_DEFS "-DSIMULATOR")
endif()

target_link_libraries(androidHAL
PRIVATE
  util
# cti
  cti_common
  cti_vision
  cti_planning
  cti_messaging
  robot_clad # imageTypes.h
# platform
#PUBLIC
  # these need to be public until we remove android-specific classes from androidHAL.h
  ${PLATFORM_LIBS}
)

android_strip(TARGET androidHAL)

target_compile_definitions(androidHAL
  PRIVATE
  -DCOZMO_BASESTATION
  -DCOZMO_V2
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(androidHAL PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> # allow "android/" prefix
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../robot/include>
  ${PLATFORM_INCLUDES}
)

