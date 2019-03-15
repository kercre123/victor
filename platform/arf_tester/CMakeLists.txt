cmake_minimum_required(VERSION 3.6)

project(arf_tester)

include(anki_build_cxx)
include(anki_build_strip)
include(protobuf)
include(nng_ext)

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

anki_build_cxx_executable(camera_main ${ANKI_SRCLIST_DIR})
anki_build_target_license(camera_main "ANKI")

target_link_libraries(camera_main
  PRIVATE
  # anki libs
  arf
  util
  cti_common
  cti_vision
  engine_clad # imageTypes.h
  robot_interface # cozmoConfig.h
  cameraService
  # platform
  ${PLATFORM_LIBS}
  ${ASAN_EXE_LINKER_FLAGS}
)

target_include_directories(camera_main
  PUBLIC  
  $<BUILD_INTERFACE:${PROTOBUF_OUTPUT_DIR}>
)

target_compile_definitions(camera_main
  PRIVATE
  ANKICORETECH_USE_OPENCV=1
  ${PLATFORM_COMPILE_DEFS}
)

anki_build_strip(TARGET camera_main)

anki_build_cxx_executable(camera_arf_main ${ANKI_SRCLIST_DIR})
anki_build_target_license(camera_arf_main "ANKI")

target_link_libraries(camera_arf_main
  PRIVATE
  # anki libs
  arf
  util
  cti_common
  cti_vision
  engine_clad # imageTypes.h
  robot_interface # cozmoConfig.h
  cameraService
  # platform
  ${PLATFORM_LIBS}
  ${ASAN_EXE_LINKER_FLAGS}
)

target_include_directories(camera_arf_main
  PUBLIC  
  $<BUILD_INTERFACE:${PROTOBUF_OUTPUT_DIR}>
)

target_compile_definitions(camera_arf_main
  PRIVATE
  ANKICORETECH_USE_OPENCV=1
  ${PLATFORM_COMPILE_DEFS}
)

anki_build_strip(TARGET camera_arf_main)