
project(libankiutil LANGUAGES CXX)

include(anki_build_cxx)
include(anki_build_strip)

anki_build_cxx_library(util ${ANKI_SRCLIST_DIR})
anki_build_target_license(util "ANKI" "LGPL-2.1" "MPL-2.0" "CC0")

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
elseif (MACOSX)
  find_library(FOUNDATION Foundation)
  set(PLATFORM_LIBS ${FOUNDATION})
endif()

target_link_libraries(util
  # anki libs
  clad
  util_clad
  # vendor libs
  jsoncpp
  kazmath
  hdr_histogram
  ${PLATFORM_LIBS}
)

anki_build_strip(TARGET util)

