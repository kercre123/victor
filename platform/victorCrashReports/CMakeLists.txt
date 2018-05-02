cmake_minimum_required(VERSION 3.6)

project(victorCrashReports)

include(anki_build_cxx)
include(anki_build_strip)

if (ANDROID)

  anki_build_cxx_library(victorCrashReports ${ANKI_SRCLIST_DIR} STATIC)

  target_include_directories(victorCrashReports
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/lib/util/source/anki>
  )

  target_compile_definitions(victorCrashReports
    PRIVATE
    USE_GOOGLE_BREAKPAD
  )

  anki_build_strip(TARGET victorCrashReports)

endif()
