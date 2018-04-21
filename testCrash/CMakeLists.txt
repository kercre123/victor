cmake_minimum_required(VERSION 3.6)

project(victor_testcrash)

include(anki_build_cxx)

include(opencv)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES ${OPENCV_INCLUDE_PATHS})
set(PLATFORM_COMPILE_DEFS "")

if (ANDROID)
    include(breakpad)
    add_definitions(-DUSE_GOOGLE_BREAKPAD)
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

if (ANDROID)

  anki_build_cxx_executable(vic-testcrash ${ANKI_SRCLIST_DIR})

  target_link_libraries(vic-testcrash
    PRIVATE
    cti_common
    util
    ${BREAKPAD_LIBS}
    ${PLATFORM_LIBS}
    PUBLIC
    robot_interface  # Needs to be public for cozmoConfig.h
  )

  android_strip(TARGET vic-testcrash)
    
endif()
