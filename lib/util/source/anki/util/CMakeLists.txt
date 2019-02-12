
project(libankiutil LANGUAGES CXX)

include(anki_build_cxx)
include(anki_build_strip)

anki_build_cxx_library(util ${ANKI_SRCLIST_DIR})
anki_build_target_license(util "ANKI" "RSA,${CMAKE_SOURCE_DIR}/licenses/md5.license" "CC0,${CMAKE_SOURCE_DIR}/licenses/simple-signal.license")

#
# Create output files named 'libankiutil' to prevent collision with MacOS 'libutil'.
# Target name remains 'util' for convenience of downstream projects.
#

set_target_properties(util PROPERTIES OUTPUT_NAME ankiutil)

target_include_directories(util
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

set (PLATFORM_LIBS "")

if (ANDROID)
  set(PLATFORM_LIBS log)
elseif (VICOS)
  set(PLATFORM_LIBS log)
  target_include_directories(util
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>)
elseif (MACOSX)
  find_library(FOUNDATION Foundation)
  set(PLATFORM_LIBS ${FOUNDATION})
  target_include_directories(util
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>)
endif()

target_link_libraries(util
  PUBLIC
  # anki libs
  clad
  util_clad
  # vendor libs
  jsoncpp
  kazmath
  hdr_histogram_static
  ${ANKITRACE}
  ${PLATFORM_LIBS}
  ${ASAN_SHARED_LINKER_FLAGS}
)

anki_build_strip(TARGET util)
