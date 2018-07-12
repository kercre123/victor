#
# lib/util/source/3rd/elfutils/libdw/CMakeLists.txt
#
# cmake declarations for building libdw
#
project(libeu)

add_library(libeu STATIC
  xstrdup.c
  xstrndup.c
  xmalloc.c
  next_prime.c
  crc32.c
  crc32_file.c
  color.c
  printversion.c

  fixedsizehash.h
  libeu.h system.h
  dynamicsizehash.h
  list.h
  eu-config.h
  color.h 
  printversion.h bpf.h
)

anki_build_target_license(libeu "LGPL-3.0,${CMAKE_CURRENT_SOURCE_DIR}/../COPYING-LGPLV3")

# target_compile_options(libdw PRIVATE
#   -Wno-comment
#   -Wno-strict-aliasing
#   -Wno-unused-function
#   -Wno-unused-value
#   -Wno-parentheses
#   -Wno-extern-initializer
# )
# 
# target_compile_options(libev PUBLIC
#   -Wno-c++1z-compat-mangling
# )
# 
# target_compile_definitions(libev PUBLIC
#   EV_STANDALONE=1
#   EV_USE_MONOTONIC=1
#   EV_USE_SELECT=1
#   EV_USE_POLL=0
#   EV_USE_EPOLL=0
#   EV_MULTIPLICITY=1
#   EV_IDLE_ENABLE=1
#   EV_CHILD_ENABLE=0
# )
# 

target_include_directories(libeu PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
