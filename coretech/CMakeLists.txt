# coretech/CMakeLists.txt

cmake_minimum_required(VERSION 3.6)
project(cti)

include(import)

include(opencv)

#set(CTI_COMMON_GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated/clad/common)
#set(CTI_VISION_GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated/clad/vision)
#set(CTI_TYPES_GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated/clad/types)
#
#file(GLOB_RECURSE CTI_GENERATED_SRC
#  ${CMAKE_CURRENT_SOURCE_DIR}/generated/clad/*.cpp
#)
#
#add_library(cti_generated
#  ${CTI_GENERATED_SRC}
#)
#
#target_link_libraries(cti_generated
#  clad
#)
#
#target_include_directories(cti_generated PUBLIC
#  $<BUILD_INTERFACE:${CTI_COMMON_GENERATED_DIR}>
#  $<BUILD_INTERFACE:${CTI_VISION_GENERATED_DIR}>
#  $<BUILD_INTERFACE:${CTI_TYPES_GENERATED_DIR}>
#)

add_subdirectory(common/clad_src/clad)
add_subdirectory(vision/clad_src/)

set(CTI_GENERATED_LIBS
    cti_common_clad
    cti_vision_clad
)

import(cti_common common)
import(cti_vision vision)
import(cti_planning planning)
import(cti_messaging messaging)

#target_include_directories(cti_common INTERFACE
#  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated/clad/common>
#  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated/clad/types>
#)
# 
#target_include_directories(cti_vision INTERFACE
#  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated/clad/vision>
#)
#
#target_include_directories(cti_messaging INTERFACE
#  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated/clad/types>
#)

if (MACOSX)
  enable_testing()
endif(MACOSX)
