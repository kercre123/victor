cmake_minimum_required(VERSION 3.6)

project(arf_tester)

include(anki_build_cxx)
include(anki_build_strip)
include(royale)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_FLAGS "")

if (MACOSX)
  find_library(FOUNDATION Foundation)
  set(PLATFORM_LIBS
    ${FOUNDATION}
  )

  anki_build_cxx_executable(royale_depth_node ${ANKI_SRCLIST_DIR})
  anki_build_target_license(royale_depth_node "ANKI")

  target_link_libraries(royale_depth_node
    PRIVATE
    #anki libs
    arf
    util
    # platform
    ${ROYALE_LIBS}
    ${PLATFORM_LIBS}
    ${ASAN_EXE_LINKER_FLAGS}
  )

  target_include_directories(royale_depth_node
    PUBLIC  
    $<BUILD_INTERFACE:${PROTOBUF_OUTPUT_DIR}>
  )

  anki_build_strip(TARGET royale_depth_node)
endif()