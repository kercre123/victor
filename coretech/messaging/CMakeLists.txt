# coretech/messaging/CMakeLists.txt

project(cti_messaging)

include(anki_build_cxx)

anki_build_cxx_library(cti_messaging ${ANKI_SRCLIST_DIR} STATIC)
 
set_target_properties(cti_messaging PROPERTIES
  FOLDER "CoreTech"#/Basestation"
)

target_link_libraries(cti_messaging
  clad
)

target_include_directories(cti_messaging 
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)

anki_build_cxx_library(cti_messaging_robot ${ANKI_SRCLIST_DIR} STATIC)
 
target_include_directories(cti_messaging_robot 
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)

target_link_libraries(cti_messaging_robot
  PUBLIC
  cti_common_robot
)
