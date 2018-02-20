cmake_minimum_required(VERSION 3.6)

project(cozmo_webserver)

include(anki_build_cxx)

anki_build_cxx_library(victor_web_library ${ANKI_SRCLIST_DIR})

include(opencv)

if (NOT DEFINED USE_DAS)
  set(USE_DAS 0)
endif()

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES ${OPENCV_INCLUDE_PATHS})
set(PLATFORM_COMPILE_DEFS "")

if (ANDROID)
    set(PLATFORM_LIBS
        android
        log
    )
elseif (MACOSX)
    include(webots)
    set(PLATFORM_LIBS
        ${WEBOTS_LIBS}
    )
    set(PLATFORM_COMPILE_DEFS "-DSIMULATOR")
endif()

target_link_libraries(victor_web_library
  PRIVATE
  util
  cti_vision
  DAS
  osState
  # vendor
  civetweb
  PUBLIC
  robot_interface  # Needs to be public for cozmoConfig.h
)

android_strip(TARGET victor_web_library)

target_compile_definitions(victor_web_library
  PRIVATE
  USE_DAS=${USE_DAS}
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(victor_web_library 
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src> # allow "android/" prefix
  ${PLATFORM_INCLUDES}
)

# victor_web_server binary only builds on Android now
# mac implementation uses (webotsCtrlWebServer)
if (ANDROID)

  anki_build_cxx_executable(vic-webserver ${ANKI_SRCLIST_DIR})

  target_link_libraries(vic-webserver
    PRIVATE
    victor_web_library
    cti_common
    util
    ${PLATFORM_LIBS}
  )

  android_strip(TARGET vic-webserver)
    
endif()
