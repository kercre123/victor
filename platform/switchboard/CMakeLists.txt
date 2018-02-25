cmake_minimum_required(VERSION 3.6)

if (VICOS)

project(vic-switchboard)

include(vicos.toolchain)
include(anki_build_cxx)
#include(sodium)

#set_target_properties(sodium PROPERTIES LINKER_LANGUAGE C)
#add_executable(vic-switchboard ${ANKI_SRCLIST_DIR})
#get_source_file_property( MYPROPS _switchboardMain.cpp COMPILE_FLAGS )
#message(FATAL_ERROR ${CMAKE_CC_FLAGS})
anki_build_cxx_executable(vic-switchboard ${ANKI_SRCLIST_DIR})

target_compile_options(vic-switchboard
PRIVATE
  -std=c14 # needed for make_unique
)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
  
set(PLATFORM_LIBS
  #log
  c
)

target_link_libraries(vic-switchboard
PRIVATE
  ${PLATFORM_LIBS}
  #${SODIUM_LIBS}
)

#android_strip(TARGET vic-switchboard)

target_compile_definitions(vic-switchboard
  PRIVATE
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(vic-switchboard
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/switchboardd>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/anki-ble>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libev>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/signals>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libsodium/libsodium/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libsodium/libsodium/include/sodium>
  $<BUILD_INTERFACE:${vicos_root}/sysroot/usr/include/glib-2.0>
  $<BUILD_INTERFACE:${vicos_root}/sysroot/usr/lib/glib-2.0/include>
  ${PLATFORM_INCLUDES}
)

endif(VICOS)