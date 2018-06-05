cmake_minimum_required(VERSION 3.6)

if (VICOS)

project(anki-ble)

include(anki_build_cxx)

anki_build_cxx_library(anki-ble ${ANKI_SRCLIST_DIR})
anki_build_target_license(anki-ble "ANKI" "BSD-2")

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")

set(PLATFORM_LIBS
  log
)

target_link_libraries(anki-ble
PRIVATE
  ${PLATFORM_LIBS}
)

target_compile_definitions(anki-ble
  PRIVATE
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(anki-ble
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
  PRIVATE
  ${PLATFORM_INCLUDES}
)

anki_build_strip(TARGET anki-ble)

endif(VICOS)
