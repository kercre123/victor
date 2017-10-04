# coretech/CMakeLists.txt

cmake_minimum_required(VERSION 3.6)
project(cti)

include(import)

include(opencv)

if(USE_TENSORFLOW)
  message(STATUS "Including TensorFlow in coretech-vision")
  include(tensorflow)
endif()

add_definitions(-DUSE_TENSORFLOW=${USE_TENSORFLOW})

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

add_subdirectory(common/clad)
add_subdirectory(vision/clad)

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

if (NOT MACOSX)
    return()
endif()

#find_library(FOUNDATION Foundation)
set(PLATFORM_LIBS
#  ${FOUNDATION}
  "${CMAKE_CURRENT_SOURCE_DIR}/../lib/util/libs/framework/gtest.framework"
)

file(STRINGS "${ANKI_SRCLIST_DIR}/cti_planning_test.srcs.lst" PLANNING_TEST_SRCS)
file(STRINGS "${ANKI_SRCLIST_DIR}/cti_vision_test.srcs.lst" VISION_TEST_SRCS)
file(STRINGS "${ANKI_SRCLIST_DIR}/cti_common_test.srcs.lst" COMMON_TEST_SRCS)

add_executable(test_coretech
    ${PLANNING_TEST_SRCS}
    ${VISION_TEST_SRCS}
    ${COMMON_TEST_SRCS}
)

target_link_libraries(test_coretech
  PRIVATE
  # anki libs
  util
  cti_common
  cti_planning
  cti_vision
  cti_messaging
  # platform
  ${PLATFORM_LIBS}
  ${TENSORFLOW_LIBS}
)

target_compile_definitions(test_coretech PRIVATE                                                       
  SIMULATOR
  CORETECH_BASESTATION                                                                          
  ANKICORETECH_USE_OPENCV=1                                                                     
  ANKICORETECH_EMBEDDED_USE_OPENCV=1
  TEST_DATA_PATH=${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(test_coretech
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/planning/basestation/src>
)
