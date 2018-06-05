cmake_minimum_required(VERSION 3.6)

project(test-vic-switchboard)

include(anki_build_cxx)

anki_build_cxx_executable(test-vic-switchboard ${ANKI_SRCLIST_DIR})
anki_build_target_license(test-vic-switchboard "ANKI")

target_link_libraries(test-vic-switchboard
  PRIVATE
  sdk_clad_cpp
  cti_messaging
  engine_clad
  anki-ble
  switchboard
)

target_include_directories(test-vic-switchboard 
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)