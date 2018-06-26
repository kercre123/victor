#
# lib/util/source/3rd/libev/CMakeLists.txt
#
# cmake declarations for building libev
#
project(libev)

add_library(libev STATIC
  ev.c
  ev.h
)

anki_build_target_license(libev "BSD-3,${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

target_compile_options(libev PRIVATE
  -Wno-comment
  -Wno-strict-aliasing
  -Wno-unused-function
  -Wno-unused-value
  -Wno-parentheses
  -Wno-extern-initializer
)

target_compile_options(libev PUBLIC
  -Wno-c++1z-compat-mangling
)

target_compile_definitions(libev PUBLIC
  EV_STANDALONE=1
  EV_USE_MONOTONIC=1
  EV_USE_SELECT=1
  EV_USE_POLL=0
  EV_USE_EPOLL=0
  EV_MULTIPLICITY=1
  EV_IDLE_ENABLE=1
  EV_CHILD_ENABLE=0
)

target_include_directories(libev PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
