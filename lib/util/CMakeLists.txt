cmake_minimum_required(VERSION 3.6)

project(anki_util)

list(APPEND CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

include(import)

# sadly... these seem to be ignored by the cmake android toolchain
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 99)

if (NOT ANKI_SRCLIST_DIR)
  set(ANKI_SRCLIST_DIR "${CMAKE_SOURCE_DIR}/generated/cmake")
endif()

if (NOT TARGET clad)
  if (NOT CLAD_SOURCE_DIR)
    set(CLAD_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/message-buffers")
  endif()
  add_subdirectory(${CLAD_SOURCE_DIR} "${CMAKE_BINARY_DIR}/message-buffers")
endif()

if (NOT TARGET jsoncpp)
  add_subdirectory("source/3rd/jsoncpp")
endif()

if (NOT TARGET kazmath)
  add_subdirectory("source/3rd/kazmath")
endif()

if (NOT TARGET civetweb)
  add_subdirectory("source/3rd/civetweb")
endif()

if (NOT TARGET hdr_histogram_static)
  add_subdirectory("source/3rd/hdrhistogram_c")
  foreach(target hdr_histogram hdr_histogram_static)
  anki_build_target_license(${target} "BSD-2,${CMAKE_CURRENT_SOURCE_DIR}/source/3rd/hdrhistogram_c/LICENSE.txt")
  endforeach()
endif()

# Disable webp until we are going to use it
# if (NOT TARGET webp)
#  add_subdirectory("source/3rd/libwebp")
# endif()

if (NOT TARGET libev)
  add_subdirectory("source/3rd/libev")
endif()

if (NOT TARGET audio_util)
  add_subdirectory("source/anki/audioUtil")
endif()

if (NOT TARGET util_clad)
  add_subdirectory("source/anki/clad")
endif()

if (NOT TARGET util)
  add_subdirectory("source/anki/util")
endif()

if (MACOSX)
  if (NOT TARGET utilUnitTest)
    add_subdirectory("source/anki/utilUnitTest")
  endif()

  enable_testing()

  add_test(NAME utilUnitTest COMMAND utilUnitTest)

  set_tests_properties(utilUnitTest
    PROPERTIES
    ENVIRONMENT "GTEST_OUTPUT=xml:utilUnitGoogleTest.xml"
  )

endif()
