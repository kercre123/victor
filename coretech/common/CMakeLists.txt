# coretech/common/CMakeLists.txt

project(cti_common)

include(anki_build_cxx_library)
anki_build_cxx_library(cti_common ${ANKI_SRCLIST_DIR} STATIC)

target_compile_definitions(cti_common
  PUBLIC
  ANKICORETECH_USE_OPENCV=1
  ANKICORETECH_USE_MATLAB=0
  ANKICORETECH_EMBEDDED_USE_OPENCV=1
  ANKICORETECH_EMBEDDED_USE_MATLAB=0
  PRIVATE
  CORETECH_BASESTATION
)

target_include_directories(cti_common
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/anki/common>
)

#set_target_properties(cti_common PROPERTIES
#  FOLDER "CoreTech"#/Basestation"
#)

target_compile_options(cti_common
  PRIVATE
  -Os
  ${SYMBOL_VISIBILITY_FLAGS}
)

set_property(TARGET cti_common PROPERTY CXX_STANDARD 11)

target_link_libraries(cti_common
  PUBLIC
  ${CTI_GENERATED_LIBS}
  util
  jsoncpp
  PRIVATE
  opencv_interface
  #  ${OPENCV_LIBS}
  z
)

#
# cti_common_robot

anki_build_cxx_library(cti_common_robot ${ANKI_SRCLIST_DIR} STATIC)

target_link_libraries(cti_common_robot
  PRIVATE
  util
  jsoncpp
  opencv_interface
)

target_compile_definitions(cti_common_robot
  PUBLIC
  ANKICORETECH_USE_OPENCV=1
  ANKICORETECH_USE_MATLAB=0
  ANKICORETECH_EMBEDDED_USE_OPENCV=1
  ANKICORETECH_EMBEDDED_USE_MATLAB=0
  PRIVATE
  CORETECH_ROBOT
)

target_include_directories(cti_common_robot
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/anki/common>
)



if (UNIT_TEST)
  add_subdirectory(basestation/test)
endif()

