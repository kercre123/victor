# coretech/common/CMakeLists.txt

project(cti_common)

if (NOT DEFINED CORETECH)
  set(CORETECH ${CMAKE_CURRENT_SOURCE_DIR}/..)
endif()

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
  opencv_interface
  ${OPENCV_LIBS}
  PRIVATE
  z
)

#
# cti_common_robot
#

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


if (MACOSX)

  include(gtest)
  #
  # cti_common_shared_test
  #

  anki_build_cxx_executable(cti_common_shared_test ${ANKI_SRCLIST_DIR})

  target_compile_options(cti_common_shared_test
    PRIVATE
    -Wno-undef
  )

  target_compile_definitions(cti_common_shared_test
    PRIVATE
    TEST_DATA_PATH=${CORETECH}
  )

  target_link_libraries(cti_common_shared_test
    PRIVATE
    cti_common
    util
    gtest
    ${OPENCV_LIBS}
  )

  #
  # cti_common_basestation_test
  #

  anki_build_cxx_executable(cti_common_basestation_test ${ANKI_SRCLIST_DIR})

  target_compile_definitions(cti_common_basestation_test
    PRIVATE
    TEST_DATA_PATH=${CORETECH}
  )

  target_link_libraries(cti_common_basestation_test
    PRIVATE
    cti_common
    util
    gtest
    ${OPENCV_LIBS}
  )

  #
  # test harness
  #
  enable_testing()

  add_test(NAME cti_common_shared_test COMMAND cti_common_shared_test)
  add_test(NAME cti_common_basestation_test COMMAND cti_common_basestation_test)

endif()

