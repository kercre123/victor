cmake_minimum_required(VERSION 3.6)

project(engine_tools)

include(nng_ext)

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
  # platform
  ${NNG_LIBS}
  pthread # Hack to get pthread to appear on linker line after nng
  ${PLATFORM_LIBS}
  ${ASAN_EXE_LINKER_FLAGS}
)

anki_build_strip(TARGET arf)

target_include_directories(arf
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
  $<BUILD_INTERFACE:${NNG_INCLUDE_PATH}>
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