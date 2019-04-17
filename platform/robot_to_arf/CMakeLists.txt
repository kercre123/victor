cmake_minimum_required(VERSION 3.6)

project(robot_to_arf_converter)

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
  set(PLATFORM_COMPILE_DEFS ${PLATFORM_COMPILE_DEFS} 
      MACOS=1)
endif()

anki_build_cxx_executable(robot_to_arf_converter ${ANKI_SRCLIST_DIR})
anki_build_target_license(robot_to_arf_converter "ANKI")

target_link_libraries(robot_to_arf_converter
  PUBLIC
  # anki libs
  util
  cti_messaging
  # platform
  ${PLATFORM_LIBS}
  ${ASAN_EXE_LINKER_FLAGS}
)

anki_build_strip(TARGET robot_to_arf_converter)

