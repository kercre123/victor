project(libankiutil LANGUAGES CXX)

include(anki_build_cxx)

anki_build_cxx_library(util ${ANKI_SRCLIST_DIR})

target_include_directories(util PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)

set (PLATFORM_LIBS "")
if (ANDROID)
  set(PLATFORM_LIBS android log)
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
  c-library
  ${PLATFORM_LIBS})

include(android_strip)
android_strip(TARGET util)
