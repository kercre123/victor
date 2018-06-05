cmake_minimum_required(VERSION 3.6)

project(test_engine)

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

anki_build_cxx_executable(test_engine ${ANKI_SRCLIST_DIR})
anki_build_target_license(test_engine "ANKI")

target_link_libraries(test_engine 
  PRIVATE
  # anki libs
  cozmo_engine
  audio_engine
  cti_common
  cti_planning
  cti_vision
  cti_messaging
  engine_clad
  cameraService
  cubeBleClient
  osState
  util
  gtest
  # platform
  ${PLATFORM_LIBS}
)

target_include_directories(test_engine 
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine>
)

target_compile_definitions(test_engine 
  PRIVATE
  SIMULATOR
  CORETECH_ENGINE
)

#
# Set up symlink to resources
#
symlink_target(TARGET test_engine
  SRC ${BUILT_RESOURCES_DIR}
  DST ${CMAKE_CURRENT_BINARY_DIR}/resources
)

#
# Set up symlinks to satisfy webots dynamic loading
#
symlink_target(TARGET test_engine
  SRC ${WEBOTS_HOME}/lib/libController.dylib
  DST $<TARGET_FILE_DIR:cozmo_engine>/lib/
)

symlink_target(TARGET test_engine
  SRC ${WEBOTS_HOME}/lib/libCppController.dylib
  DST $<TARGET_FILE_DIR:cozmo_engine>/lib/
)

symlink_target(TARGET test_engine
  SRC ${WEBOTS_HOME}/lib/libode.dylib
  DST $<TARGET_FILE_DIR:cozmo_engine>/lib/
)

if (MACOSX)
  enable_testing()
  add_test(NAME test_engine 
    COMMAND test_engine
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

  set_tests_properties(test_engine
    PROPERTIES
    ENVIRONMENT "GTEST_OUTPUT=xml:cozmoEngineGoogleTest.xml;ROOT_DIR=${CMAKE_SOURCE_DIR}"
  )
endif(MACOSX)
