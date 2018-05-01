cmake_minimum_required(VERSION 3.6)

project(engine_tools)

include(anki_build_cxx)
include(android_strip)

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
  )
endif()

anki_build_cxx_executable(vic-engine ${ANKI_SRCLIST_DIR})

target_link_libraries(vic-engine
  PRIVATE
  # anki libs
  util
  cozmo_engine
  victorCrashReports
  # vendor
  breakpad_client
  # platform
  ${PLATFORM_LIBS}
)

android_strip(TARGET vic-engine)
