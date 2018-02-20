cmake_minimum_required(VERSION 3.6)

if (ANDROID)

project(vic-switchboard)

include(anki_build_cxx)
include(sodium)

anki_build_cxx_executable(vic-switchboard ${ANKI_SRCLIST_DIR})

target_compile_options(vic-switchboard
PRIVATE
  -std=c14 # needed for make_unique
)

#set_target_properties(sodium PROPERTIES LINKER_LANGUAGE C)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
  
set(PLATFORM_LIBS
  log
)

target_link_libraries(vic-switchboard
PRIVATE
  ${SODIUM_LIBS}
  ${PLATFORM_LIBS}
)

android_strip(TARGET vic-switchboard)

target_compile_definitions(vic-switchboard
  PRIVATE
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(vic-switchboard
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/switchboardd>
  #$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libev>
  #$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/signals>
  ${PLATFORM_INCLUDES}
)

endif(ANDROID)