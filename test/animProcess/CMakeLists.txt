cmake_minimum_required(VERSION 3.6)

project(test_animprocess)

set(BUILT_RESOURCES_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources")

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
set(PLATFORM_FLAGS "")
if (VICOS)
  set(PLATFORM_LIBS log)
elseif (MACOSX)
  include(webots)
  include(gtest)
  find_library(FOUNDATION Foundation)
  set(PLATFORM_LIBS
    ${FOUNDATION}
  )
endif()

include(anki_build_cxx)
include(symlink_target)

anki_build_cxx_executable(test_animprocess ${ANKI_SRCLIST_DIR})

target_link_libraries(test_animprocess 
  PRIVATE
  # anki libs
  victor_anim
  cti_common
  engine_clad
  cameraService
  cubeBleClient
  osState
  util
  gtest
  # platform
  ${PLATFORM_LIBS}
)

target_include_directories(test_animprocess 
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_compile_definitions(test_animprocess 
  PRIVATE
  SIMULATOR
  CORETECH_ENGINE
)

#
# Set up symlink to resources
#
message("BUILT_RESOURCES_DIR ${BUILT_RESOURCES_DIR}")
message("CMAKE_CURRENT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}")
symlink_target(TARGET test_animprocess
  SRC ${BUILT_RESOURCES_DIR}
  DST ${CMAKE_CURRENT_BINARY_DIR}/resources
)

if (MACOSX)
  enable_testing()
  add_test(NAME test_animprocess 
    COMMAND test_animprocess
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

  set_tests_properties(test_animprocess
    PROPERTIES
    ENVIRONMENT "GTEST_OUTPUT=xml:cozmoAnimProcessUnitGoogleTest.xml"
  )
endif(MACOSX)
