cmake_minimum_required(VERSION 3.6)

project(cozmo_neon)

include(anki_build_cxx)

anki_build_cxx_library(victor_neon ${ANKI_SRCLIST_DIR} STATIC)

include(opencv)

if (NOT DEFINED USE_DAS)
  set(USE_DAS 0)
endif()

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES ${OPENCV_INCLUDE_PATHS})
set(PLATFORM_COMPILE_DEFS "")

if (ANDROID)
    set(PLATFORM_LIBS
        android)
endif()

target_link_libraries(victor_neon
  PRIVATE
  util
  cti_vision
  ${OPENCV_LIBS}
  ${PLATFORM_LIBS}
)

android_strip(TARGET victor_neon)

target_compile_definitions(victor_neon
  PRIVATE
  USE_DAS=${USE_DAS}
  ${PLATFORM_COMPILE_DEFS}
)

target_compile_options(victor_neon
  PRIVATE
  -O3
)

target_include_directories(victor_neon 
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src> # allow "android/" prefix
  ${PLATFORM_INCLUDES}
)

# victor_neonator binary only builds on Android now
# mac implementation uses (webotsCtrlneon)
if (ANDROID)

  anki_build_cxx_executable(victor_neonator ${ANKI_SRCLIST_DIR})

  target_link_libraries(victor_neonator
    PRIVATE
    victor_neon
    util
    ${PLATFORM_LIBS}
  )

  android_strip(TARGET victor_neonator)
    
endif()
