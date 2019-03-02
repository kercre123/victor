cmake_minimum_required(VERSION 3.6)

project(engine_tools)

include(anki_build_cxx)
include(anki_build_strip)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_FLAGS "")

if (VICOS)
  set(PLATFORM_LIBS log)
elseif (MACOSX)
  find_library(FOUNDATION Foundation)
  set(PLATFORM_LIBS
    ${FOUNDATION}
  )
endif()

anki_build_cxx_library(arf ${ANKI_SRCLIST_DIR})
anki_build_target_license(arf "ANKI")

target_link_libraries(arf
  PUBLIC
  nng
  pthread # Hack to get pthread to appear on linker line after nng
  # platform
  ${PLATFORM_LIBS}
  ${ASAN_EXE_LINKER_FLAGS}
)

anki_build_strip(TARGET arf)

target_include_directories(arf
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
  ${PLATFORM_INCLUDES}
)

anki_build_cxx_executable(subscriber ${ANKI_SRCLIST_DIR})
anki_build_target_license(subscriber "ANKI")

target_link_libraries(subscriber
  PRIVATE
  arf
  # platform
  ${PLATFORM_LIBS}
  ${ASAN_EXE_LINKER_FLAGS}
)