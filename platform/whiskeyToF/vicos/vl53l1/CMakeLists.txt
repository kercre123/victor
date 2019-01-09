cmake_minimum_required(VERSION 3.6)

project(vl53l1)

if (VICOS)

add_library(vl53l1 STATIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/src>
)

target_include_directories(vl53l1
  SYSTEM PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/inc>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/platform/inc>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/protected/inc>
)

set_target_properties(vl53l1 PROPERTIES LINKER_LANGUAGE C)

target_compile_options(vl53l1 PUBLIC -Wno-error=conditional-uninitialized)
target_compile_options(vl53l1 PUBLIC -Wno-error=unused-variable)

endif()
