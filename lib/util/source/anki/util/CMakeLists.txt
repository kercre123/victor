
project(libankiutil LANGUAGES CXX)

include(anki_build_cxx)
include(android_strip)

anki_build_cxx_library(util ${ANKI_SRCLIST_DIR})

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
  ${PLATFORM_LIBS}
)

android_strip(TARGET util)

