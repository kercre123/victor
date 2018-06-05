cmake_minimum_required(VERSION 3.6)

project(victorDAS)

include(anki_build_cxx)
include(anki_build_strip)

if (VICOS)

  anki_build_cxx_library(victorDAS ${ANKI_SRCLIST_DIR})
  anki_build_target_license(victorDAS "ANKI")

  target_link_libraries(victorDAS
    PUBLIC
    DAS
    util
  )

  anki_build_strip(TARGET victorDAS)

endif()

