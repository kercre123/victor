cmake_minimum_required(VERSION 3.6)

project(victorDAS)

include(anki_build_cxx)
include(android_strip)

if (ANDROID OR VICOS)

  anki_build_cxx_library(victorDAS ${ANKI_SRCLIST_DIR})

  target_link_libraries(victorDAS
    PUBLIC
    DAS
    util
  )

  android_strip(TARGET victorDAS)

endif()

